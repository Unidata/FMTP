/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpRecvv3.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 17, 2014
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     Define the entity of VCMTPv3 receiver side method function.
 *
 * Receiver side of VCMTPv3 protocol. It handles incoming multicast packets
 * and issues retransmission requests to the sender side.
 */


#include "vcmtpRecvv3.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <system_error>


/**
 * Constructs the receiver side instance (for integration with LDM).
 *
 * @param[in] tcpAddr       Tcp unicast address for retransmission.
 * @param[in] tcpPort       Tcp unicast port for retransmission.
 * @param[in] mcastAddr     Udp multicast address for receiving data products.
 * @param[in] mcastPort     Udp multicast port for receiving data products.
 * @param[in] notifier      Callback function to notify receiving application
 *                          of incoming Begin-Of-Product messages.
 */
vcmtpRecvv3::vcmtpRecvv3(
    std::string          tcpAddr,
    const unsigned short tcpPort,
    std::string          mcastAddr,
    const unsigned short mcastPort,
    RecvAppNotifier*     notifier)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    tcprecv(new TcpRecv(tcpAddr, tcpPort)),
    prodptr(0),
    notifier(notifier),
    mcastSock(0),
    retxSock(0),
    msgQfilled(),
    msgQmutex(),
    BOPListMutex(),
    bitmap(0),
    EOPStatus(false),
    exitMutex(),
    except(),
    exceptIsSet(false),
    retx_rq(),
    retx_t(),
    mcast_t(),
    timer_t(),
    newinterface(false)
{
}


/**
 * Constructs the receiver side instance (for independent tests).
 *
 * @param[in] tcpAddr       Tcp unicast address for retransmission.
 * @param[in] tcpPort       Tcp unicast port for retransmission.
 * @param[in] mcastAddr     Udp multicast address for receiving data products.
 * @param[in] mcastPort     Udp multicast port for receiving data products.
 */
vcmtpRecvv3::vcmtpRecvv3(
    std::string          tcpAddr,
    const unsigned short tcpPort,
    std::string          mcastAddr,
    const unsigned short mcastPort)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    tcprecv(new TcpRecv(tcpAddr, tcpPort)),
    prodptr(0),
    /**
     * constructor called by independent test program will set notifier to NULL
     */
    notifier(0),
    mcastSock(0),
    retxSock(0),
    msgQfilled(),
    msgQmutex(),
    BOPListMutex(),
    bitmap(0),
    EOPStatus(false),
    exitMutex(),
    except(),
    exceptIsSet(false),
    retx_rq(),
    retx_t(),
    mcast_t(),
    timer_t(),
    newinterface(false)
{
}


/**
 * Destroys the receiver side instance. Releases the resources.
 *
 * @param[in] none
 */
vcmtpRecvv3::~vcmtpRecvv3()
{
    Stop();
    close(mcastSock);
    (void)close(retxSock); // failure is irrelevant
    misBOPlist.clear();
    delete tcprecv;
    if (bitmap)
        delete bitmap;
}


/**
 * A public setter of link speed. The setter is thread-safe, but a recommended
 * way is to set the link speed before the receiver starts. Due to the feature
 * of virtual circuits, the link speed won't change when it's set up. So the
 * link speed remains for the whole life of the receiver. As the possible link
 * speed nowadays could possibly be 10Gbps or even higher, a 64-bit unsigned
 * integer is used to hold the value, which can be up to 18000 Pbps.
 *
 * @param[in] speed                 User-specified link speed
 */
void vcmtpRecvv3::SetLinkSpeed(uint64_t speed)
{
    std::unique_lock<std::mutex> lock(linkmtx);
    linkspeed = speed;
}


/**
 * Connect to sender via TCP socket, join given multicast group (defined by
 * mcastAddr:mcastPort) to receive multicasting products. Start retransmission
 * handler and retransmission requester and also start multicast receiving
 * thread. Doesn't return until `vcmtpRecvv3::Stop` is called or an exception
 * is thrown. Exceptions will be caught and all the threads will be terminated
 * by the exception handling code.
 *
 * @throw std::system_error  if the multicast group couldn't be joined.
 * @throw std::system_error  if an I/O error occurs.
 * @throw std::system_error  if the multicast-receiving thread couldn't be
 *                           created.
 */
void vcmtpRecvv3::Start()
{
    /** connect to the sender */
    tcprecv->Init();

    /** set prodindex to max to avoid BOP missing for prodindex=0 */
    vcmtpHeader.prodindex = 0xFFFFFFFF;
    /** clear EOPStatus for new product */
    clearEOPState();
    joinGroup(mcastAddr, mcastPort);
    if (newinterface) {
        if (setDefaultIF() < 0) {
            throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::Start(): Couldn't set default interface");
        }
    }

    StartRetxProcedure();
    startTimerThread();

    int status = pthread_create(&mcast_t, NULL, &vcmtpRecvv3::StartMcastHandler,
                                this);
    if (status) {
        Stop();
        throw std::system_error(status, std::system_category(),
            "vcmtpRecvv3::Start(): Couldn't start multicast-receiving thread");
    }
    (void)pthread_join(mcast_t, NULL);
    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (exceptIsSet)
            throw except;
    }
}


/**
 * Stops a running VCMTP receiver. Idempotent.
 *
 * @pre  `vcmtpRecvv3::Start()` was previously called.
 */
void vcmtpRecvv3::Stop()
{
    (void)pthread_cancel(timer_t); /* failure is irrelevant */
    (void)pthread_cancel(mcast_t); /* failure is irrelevant */
    (void)pthread_cancel(retx_rq); /* failure is irrelevant */
    (void)pthread_cancel(retx_t);  /* failure is irrelevant */
}


/**
 * Stores the interface IP and sets boolean flag.
 *
 * @param[in] ifaceip         IP address of the specified interface.
 */
void vcmtpRecvv3::SetDefaultIF(const std::string ifaceip)
{
    ifAddr = ifaceip;
    newinterface = true;
}


/**
 * Add the unrequested BOP identified by the given prodindex into the list.
 * If the BOP is already in the list, return with a false. If it's not, add
 * it into the list and return with a true.
 *
 * @param[in] prodindex        Product index of the missing BOP
 */
bool vcmtpRecvv3::addUnrqBOPinList(uint32_t prodindex)
{
    bool addsuccess;
    std::list<uint32_t>::iterator it;
    std::unique_lock<std::mutex>  lock(BOPListMutex);
    for(it=misBOPlist.begin(); it!=misBOPlist.end(); ++it) {
        if (*it == prodindex) {
            addsuccess = false;
            return addsuccess;
        }
    }
    misBOPlist.push_back(prodindex);
    addsuccess = true;
    return addsuccess;
}


/**
 * Handles a multicast BOP message given its peeked-at and decoded VCMTP header.
 *
 * @pre                       The multicast socket contains a VCMTP BOP packet.
 * @param[in] header          The associated, peeked-at and already-decoded
 *                            VCMTP header.
 * @throw std::system_error   if an error occurs while reading the socket.
 * @throw std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::BOPHandler(const VcmtpHeader& header)
{
    char          pktBuf[MAX_VCMTP_PACKET_LEN];
    const ssize_t nbytes = recv(mcastSock, pktBuf, MAX_VCMTP_PACKET_LEN, 0);

    if (nbytes < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::BOPHandler() recv() error.");

    checkPayloadLen(header, nbytes);
    BOPHandler(header, pktBuf + VCMTP_HEADER_LEN);
}


/**
 * Parse BOP message and call notifier to notify receiving application.
 *
 * @param[in] header           Header associated with the packet.
 * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
 * @throw std::runtime_error   if the payload is too small.
 * @throw std::runtime_error   if the amount of metadata is invalid.
 */
void vcmtpRecvv3::BOPHandler(const VcmtpHeader& header,
                             const char* const  VcmtpPacketData)
{
    /**
     * Every time a new BOP arrives, save the msg to check following data
     * packets
     */
    if (header.payloadlen < 6)
        throw std::runtime_error("vcmtpRecvv3::BOPHandler(): packet too small");
    BOPmsg.prodsize = ntohl(*(uint32_t*)VcmtpPacketData);
    BOPmsg.metasize = ntohs(*(uint16_t*)(VcmtpPacketData+4));
    BOPmsg.metasize = BOPmsg.metasize > AVAIL_BOP_LEN
                      ? AVAIL_BOP_LEN : BOPmsg.metasize;
    if (header.payloadlen - 6 < BOPmsg.metasize)
        throw std::runtime_error("vcmtpRecvv3::BOPHandler(): Metasize too big");
    (void)memcpy(BOPmsg.metadata, VcmtpPacketData+6, BOPmsg.metasize);

    /**
     * Every time a new BOP arrives, save the header to check following data
     * packets.
     */
    {
        std::unique_lock<std::mutex> lock(vcmtpHeaderMutex);
        vcmtpHeader = header;
        vcmtpHeader.seqnum     = 0;
        vcmtpHeader.payloadlen = 0;
    }

    #ifdef DEBUG2
        std::string debugmsg = "Product #" +
            std::to_string(vcmtpHeader.prodindex);
        debugmsg += ": BOP is received. Product size = ";
        debugmsg += std::to_string(BOPmsg.prodsize);
        debugmsg += ", Metadata size = ";
        debugmsg += std::to_string(BOPmsg.metasize);
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    /** forcibly terminate the previous timer */
    timerWake.notify_all();

    if(notifier)
        notifier->notify_of_bop(BOPmsg.prodsize, BOPmsg.metadata,
                                BOPmsg.metasize, &prodptr);

    if (bitmap) {
        delete bitmap;
        bitmap = 0;
    }
    uint32_t blocknum = BOPmsg.prodsize ?
        (BOPmsg.prodsize - 1) / VCMTP_DATA_LEN + 1 : 0;
    // TODO: what if blocknum = 0?
    bitmap = new ProdBitMap(blocknum);

    /**
     * clear EOPStatus for new product. due to the sequencial feature that VC
     * has, if new BOP arrives and previous EOP is missing, then the EOP is
     * really missing. In which case, should be requested.
     */
    clearEOPState();

    /**
     * Set the amount of sleeping time. Based on a precise network delay model,
     * total delay should include transmission delay, propogation delay,
     * processing delay and queueing delay. From a pragmatic view, the queueing
     * delay, processing delay of routers and switches, and propogation delay
     * are all added into RTT. Thus only processing delay of the receiver and
     * transmission delay need to be considered except for RTT. Processing
     * delay is usually nanoseconds to microseconds, which can be ignored. And
     * since the receiver timer starts after BOP is received, the RTT is not
     * affecting the timer model. Sleeptime here means the estimated remaining
     * time of the current product. Thus, the only thing needs to be considered
     * is the transmission delay, which can be calculated as product size over
     * link speed. Besides, a little more extra time would be favorable to
     * tolerate possible fluctuation.
     */
    float sleeptime = 1.5 * (BOPmsg.prodsize / linkspeed);
    /** add the new product into timer queue */
    {
        std::unique_lock<std::mutex> lock(timerQmtx);
        //timerParam timerparam = {vcmtpHeader.prodindex, sleeptime};
        timerParam timerparam = {vcmtpHeader.prodindex, 0.2};
        timerParamQ.push(timerparam);
        timerQfilled.notify_all();
    }
}


/**
 * Checks the length of the payload of a VCMTP packet -- as stated in the VCMTP
 * header -- against the actual length of a VCMTP packet.
 *
 * @param[in] header              The decoded VCMTP header.
 * @param[in] nbytes              The size of the VCMTP packet in bytes.
 * @throw     std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::checkPayloadLen(const VcmtpHeader& header, const size_t nbytes)
{
    if (header.payloadlen != nbytes - VCMTP_HEADER_LEN)
        throw std::runtime_error("vcmtpRecvv3::checkPayloadLen(): "
                "Invalid payload length");
}


/**
 * Clears the EOPStatus to false as initialization for new product.
 *
 * @param[in] none
 */
void vcmtpRecvv3::clearEOPState()
{
    std::unique_lock<std::mutex> lock(EOPStatMtx);
    EOPStatus = false;
}


/**
 * Decodes the header of a VCMTP packet in-place. It only does the network
 * order to host order translation.
 *
 * @param[in,out] header  The VCMTP header to be decoded.
 */
void vcmtpRecvv3::decodeHeader(VcmtpHeader& header)
{
    header.prodindex  = ntohl(header.prodindex);
    header.seqnum     = ntohl(header.seqnum);
    header.payloadlen = ntohs(header.payloadlen);
    header.flags      = ntohs(header.flags);
}


/**
 * Decodes a VCMTP packet header. It does the network order to host order
 * translation as well as parsing the payload into a given buffer. Besides,
 * it also does a header size check.
 *
 * @param[in]  packet         The raw packet.
 * @param[in]  nbytes         The size of the raw packet in bytes.
 * @param[out] header         The decoded packet header.
 * @param[out] payload        Payload of the packet.
 * @throw std::runtime_error  if the packet is too small.
 */
void vcmtpRecvv3::decodeHeader(char* const  packet, const size_t nbytes,
                               VcmtpHeader& header, char** const payload)
{
    if (nbytes < VCMTP_HEADER_LEN)
        throw std::runtime_error(
                std::string("vcmtpRecvv3::decodeHeader(): Packet is too small: ")
                + std::to_string(static_cast<long long>(nbytes)) + " bytes");

    header.prodindex  = ntohl(*(uint32_t*)packet);
    header.seqnum     = ntohl(*(uint32_t*)(packet+4));
    header.payloadlen = ntohs(*(uint16_t*)(packet+8));
    header.flags      = ntohs(*(uint16_t*)(packet+10));

    *payload = packet + VCMTP_HEADER_LEN;
}


/**
 * Handles a received EOP from the unicast thread. Check the bitmap to see if
 * all the data blocks are received. If true, notify the RecvApp. If false,
 * request for retransmission if it has to be so.
 *
 * @param[in] VcmtpHeader    Reference to the received VCMTP packet header
 */
void vcmtpRecvv3::EOPHandler(const VcmtpHeader& header)
{
    /**
     * Under the assumption that all packets are in sequence, the EOP should
     * always come in after the BOP. In which case, BOP and EOP are mapped
     * one to one. When timer thread checks the EOPStatus, it always reflects
     * the status of current EOP which associates with the latest BOP.
     */
    setEOPReceived();

    #ifdef DEBUG2
        std::string debugmsg = "Product #" + std::to_string(header.prodindex);
        debugmsg += ": EOP is received";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    if (bitmap) {
        /**
         * if bitmap check tells everything is completed, then sends the
         * RETX_END message back to sender. Meanwhile notify receiving
         * application.
         */
        if (bitmap->isComplete()) {
            sendRetxEnd(header.prodindex);
            if (notifier)
                notifier->notify_of_eop();

            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(header.prodindex);
                debugmsg += " has been completely received";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #elif DEBUG1
                std::string debugmsg = "Product #" +
                    std::to_string(header.prodindex);
                debugmsg += " has been completely received";
                std::cout << debugmsg << std::endl;
            #endif
        }
        else {
            /**
             * check if the last data block has been received. If true, then
             * all the other missing blocks have been requested. In this case,
             * receiver just needs to wait until product being all completed.
             * Otherwise, last block is missing as well, receiver needs to
             * request retx for all the missing blocks including the last one.
             */
            if (!hasLastBlock()) {
                requestAnyMissingData(BOPmsg.prodsize);
            }
        }
    }
}


/**
 * Check if the last data block has been received.
 *
 * @param[in] none
 */
bool vcmtpRecvv3::hasLastBlock()
{
    std::unique_lock<std::mutex> lock(vcmtpHeaderMutex);
    /**
     * seqnum + payloadlen should always be equal to or smaller than prodsize
     */
    return (vcmtpHeader.seqnum + vcmtpHeader.payloadlen == BOPmsg.prodsize);
}


/**
 * Returns the current EOPStatus.
 *
 * @param[in] none
 */
bool vcmtpRecvv3::isEOPReceived()
{
    std::unique_lock<std::mutex> lock(EOPStatMtx);
    return EOPStatus;
}


/**
 * Join multicast group specified by mcastAddr:mcastPort.
 *
 * @param[in] mcastAddr      Udp multicast address for receiving data products.
 * @param[in] mcastPort      Udp multicast port for receiving data products.
 * @throw std::system_error  if the socket couldn't be created.
 * @throw std::system_error  if the socket couldn't be bound.
 * @throw std::system_error  if the socket couldn't join the multicast group.
 */
void vcmtpRecvv3::joinGroup(
        std::string          mcastAddr,
        const unsigned short mcastPort)
{
    (void) memset(&mcastgroup, 0, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    mcastgroup.sin_addr.s_addr = htonl(INADDR_ANY);
    mcastgroup.sin_port = htons(mcastPort);
    if((mcastSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() creating socket failed");
    if (::bind(mcastSock, (struct sockaddr *) &mcastgroup, sizeof(mcastgroup))
            < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup(): Couldn't bind socket " +
                std::to_string(static_cast<long long>(mcastSock)) +
                               " to multicast group " + mcastgroup);
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if( setsockopt(mcastSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(mreq)) < 0 )
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() setsockopt() failed");
}


/**
 * Handles multicast packets. To avoid extra copying operations, here recv()
 * is called with a MSG_PEEK flag to only peek the header instead of reading
 * it out (which would cause the buffer to be wiped). And the recv() call
 * will block if there is no data coming to the mcastSock.
 *
 * @throw std::system_error   if an I/O error occurs.
 * @throw std::runtime_error  if a packet is invalid.
 */
void vcmtpRecvv3::mcastHandler()
{
    while(1)
    {
        VcmtpHeader   header;
        const ssize_t nbytes = recv(mcastSock, &header, sizeof(header),
                                    MSG_PEEK);
        /*
         * Allow the current thread to be cancelled only when it is likely
         * blocked attempting to read from the multicast socket because that
         * prevents the receiver from being put into an inconsistent state yet
         * allows for fast termination.
         */
        int initState;
        (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &initState);

        if (nbytes < 0)
            throw std::system_error(errno, std::system_category(),
                    "vcmtpRecvv3::mcastHandler() recv() error.");
        if (nbytes != sizeof(header))
            throw std::runtime_error("Invalid packet length");

        decodeHeader(header);

        if (header.flags == VCMTP_BOP) {
            BOPHandler(header);

            #ifdef MEASURE
                recvbytes = 0;
                std::string measure = "Product #" +
                    std::to_string(header.prodindex);
                measure += ": Transmission start time (BOP)";
                std::cout << measure << std::endl;
                /* set rxdone to false upon receiving BOP */
                rxdone = false;
                start_t = std::chrono::high_resolution_clock::now();
                WriteToLog(measure);
            #endif
        }
        else if (header.flags == VCMTP_MEM_DATA) {
            recvMemData(header);

            #ifdef MEASURE
                /* header size + payload length equals whole block size */
                recvbytes += nbytes;
                recvbytes += header.payloadlen;
            #endif
        }
        else if (header.flags == VCMTP_EOP) {
            mcastEOPHandler(header);

            #ifdef MEASURE
                /* header size + payload length equals whole block size */
                recvbytes += nbytes;
                recvbytes += header.payloadlen;
                std::string measure = "Product #" +
                    std::to_string(header.prodindex);
                measure += ": Transmission end time (EOP), received bytes = ";
                measure += std::to_string(recvbytes);
                std::cout << measure << std::endl;
                /* set rxdone to true upon receiving EOP */
                rxdone = true;
                end_t = std::chrono::high_resolution_clock::now();
                WriteToLog(measure);
            #endif
        }

        int ignoredState;
        (void)pthread_setcancelstate(initState, &ignoredState);
    }
}


/**
 * Handles a received EOP from the multicast thread. Since the data is only
 * fetched with a MSG_PEEK flag, it's necessary to remove the data by calling
 * recv() again without MSG_PEEK.
 *
 * @param[in] VcmtpHeader    Reference to the received VCMTP packet header
 */
void vcmtpRecvv3::mcastEOPHandler(const VcmtpHeader& header)
{
    char          pktBuf[VCMTP_HEADER_LEN];
    /** read the EOP packet out in order to remove it from buffer */
    const ssize_t nbytes = recv(mcastSock, pktBuf, VCMTP_HEADER_LEN, 0);

    if (nbytes < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::EOPHandler() recv() error.");

    setEOPReceived();
    timerWake.notify_all();
    EOPHandler(header);
}


/**
 * Pushes a request for a data-packet onto the retransmission-request queue.
 *
 * @pre                  The retransmission-request queue is locked.
 * @param[in] prodindex  Index of the associated data-product.
 * @param[in] seqnum     Sequence number of the data-packet.
 * @param[in] datalen    Amount of data in bytes.
 */
void vcmtpRecvv3::pushMissingDataReq(const uint32_t prodindex,
                                     const uint32_t seqnum,
                                     const uint16_t datalen)
{
    INLReqMsg reqmsg = {MISSING_DATA, prodindex, seqnum, datalen};
    msgqueue.push(reqmsg);
}


/**
 * Pushes a request for a BOP-packet onto the retransmission-request queue.
 *
 * @param[in] prodindex  Index of the associated data-product.
 */
void vcmtpRecvv3::pushMissingBopReq(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(msgQmutex);
    INLReqMsg                    reqmsg = {MISSING_BOP, prodindex, 0, 0};
    msgqueue.push(reqmsg);
    msgQfilled.notify_one();
}


/**
 * Pushes a request for a EOP-packet onto the retransmission-request queue.
 *
 * @param[in] prodindex  Index of the associated data-product.
 */
void vcmtpRecvv3::pushMissingEopReq(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(msgQmutex);
    INLReqMsg                    reqmsg = {MISSING_EOP, prodindex, 0, 0};
    msgqueue.push(reqmsg);
    msgQfilled.notify_one();
}


/**
 * Handles all kinds of packets received from the unicast connection. Since
 * the underlying layer offers a stream-based reliable transmission, MSG_PEEK
 * is not necessary any more to reduce extra copies. It's always possible to
 * read the amount of bytes equaling to VCMTP_HEADER_LEN first, and read the
 * remaining payload next.
 *
 * @param[in] none
 */
void vcmtpRecvv3::retxHandler()
{
    char pktHead[VCMTP_HEADER_LEN];
    (void) memset(pktHead, 0, sizeof(pktHead));
    VcmtpHeader header;

    while(1)
    {
        /** temp buffer, do not access in case of out of bound issues */
        char* paytmp;
        ssize_t nbytes = tcprecv->recvData(pktHead, VCMTP_HEADER_LEN, NULL, 0);
        /*
         * Allow the current thread to be cancelled only when it is likely
         * blocked attempting to read from the unicast socket because that
         * prevents the receiver from being put into an inconsistent state yet
         * allows for fast termination.
         */
        int ignoredState;
        int initState;
        (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &initState);

        decodeHeader(pktHead, nbytes, header, &paytmp);

        if (header.flags == VCMTP_RETX_BOP) {
            (void)pthread_setcancelstate(initState, &ignoredState);
            tcprecv->recvData(NULL, 0, paytmp, header.payloadlen);
            (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ignoredState);

            BOPHandler(header, paytmp);

            /** remove the BOP from missing list */
            (void)rmMisBOPinList(header.prodindex);

            /**
             * Under the assumption that all packets are coming in sequence,
             * when a missing BOP is retransmitted, all the following data
             * blocks will be missing as well. Thus retxHandler should issue
             * RETX_REQ for all data blocks as well as EOP packet.
             */
            requestAnyMissingData(BOPmsg.prodsize);
            pushMissingEopReq(header.prodindex);
        }
        else if (header.flags == VCMTP_RETX_DATA) {
            /**
             * directly writing unwanted data to NULL is not allowed. So here
             * uses a temp buffer as trash to dump the payload content.
             */
            char tmp[VCMTP_DATA_LEN];

            if(prodptr) {
                (void)pthread_setcancelstate(initState, &ignoredState);
                tcprecv->recvData(NULL, 0, (char*)prodptr + header.seqnum,
                                  header.payloadlen);
                (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                        &ignoredState);
            }
            else {
                /** dump the payload since there is no product queue */
                (void)pthread_setcancelstate(initState, &ignoredState);
                tcprecv->recvData(NULL, 0, tmp, header.payloadlen);
                (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                        &ignoredState);
            }

            bitmap->set(header.seqnum/VCMTP_DATA_LEN);
            if (bitmap && bitmap->isComplete()) {
                sendRetxEnd(header.prodindex);
                if (notifier)
                    notifier->notify_of_eop();

                #ifdef DEBUG2
                    std::string debugmsg = "Product #" +
                        std::to_string(header.prodindex);
                    debugmsg += " has been completely received";
                    std::cout << debugmsg << std::endl;
                    WriteToLog(debugmsg);
                #elif DEBUG1
                    std::string debugmsg = "Product #" +
                        std::to_string(header.prodindex);
                    debugmsg += " has been completely received";
                    std::cout << debugmsg << std::endl;
                #endif
            }

            #ifdef DEBUG2
                if (bitmap && !bitmap->isComplete()) {
                    std::string debugmsg = "Product #" +
                        std::to_string(header.prodindex);
                    debugmsg += ": RETX data block (SeqNum = ";
                    debugmsg += std::to_string(header.seqnum);
                    debugmsg += ") is received";
                    std::cout << debugmsg << std::endl;
                    WriteToLog(debugmsg);
                }
            #endif
        }
        else if (header.flags == VCMTP_RETX_EOP) {
            retxEOPHandler(header);
        }

        (void)pthread_setcancelstate(initState, &ignoredState);
    }
}


/**
 * Fetch the requests from an internal message queue and call corresponding
 * handler to send requests respectively. The read operation on the internal
 * message queue will block if the queue is empty itself. The existing request
 * being handled will only be removed from the queue if the handler returns a
 * successful state.
 *
 * @param[in] none
 */
void vcmtpRecvv3::retxRequester()
{
    while(1)
    {
        INLReqMsg reqmsg;

        {
            std::unique_lock<std::mutex> lock(msgQmutex);
            while (msgqueue.empty())
                msgQfilled.wait(lock);
            reqmsg = msgqueue.front();
        }

        if ( ((reqmsg.reqtype == MISSING_BOP) &&
                sendBOPRetxReq(reqmsg.prodindex)) ||
            ((reqmsg.reqtype == MISSING_DATA) &&
                sendDataRetxReq(reqmsg.prodindex, reqmsg.seqnum,
                                reqmsg.payloadlen)) ||
            ((reqmsg.reqtype == MISSING_EOP) &&
                sendEOPRetxReq(reqmsg.prodindex)) )
        {
            std::unique_lock<std::mutex> lock(msgQmutex);
            msgqueue.pop();
        }
    }
}


/**
 * Remove the BOP identified by the given prodindex out of the list. If the
 * BOP is not in the list, return a false. Or if it's in the list, remove it
 * and reurn a true.
 *
 * @param[in] prodindex        Product index of the missing BOP
 */
bool vcmtpRecvv3::rmMisBOPinList(uint32_t prodindex)
{
    bool rmsuccess;
    std::list<uint32_t>::iterator it;
    std::unique_lock<std::mutex>  lock(BOPListMutex);

    for(it=misBOPlist.begin(); it!=misBOPlist.end(); ++it) {
        if (*it == prodindex) {
            misBOPlist.erase(it);
            rmsuccess = true;
            break;
        }
        else
            rmsuccess = false;
    }
    return rmsuccess;
}


/**
 * Handles a received EOP from the unicast thread. No need to remove the data,
 * just call the handling process directly.
 *
 * @param[in] VcmtpHeader    Reference to the received VCMTP packet header
 */
void vcmtpRecvv3::retxEOPHandler(const VcmtpHeader& header)
{
    EOPHandler(header);
}


/**
 * Reads the data portion of a VCMTP data-packet into the location specified
 * by the receiving application given the associated, peeked-at, and decoded
 * VCMTP header.
 *
 * @pre                       The socket contains a VCMTP data-packet.
 * @param[in] header          The associated, peeked-at, and decoded header.
 * @throw std::system_error   if an error occurs while reading the multicast
 *                            socket.
 * @throw std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::readMcastData(const VcmtpHeader& header)
{
    ssize_t nbytes;

    if (0 == prodptr) {
        char pktbuf[MAX_VCMTP_PACKET_LEN];
        nbytes = read(mcastSock, &pktbuf, sizeof(pktbuf));
    }
    else {
        struct iovec iovec[2];
        VcmtpHeader  headBuf; // ignored because already have peeked-at header

        iovec[0].iov_base = &headBuf;
        iovec[0].iov_len  = sizeof(headBuf);
        iovec[1].iov_base = (char*)prodptr + header.seqnum;
        iovec[1].iov_len  = header.payloadlen;

        nbytes = readv(mcastSock, iovec, 2);
    }

    if (nbytes == -1) {
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::readMcastData(): read() failure.");
    }
    else {
        checkPayloadLen(header, nbytes);

        if (0 == prodptr) {
            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(header.prodindex);
                debugmsg += ": Data block is received. SeqNum = ";
                debugmsg += std::to_string(header.seqnum);
                debugmsg += ", Paylen = ";
                debugmsg += std::to_string(header.payloadlen);
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
        }
        /** receiver should trust the packet from sender is legal */
        bitmap->set(header.seqnum/VCMTP_DATA_LEN);
    }
}


/**
 * Requests data-packets that lie between the last previously-received
 * data-packet of the current data-product and its most recently-received
 * data-packet.
 *
 * @pre               The most recently-received data-packet is for the
 *                    current data-product.
 * @param[in] seqnum  The most recently-received data-packet of the current
 *                    data-product.
 */
void vcmtpRecvv3::requestAnyMissingData(const uint32_t mostRecent)
{
    std::unique_lock<std::mutex> lock(vcmtpHeaderMutex);
    uint32_t seqnum = vcmtpHeader.seqnum + vcmtpHeader.payloadlen;
    uint32_t prodindex = vcmtpHeader.prodindex;
    lock.unlock();

    if (seqnum != mostRecent) {
        seqnum = (seqnum / VCMTP_DATA_LEN) * VCMTP_DATA_LEN;
        /**
         * The data-packet associated with the VCMTP header is out-of-order.
         */
        std::unique_lock<std::mutex> lock(msgQmutex);

        for (; seqnum < mostRecent; seqnum += VCMTP_DATA_LEN) {
            pushMissingDataReq(prodindex, seqnum, VCMTP_DATA_LEN);

            #ifdef DEBUG2
                std::string debugmsg = "Product #" +
                    std::to_string(prodindex);
                debugmsg += ": Data block is missing. SeqNum = ";
                debugmsg += std::to_string(seqnum);
                debugmsg += ". Requesting retx";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
        }
        msgQfilled.notify_one();
    }
}


/**
 * Requests BOP packets for data-products that come after the current
 * data-product up to and including a given data-product but only if the BOP
 * hasn't already been requested (i.e., each missed BOP is requested only once).
 *
 * @param[in] prodindex  Index of the last data-product whose BOP packet was
 *                       missed.
 */
void vcmtpRecvv3::requestMissingBops(const uint32_t prodindex)
{
    // Careful! Product-indexes wrap around!
    std::unique_lock<std::mutex> lock(vcmtpHeaderMutex);
    for (uint32_t i = vcmtpHeader.prodindex; i++ != prodindex;) {
        if (addUnrqBOPinList(i)) {
            pushMissingBopReq(i);
        }
    }
}


/**
 * Handles a multicast VCMTP data-packet given the associated peeked-at and
 * decoded VCMTP header. Directly store and check for missing blocks.
 *
 * @pre                       The socket contains a VCMTP data-packet.
 * @param[in] header          The associated, peeked-at and decoded header.
 * @throw std::system_error   if an error occurs while reading the socket.
 * @throw std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::recvMemData(const VcmtpHeader& header)
{
    if (header.prodindex == vcmtpHeader.prodindex) {
        /*
         * The data-packet is for the current data-product.
         */
        readMcastData(header);
        requestAnyMissingData(header.seqnum);
        vcmtpHeader = header;
    }
    else {
        /*
         * The data-packet is not for the current data-product. At least one BOP
         * packet was missed.
         */
        char buf[1];
        (void)recv(mcastSock, buf, 1, 0); // skip unusable datagram
        requestMissingBops(header.prodindex);
    }
}


/**
 * Request for EOP retransmission if the EOP is not received. This function is
 * an integration of isEOPReceived() and pushMissingEopReq() but being made
 * atomic. If the EOP is requested by this function, it returns a boolean true.
 * Otherwise, if not requested, it returns a boolean false.
 *
 * @param[in] prodindex        Product index which the EOP is using.
 */
bool vcmtpRecvv3::reqEOPifMiss(const uint32_t prodindex)
{
    bool hasReq = false;
    std::unique_lock<std::mutex> lock(EOPStatMtx);
    if (!EOPStatus) {
        pushMissingEopReq(prodindex);
        hasReq = true;
    }
    return hasReq;
}


/**
 * Start the actual timer thread.
 *
 * @param[in] *ptr    A pointer to an vcmtpRecvv3 instance.
 */
void* vcmtpRecvv3::runTimerThread(void* ptr)
{
    vcmtpRecvv3* const recvr = static_cast<vcmtpRecvv3*>(ptr);
    try {
        recvr->timerThread();
    }
    catch (std::exception& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Sets the interface associated with the given IP address as default.
 *
 * @return                    0 indicates success, -1 indicates failure.
 */
int vcmtpRecvv3::setDefaultIF()
{
    struct in_addr ifaddr;
    ifaddr.s_addr = inet_addr(ifAddr.c_str());
    if (setsockopt(mcastSock, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr,
                   sizeof(ifaddr)) < 0 ) {
        return -1;
    }
    else {
        return 0;
    }
}


/**
 * Sends a request for retransmission of the missing BOP identified by the
 * given product index.
 *
 * @param[in] prodindex        The product index of the requested BOP.
 */
bool vcmtpRecvv3::sendBOPRetxReq(uint32_t prodindex)
{
    VcmtpHeader header;
    header.prodindex  = htonl(prodindex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_BOP_REQ);

    return (-1 != tcprecv->sendData(&header, sizeof(VcmtpHeader), NULL, 0));
}


/**
 * Sends a request for retransmission of the missing EOP identified by the
 * given product index.
 *
 * @param[in] prodindex        The product index of the requested EOP.
 */
bool vcmtpRecvv3::sendEOPRetxReq(uint32_t prodindex)
{
    VcmtpHeader header;
    header.prodindex  = htonl(prodindex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_EOP_REQ);

    return (-1 != tcprecv->sendData(&header, sizeof(VcmtpHeader), NULL, 0));
}


/**
 * Sends a request for retransmission of the missing block. The sequence
 * number and payload length are guaranteed to be aligned to the boundary of
 * a legal block.
 *
 * @param[in] prodindex        The product index of the requested block.
 * @param[in] seqnum           The sequence number of the requested block.
 * @param[in] payloadlen       The block size of the requested block.
 */
bool vcmtpRecvv3::sendDataRetxReq(uint32_t prodindex, uint32_t seqnum,
                                  uint16_t payloadlen)
{
    VcmtpHeader header;
    header.prodindex  = htonl(prodindex);
    header.seqnum     = htonl(seqnum);
    header.payloadlen = htons(payloadlen);
    header.flags      = htons(VCMTP_RETX_REQ);

    return (-1 != tcprecv->sendData(&header, sizeof(VcmtpHeader), NULL, 0));
}


/**
 * Sends a retransmission end message to the sender to indicate the product
 * indexed by prodindex has been completely received.
 *
 * @param[in] prodindex        The product index of the finished product.
 */
bool vcmtpRecvv3::sendRetxEnd(uint32_t prodindex)
{
    VcmtpHeader header;
    header.prodindex  = htonl(prodindex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_RETX_END);

    return (-1 != tcprecv->sendData(&header, sizeof(VcmtpHeader), NULL, 0));
}


/**
 * Start the retxRequester thread using a passed-in vcmtpRecvv3 pointer.
 *
 * @param[in] *ptr        A pointer to the pre-defined data structure in the
 *                        caller. Here it's the pointer to a vcmtpRecvv3 class.
 */
void* vcmtpRecvv3::StartRetxRequester(void* ptr)
{
    vcmtpRecvv3* const recvr = static_cast<vcmtpRecvv3*>(ptr);
    try {
        recvr->retxRequester();
    }
    catch (const std::exception& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Start the retxHandler thread using a passed-in vcmtpRecvv3 pointer.
 *
 * @param[in] *ptr        A pointer to the pre-defined data structure in the
 *                        caller. Here it's the pointer to a vcmtpRecvv3 class.
 */
void* vcmtpRecvv3::StartRetxHandler(void* ptr)
{
    vcmtpRecvv3* const recvr = static_cast<vcmtpRecvv3*>(ptr);
    try {
        recvr->retxHandler();
    }
    catch (const std::exception& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Starts the multicast-receiving task of a VCMTP receiver. Called by
 * `pthread_create()`.
 *
 * @param[in] arg   Pointer to the VCMTP receiver.
 * @retval    NULL  Always.
 */
void* vcmtpRecvv3::StartMcastHandler(
        void* const arg)
{
    vcmtpRecvv3* const recvr = static_cast<vcmtpRecvv3*>(arg);
    try {
        recvr->mcastHandler();
    }
    catch (const std::exception& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Start a Retx procedure, including the retxHandler thread and retxRequester
 * thread. These two threads will be started independently and after the
 * procedure returns, it continues to run the mcastHandler thread.
 *
 * @param[in] none
 */
void vcmtpRecvv3::StartRetxProcedure()
{
    int retval = pthread_create(&retx_t, NULL, &vcmtpRecvv3::StartRetxHandler,
                                this);
    if(retval != 0) {
        throw std::system_error(retval, std::system_category(),
            "vcmtpRecvv3::StartRetxProcedure() pthread_create() error");
    }
    pthread_detach(retx_t);

    retval = pthread_create(&retx_rq, NULL, &vcmtpRecvv3::StartRetxRequester,
                            this);
    if(retval != 0) {
        throw std::system_error(retval, std::system_category(),
            "vcmtpRecvv3::StartRetxProcedure() pthread_create() error");
    }
    pthread_detach(retx_rq);
}


/**
 * Starts a timer thread to watch for the case of missing EOP.
 *
 * @return  none
 */
void vcmtpRecvv3::startTimerThread()
{
    int retval = pthread_create(&timer_t, NULL, &vcmtpRecvv3::runTimerThread,
            this);

    if(retval != 0) {
        throw std::system_error(retval, std::system_category(),
            "vcmtpRecvv3::startTimerThread() pthread_create() error");
    }

    pthread_detach(timer_t);
}


/**
 * Sets the EOPStatus to true, which indicates the successful reception of EOP.
 *
 * @param[in] none
 */
void vcmtpRecvv3::setEOPReceived()
{
    std::unique_lock<std::mutex> lock(EOPStatMtx);
    EOPStatus = true;
}


/**
 * Runs a timer thread to watch for the case of missing EOP. If an expected
 * EOP is not received, the timer should trigger after sleeping. If it is
 * received from the mcast socket, do not trigger to request for retransmission
 * of the EOP. mcastEOPHandler will notify the condition variable to interrupt
 * the timed wait function.
 *
 * @param[in] none
 */
void vcmtpRecvv3::timerThread()
{
    while (1) {
        timerParam timerparam;
        {
            std::unique_lock<std::mutex> lock(timerQmtx);
            while (timerParamQ.empty())
                timerQfilled.wait(lock);
            timerparam = timerParamQ.front();
        }

        unsigned long period = timerparam.seconds * 1000000000lu;
        {
            std::unique_lock<std::mutex> lk(timerWakemtx);
            /** sleep for a given amount of time in precision of nanoseconds */
            timerWake.wait_for(lk, std::chrono::nanoseconds(period));
        }

        /** pop the current entry in timer queue when timer wakes up */
        {
            std::unique_lock<std::mutex> lock(timerQmtx);
            timerParamQ.pop();
        }

        /** if EOP has not been received yet, issue a request for retx */
        if (reqEOPifMiss(timerparam.prodindex)) {
            #ifdef DEBUG2
                std::string debugmsg = "Timer has waken up. Product #" +
                    std::to_string(timerparam.prodindex);
                debugmsg += " is still missing EOP. Reuquest retx";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
        }
    }
}


/**
 * Task terminator. If an exception is caught, this function will be called.
 * It consequently terminates all the other threads by calling the Stop(). This
 * task exit call will not be blocking.
 *
 * @param[in] e                     Exception status
 */
void vcmtpRecvv3::taskExit(const std::exception& e)
{
    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (!exceptIsSet) {
            except = e;
            exceptIsSet = true;
        }
    }
    Stop();
}


/**
 * Write a line of log record into the log file. If the log file doesn't exist,
 * create a new one and then append to it.
 *
 * @param[in] content       The content of the log record to be written.
 */
void vcmtpRecvv3::WriteToLog(const std::string& content)
{
    time_t rawtime;
    struct tm *timeinfo;
    char buf[30];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, 30, "%Y-%m-%d %I:%M:%S  ", timeinfo);
    std::string time(buf);

    /* code block only effective for measurement code */
    #ifdef MEASURE
    std::string hrclk;
    std::string nanosec;
    if (rxdone) {
        hrclk = std::to_string(end_t.time_since_epoch().count())
            + " since epoch, ";
    }
    else {
        hrclk = std::to_string(start_t.time_since_epoch().count())
            + " since epoch, ";
    }

    if (rxdone) {
        std::chrono::duration<double> timespan =
            std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
        nanosec = ", Elapsed time: " + std::to_string(timespan.count());
        nanosec += " seconds.";
    }
    #endif
    /* code block ends */

    std::ofstream logfile("VCMTPv3_RECEIVER.log",
            std::ofstream::out | std::ofstream::app);
    #ifdef MEASURE
        if (rxdone) {
            logfile << time << hrclk << content << nanosec << std::endl;
        }
        else {
            logfile << time << hrclk << content << std::endl;
        }
    #else
        logfile << time << content << std::endl;
    #endif
    logfile.close();
}
