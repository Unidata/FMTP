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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief     Define the entity of VCMTPv3 receiver side method function.
 *
 * Receiver side of VCMTPv3 protocol. It handles incoming multicast packets
 * and issues retransmission requests to the sender side.
 */


#include "vcmtpRecvv3.h"

#include <arpa/inet.h>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <math.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <utility>
#include <fstream>
#include <iostream>
#include <system_error>

#define Frcv 20


/**
 * Constructs the receiver side instance (for integration with LDM).
 *
 * @param[in] tcpAddr       Tcp unicast address for retransmission.
 * @param[in] tcpPort       Tcp unicast port for retransmission.
 * @param[in] mcastAddr     Udp multicast address for receiving data products.
 * @param[in] mcastPort     Udp multicast port for receiving data products.
 * @param[in] notifier      Callback function to notify receiving application
 *                          of incoming Begin-Of-Product messages.
 * @param[in] ifAddr        IP of the interface to listen for multicast packets.
 */
vcmtpRecvv3::vcmtpRecvv3(
    const std::string    tcpAddr,
    const unsigned short tcpPort,
    const std::string    mcastAddr,
    const unsigned short mcastPort,
    RecvAppNotifier*     notifier,
    const std::string    ifAddr)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    mcastgroup(),
    mreq(),
    prodidx_mcast(0xFFFFFFFF),
    ifAddr(ifAddr),
    tcprecv(new TcpRecv(tcpAddr, tcpPort)),
    notifier(notifier),
    mcastSock(0),
    retxSock(0),
    pBlockMNG(new ProdBlockMNG()),
    msgQfilled(),
    msgQmutex(),
    BOPSetMtx(),
    exitMutex(),
    exitCond(),
    stopRequested(false),
    except(),
    retx_rq(),
    retx_t(),
    mcast_t(),
    timer_t(),
    linkspeed(0),
    retxHandlerCanceled(ATOMIC_FLAG_INIT),
    mcastHandlerCanceled(ATOMIC_FLAG_INIT),
    measure(new Measure())
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
    {
        std::unique_lock<std::mutex> lock(BOPSetMtx);
        misBOPset.clear();
    }
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        trackermap.clear();
    }
    delete tcprecv;
    delete pBlockMNG;
    delete measure;
}


/**
 * Gets the notified product index.
 *
 * @return    The product index of the latest completed product.
 */
uint32_t vcmtpRecvv3::getNotify()
{
    std::unique_lock<std::mutex> lock(notifyprodmtx);
    notify_cv.wait(lock);
    return notifyprodidx;
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
 * @throw std::runtime_error  If a retransmission-reception thread can't be
 *                            started.
 * @throw std::runtime_error  If a retransmission-request thread can't be
 *                            started.
 * @throw std::runtime_error  If a multicast-receiving thread couldn't be
 *                            created.
 * @throw std::runtime_error  If the multicast group couldn't be joined.
 * @throw std::runtime_error  If an I/O error occurs.
 */
void vcmtpRecvv3::Start()
{
    /** connect to the sender */
    tcprecv->Init();

    joinGroup(mcastAddr, mcastPort);

    StartRetxProcedure();
    startTimerThread();

    int status = pthread_create(&mcast_t, NULL, &vcmtpRecvv3::StartMcastHandler,
                                this);
    if (status) {
        Stop();
        throw std::runtime_error("vcmtpRecvv3::Start(): Couldn't start "
                "multicast-receiving thread, failed with status = "
                + std::to_string(status));
    }

    {
        std::unique_lock<std::mutex> lock(exitMutex);
        while (!stopRequested && !except)
            exitCond.wait(lock);
    }
    stopJoinRetxRequester();
    stopJoinRetxHandler();
    stopJoinTimerThread();
    stopJoinMcastHandler();

    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (except) {
            /**
             * The actual exception object that exception_ptr is pointing to
             * gets rethrown. There is no copying here, which means the
             * exception message will not be sliced by rethrow_exception().
             * Also, exception_ptr points to the actual exception by reference,
             * so there will be no copying caused by assigning exception_ptr.
             */
            std::rethrow_exception(except);
        }
    }
}


/**
 * Stops a running VCMTP receiver. Returns immediately. Idempotent.
 *
 * @pre  `vcmtpRecvv3::Start()` was previously called.
 */
void vcmtpRecvv3::Stop()
{
    {
        std::unique_lock<std::mutex> lock(exitMutex);
        stopRequested = true;
        exitCond.notify_one();
    }

    int prevState;

    /*
     * Prevent a thread on which `taskExit()` is executing from canceling itself
     * before it cancels others
     */
    (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &prevState);
    (void)pthread_setcancelstate(prevState, &prevState);
}


/**
 * Add the unrequested BOP identified by the given prodindex into the list.
 * If the BOP is already in the list, return with a false. If it's not, add
 * it into the list and return with a true.
 *
 * @param[in] prodindex        Product index of the missing BOP
 */
bool vcmtpRecvv3::addUnrqBOPinSet(uint32_t prodindex)
{
    std::pair<std::unordered_set<uint32_t>::iterator, bool> retval;
    int size = 0;
    {
        std::unique_lock<std::mutex> lock(BOPSetMtx);
        retval = misBOPset.insert(prodindex);
        size = misBOPset.size();
    }
    return retval.second;
}


/**
 * Handles a multicast BOP message given its peeked-at and decoded VCMTP header.
 *
 * @pre                       The multicast socket contains a VCMTP BOP packet.
 * @param[in] header          The associated, peeked-at and already-decoded
 *                            VCMTP header.
 * @throw std::runtime_error  if an error occurs while reading the socket.
 * @throw std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::mcastBOPHandler(const VcmtpHeader& header)
{
    #ifdef MODBASE
        uint32_t tmpidx = header.prodindex % MODBASE;
    #else
        uint32_t tmpidx = header.prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "[MCAST BOP] Product #" +
            std::to_string(tmpidx);
        debugmsg += ": BOP received from multicast.";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    char          pktBuf[MAX_VCMTP_PACKET_LEN];
    const ssize_t nbytes = recv(mcastSock, pktBuf, MAX_VCMTP_PACKET_LEN, 0);

    if (nbytes < 0) {
        throw std::runtime_error("vcmtpRecvv3::mcastBOPHandler() recv() got less"
                "than 0 bytes returned.");
    }

    /* records the most recent product index */
    prodidx_mcast = header.prodindex;

    checkPayloadLen(header, nbytes);
    BOPHandler(header, pktBuf + VCMTP_HEADER_LEN);
}


/**
 * Handles a retransmitted BOP message given its VCMTP header.
 *
 * @param[in] header           Header associated with the packet.
 * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
 */
void vcmtpRecvv3::retxBOPHandler(const VcmtpHeader& header,
                                 const char* const  VcmtpPacketData)
{
    #ifdef MODBASE
        uint32_t tmpidx = header.prodindex % MODBASE;
    #else
        uint32_t tmpidx = header.prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "[RETX BOP] Product #" +
            std::to_string(tmpidx);
        debugmsg += ": BOP received from unicast.";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    BOPHandler(header, VcmtpPacketData);
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
    uint32_t blknum = 0;
    void*    prodptr = NULL;
    BOPMsg   BOPmsg;
    /**
     * Every time a new BOP arrives, save the msg to check following data
     * packets
     */
    size_t BOPCONST = sizeof(BOPmsg.prodsize) + sizeof(BOPmsg.metasize);
    if (header.payloadlen < BOPCONST) {
        throw std::runtime_error("vcmtpRecvv3::BOPHandler(): packet too small");
    }
    const unsigned char* wire = (unsigned char*)VcmtpPacketData;
    BOPmsg.prodsize = ntohl(*(uint32_t*)wire);
    wire += sizeof(BOPmsg.prodsize);
    BOPmsg.metasize = ntohs(*(uint16_t*)wire);
    wire += sizeof(BOPmsg.metasize);
    BOPmsg.metasize = BOPmsg.metasize > AVAIL_BOP_LEN
                      ? AVAIL_BOP_LEN : BOPmsg.metasize;
    if ((header.payloadlen - BOPCONST) != BOPmsg.metasize) {
        throw std::runtime_error("vcmtpRecvv3::BOPHandler(): metasize "
                "mismatched payload indicated by header");
    }
    (void)memcpy(BOPmsg.metadata, wire, BOPmsg.metasize);

    if(notifier) {
        notifier->notify_of_bop(header.prodindex, BOPmsg.prodsize,
                BOPmsg.metadata, BOPmsg.metasize, &prodptr);
    }

    /* Atomic insertion for BOP of new product */
    {
        ProdTracker tracker = {BOPmsg.prodsize, prodptr, 0, 0};
        std::unique_lock<std::mutex> lock(trackermtx);
        trackermap[header.prodindex] = tracker;
    }

    blknum = BOPmsg.prodsize ? (BOPmsg.prodsize - 1) / VCMTP_DATA_LEN + 1 : 0;

    /** forcibly terminate the previous timer */
    timerWake.notify_all();

    /* check if the product is already under tracking */
    if (!pBlockMNG->addProd(header.prodindex, blknum)) {
        std::cout << "vcmtpRecvv3::BOPHandler(): duplicate BOP for product #"
            << header.prodindex << "received." << std::endl;
    }

    initEOPStatus(header.prodindex);

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
    double sleeptime = 0.0;
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        if (trackermap.count(header.prodindex)) {
            sleeptime = Frcv * ((double)trackermap[header.prodindex].prodsize /
                    (double)linkspeed);
        }
        else {
            throw std::runtime_error("vcmtpRecvv3::BOPHandler(): "
                    "Error accessing newly added BOP in trackermap.");
        }
    }
    /** add the new product into timer queue */
    {
        std::unique_lock<std::mutex> lock(timerQmtx);
        timerParam timerparam = {header.prodindex, sleeptime};
        timerParamQ.push(timerparam);
        timerQfilled.notify_all();
    }

    #ifdef MODBASE
        uint32_t tmpidx = header.prodindex % MODBASE;
    #else
        uint32_t tmpidx = header.prodindex;
    #endif

    #ifdef MEASURE
        {
            std::unique_lock<std::mutex> lock(trackermtx);
            if (trackermap.count(header.prodindex)) {
                ProdTracker tracker = trackermap[header.prodindex];
                measure->insert(header.prodindex, tracker.prodsize);
            }
            else {
                throw std::runtime_error("vcmtpRecvv3::BOPHandler(): "
                        "[Measure] Error accessing newly added BOP");
            }
        }
        std::string measuremsg = "[MEASURE] Product #" +
            std::to_string(tmpidx);
        measuremsg += ": BOP is received. Product size = ";
        measuremsg += std::to_string(BOPmsg.prodsize);
        measuremsg += ", Metadata size = ";
        measuremsg += std::to_string(BOPmsg.metasize);
        std::cout << measuremsg << std::endl;
        WriteToLog(measuremsg);
    #endif
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
    if (header.payloadlen != nbytes - VCMTP_HEADER_LEN) {
        throw std::runtime_error("vcmtpRecvv3::checkPayloadLen(): "
                "Invalid payload length");
    }
}


/**
 * Clears the EOP arrival status.
 *
 * @param[in] prodindex    Product index which the EOP belongs to.
 */
void vcmtpRecvv3::clearEOPStatus(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(EOPmapmtx);
    EOPmap.erase(prodindex);
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
 * translation.
 *
 * @param[in]  packet         The raw packet.
 * @param[out] header         The decoded packet header.
 */
void vcmtpRecvv3::decodeHeader(char* const  packet, VcmtpHeader& header)
{
    unsigned char* wire = (unsigned char*)packet;
    header.prodindex  = ntohl(*(uint32_t*)wire);
    wire += sizeof(header.prodindex);
    header.seqnum     = ntohl(*(uint32_t*)wire);
    wire += sizeof(header.seqnum);
    header.payloadlen = ntohs(*(uint16_t*)wire);
    wire += sizeof(header.payloadlen);
    header.flags      = ntohs(*(uint16_t*)wire);
}


/**
 * Handles a received EOP from the unicast thread. Check the bitmap to see if
 * all the data blocks are received. If true, notify the RecvApp. If false,
 * request for retransmission if it has to be so.
 *
 * @param[in] header           Reference to the received VCMTP packet header.
 * @throws std::out_of_range   The notifier doesn't know about
 *                             `header.prodindex`.
 * @throws std::runtime_error  Receiving application error.
 */
void vcmtpRecvv3::EOPHandler(const VcmtpHeader& header)
{
    /**
     * if bitmap check tells everything is completed, then sends the
     * RETX_END message back to sender. Meanwhile notify receiving
     * application.
     */
    if (pBlockMNG->delIfComplete(header.prodindex)) {
        sendRetxEnd(header.prodindex);
        if (notifier) {
            notifier->notify_of_eop(header.prodindex);
        }
        else {
            /**
             * Updates the most recently acknowledged product and notifies
             * a dummy notification handler (getNotify()).
             */
            {
                std::unique_lock<std::mutex> lock(notifyprodmtx);
                notifyprodidx = header.prodindex;
            }
            notify_cv.notify_one();
        }

        {
            std::unique_lock<std::mutex> lock(trackermtx);
            trackermap.erase(header.prodindex);
        }

        #ifdef MODBASE
            uint32_t tmpidx = header.prodindex % MODBASE;
        #else
            uint32_t tmpidx = header.prodindex;
        #endif

        #ifdef DEBUG2
            std::string debugmsg = "[MSG] Product #" +
                std::to_string(tmpidx);
            debugmsg += " has been completely received";
            std::cout << debugmsg << std::endl;
            WriteToLog(debugmsg);
        #elif DEBUG1
            std::string debugmsg = "[MSG] Product #" +
                std::to_string(tmpidx);
            debugmsg += " has been completely received";
            std::cout << debugmsg << std::endl;
        #endif

        #ifdef MEASURE
            uint32_t bytes = measure->getsize(header.prodindex);
            std::string measuremsg = "[SUCCESS] Product #" +
                std::to_string(tmpidx);
            measuremsg += ": product received, size = ";
            measuremsg += std::to_string(bytes);
            measuremsg += " bytes, elapsed time = ";
            measuremsg += measure->gettime(header.prodindex);
            measuremsg += " seconds.";
            if (measure->getEOPmiss(header.prodindex)) {
                measuremsg += " EOP is retransmitted";
            }
            std::cout << measuremsg << std::endl;
            WriteToLog(measuremsg);
            /* remove the measurement if completely received */
            measure->remove(header.prodindex);
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
        if (!hasLastBlock(header.prodindex)) {
            uint32_t prodsize;
            bool     haveProdsize;
            {
                std::unique_lock<std::mutex> lock(trackermtx);
                haveProdsize = trackermap.count(header.prodindex) > 0;
                if (haveProdsize)
                    prodsize = trackermap[header.prodindex].prodsize;
            }
            if (haveProdsize)
                requestAnyMissingData(header.prodindex, prodsize);
        }
    }
}


/**
 * Gets the EOP arrival status.
 *
 * @param[in] prodindex    Product index which the EOP belongs to.
 */
bool vcmtpRecvv3::getEOPStatus(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(EOPmapmtx);
    return EOPmap[prodindex];
}


/**
 * Check if the last data block has been received.
 *
 * @param[in] none
 */
bool vcmtpRecvv3::hasLastBlock(const uint32_t prodindex)
{
    return pBlockMNG->getLastBlock(prodindex);
}


/**
 * Initialize the EOP arrival status.
 *
 * @param[in] prodindex    Product index which the EOP belongs to.
 */
void vcmtpRecvv3::initEOPStatus(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(EOPmapmtx);
    EOPmap[prodindex] = false;
}


/**
 * Join multicast group specified by mcastAddr:mcastPort.
 *
 * @param[in] mcastAddr      Udp multicast address for receiving data products.
 * @param[in] mcastPort      Udp multicast port for receiving data products.
 * @throw std::runtime_error if the socket couldn't be created.
 * @throw std::runtime_error if the socket couldn't be bound.
 * @throw std::runtime_error if the socket couldn't join the multicast group.
 */
void vcmtpRecvv3::joinGroup(
        std::string          mcastAddr,
        const unsigned short mcastPort)
{
    (void) memset(&mcastgroup, 0, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    // mcastgroup.sin_addr.s_addr = htonl(INADDR_ANY);
    mcastgroup.sin_port = htons(mcastPort);
    mcastgroup.sin_addr.s_addr = inet_addr(mcastAddr.c_str());
    if((mcastSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        throw std::runtime_error("vcmtpRecvv3::joinGroup() creating socket failed");
    }
    if (::bind(mcastSock, (struct sockaddr *) &mcastgroup, sizeof(mcastgroup))
            < 0) {
        throw std::runtime_error("vcmtprecvv3::joingroup(): couldn't bind socket "
                " to multicast group " + mcastgroup);
    }
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr.c_str());
    mreq.imr_interface.s_addr = inet_addr(ifAddr.c_str());
    if( setsockopt(mcastSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(mreq)) < 0 ) {
        throw std::runtime_error("vcmtpRecvv3::joinGroup() setsockopt() add "
                "membership failed.");
    }
}


/**
 * Handles multicast packets. To avoid extra copying operations, here recv()
 * is called with a MSG_PEEK flag to only peek the header instead of reading
 * it out (which would cause the buffer to be wiped). And the recv() call
 * will block if there is no data coming to the mcastSock.
 *
 * @throw std::runtime_error   if an I/O error occurs.
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

        if (nbytes < 0) {
            throw std::runtime_error("vcmtpRecvv3::mcastHandler() recv() less "
                    "than zero bytes.");
        }
        if (nbytes != sizeof(header)) {
            throw std::runtime_error("vcmtpRecvv3::mcastHandler() Invalid packet "
                    "length.");
        }

        decodeHeader(header);

        if (header.flags == VCMTP_BOP) {
            mcastBOPHandler(header);
        }
        else if (header.flags == VCMTP_MEM_DATA) {
            #ifdef MEASURE
                measure->setMcastClock(header.prodindex);
            #endif

            recvMemData(header);
        }
        else if (header.flags == VCMTP_EOP) {
            #ifdef MEASURE
                measure->setMcastClock(header.prodindex);
            #endif

            mcastEOPHandler(header);
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
    uint32_t      lastprodidx = 0xFFFFFFFF;
    /** read the EOP packet out in order to remove it from buffer */
    const ssize_t nbytes = recv(mcastSock, pktBuf, VCMTP_HEADER_LEN, 0);

    if (nbytes < 0) {
        throw std::runtime_error("vcmtpRecvv3::mcastEOPHandler() recv() less than "
                "zero bytes.");
    }

    #ifdef MODBASE
        uint32_t tmpidx = header.prodindex % MODBASE;
    #else
        uint32_t tmpidx = header.prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "[MCAST EOP] Product #" +
            std::to_string(tmpidx);
        debugmsg += ": EOP is received";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    bool hasBOP = false;
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        if (trackermap.count(header.prodindex)) {
            hasBOP = true;
        }
    }
    if (hasBOP) {
        setEOPStatus(header.prodindex);
        timerWake.notify_all();
        EOPHandler(header);
    }
    else {
        int state = requestMissingBops(header.prodindex);
        /**
         * prodidx_mcast is only updated if no corresponding BOP is found.
         * Because if we assume packets arriving in sequence, then the index
         * would not change upon EOP arrival. On the other hand, if there do
         * exist out-of-sequence packets, updating the prodidx_mcast could
         * decrease the value. When following packets arrive, the receiver
         * may think a BOP is missed. But if no BOP is found, this EOP is the
         * first packet received with the index, the prodidx_mcast has to be
         * updated to avoid duplicate BOP request since retxBOPHandler does
         * not update prodidx_mcast. Whether to update lastprodidx after
         * requestMissingBops() returns depends on the return value. A return
         * value of 2 suggests discarding the out-of-sequence packet.
         */
        if (state == 1) {
            prodidx_mcast = header.prodindex;
        }
    }
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
 * @throws std::out_of_range   The notifier doesn't know about
 *                             `header.prodindex`.
 * @throws std::runtime_error  Receiving application error.
 */
void vcmtpRecvv3::retxHandler()
{
    VcmtpHeader header;
    char        pktHead[VCMTP_HEADER_LEN];
    char        paytmp[VCMTP_DATA_LEN];
    int         initState;
    int         ignoredState;

    (void)memset(pktHead, 0, sizeof(pktHead));
    (void)memset(paytmp, 0, sizeof(paytmp));
    /*
     * Allow the current thread to be cancelled only when it is likely blocked
     * attempting to read from the unicast socket because that prevents the
     * receiver from being put into an inconsistent state yet allows for fast
     * termination.
     */
    (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &initState);

    while(1)
    {
        /* a useless place holder for header parsing */
        char* pholder;

        (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ignoredState);
        size_t nbytes = tcprecv->recvData(pktHead, VCMTP_HEADER_LEN, NULL, 0);
        (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ignoredState);
        /*
         * recvData returning 0 indicates an unexpected socket close, thus
         * VCMTP receiver should stop right away and return.
         * Since TcpRecv::recvData() either returns 0 or VCMTP_HEADER_LEN here,
         * decodeHeader() should only be called if nbytes is not 0. Besides,
         * decodeHeader() itself does not do any header size check, because
         * nbytes should always equal VCMTP_HEADER_LEN when successful.
         */
        if (nbytes == 0) {
            Stop();
        }
        else {
            /* TcpRecv::recvData() will return requested number of bytes */
            decodeHeader(pktHead, header);
        }

        if (header.flags == VCMTP_RETX_BOP) {
            (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ignoredState);
            nbytes = tcprecv->recvData(NULL, 0, paytmp, header.payloadlen);
            (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &ignoredState);

            if (nbytes == 0) {
                Stop();
            }
            else {
                retxBOPHandler(header, paytmp);
            }

            /** remove the BOP from missing list */
            (void)rmMisBOPinSet(header.prodindex);

            uint32_t prodsize    = 0;
            uint32_t seqnum      = 0;
            uint32_t lastprodidx = 0xFFFFFFFF;
            {
                std::unique_lock<std::mutex> lock(antiracemtx);
                {
                    std::unique_lock<std::mutex> lock(trackermtx);
                    if (trackermap.count(header.prodindex)) {
                        ProdTracker tracker  = trackermap[header.prodindex];
                        prodsize             = tracker.prodsize;
                        seqnum               = tracker.seqnum;

                        lastprodidx = prodidx_mcast;
                    }
                }
                if (prodsize > 0) {
                    /**
                     * If seqnum != 0, the seqnum is updated right after the
                     * retx BOP is handled, which means the multicast thread
                     * is receiving blocks. In this case, nothing needs to be
                     * done.
                     */
                    if (seqnum == 0) {
                        /**
                         * If two indices don't equal, the product is totally
                         * missed. Thus, all blocks should be requested.
                         * On the other hand, if they equal, there could be
                         * concurrency or a gap before next product arrives.
                         * Only requesting EOP is the most economic choice.
                         */
                        if (lastprodidx != header.prodindex) {
                            requestAnyMissingData(header.prodindex, prodsize);
                        }
                        pushMissingEopReq(header.prodindex);
                    }
                }
                else {
                    throw std::runtime_error("vcmtpRecvv3::retxHandler() "
                            "Product not found in BOPMap after receiving retx BOP");
                }
            }
        }
        else if (header.flags == VCMTP_RETX_DATA) {
            #ifdef MEASURE
                /* log the time first */
                measure->setRetxClock(header.prodindex);
            #endif

            #ifdef MODBASE
                uint32_t tmpidx = header.prodindex % MODBASE;
            #else
                uint32_t tmpidx = header.prodindex;
            #endif

            #ifdef DEBUG2
                std::string debugmsg = "[RETX DATA] Product #" +
                    std::to_string(tmpidx);
                debugmsg += ": Data block received on unicast, SeqNum = ";
                debugmsg += std::to_string(header.seqnum);
                debugmsg += ", Paylen = ";
                debugmsg += std::to_string(header.payloadlen);
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif

            /**
             * directly writing unwanted data to NULL is not allowed. So here
             * uses a temp buffer as trash to dump the payload content.
             */
            char     tmp[VCMTP_DATA_LEN];
            uint32_t prodsize = 0;
            void*    prodptr  = NULL;
            {
                std::unique_lock<std::mutex> lock(trackermtx);
                if (trackermap.count(header.prodindex)) {
                    ProdTracker tracker = trackermap[header.prodindex];
                    prodsize = tracker.prodsize;
                    prodptr  = tracker.prodptr;
                }
            }

            if ((prodsize > 0) &&
                (header.seqnum + header.payloadlen > prodsize)) {
                throw std::runtime_error("vcmtpRecvv3::retxHandler() "
                        "retx block out of boundary: seqnum=" +
                        std::to_string(header.seqnum) + ", payloadlen=" +
                        std::to_string(header.payloadlen) + "prodsize=" +
                        std::to_string(prodsize));
            }
            else if (prodsize <= 0) {
                (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,
                                             &ignoredState);
                /**
                 * drop the payload since there is no location allocated
                 * in the product queue.
                 */
                tcprecv->recvData(NULL, 0, tmp, header.payloadlen);
                (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                             &ignoredState);

                /*
                 * The TrackerMap will only be erased when the associated
                 * product has been completely received. So if no valid
                 * prodindex found, it indicates the product is received
                 * and thus removed or there is out-of-order arrival on
                 * TCP.
                 */
                continue;
            }

            if(prodptr) {
                (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,
                                             &ignoredState);
                tcprecv->recvData(NULL, 0, (char*)prodptr + header.seqnum,
                                  header.payloadlen);
                (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                             &ignoredState);
            }
            else {
                /** dump the payload since there is no product queue */
                (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,
                                             &ignoredState);
                tcprecv->recvData(NULL, 0, tmp, header.payloadlen);
                (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                             &ignoredState);
            }

            uint32_t iBlock = header.seqnum/VCMTP_DATA_LEN;
            pBlockMNG->set(header.prodindex, iBlock);

            if (pBlockMNG->delIfComplete(header.prodindex)) {
                sendRetxEnd(header.prodindex);
                if (notifier) {
                    notifier->notify_of_eop(header.prodindex);
                }
                else {
                    /**
                     * Updates the most recently acknowledged product and notifies
                     * a dummy notification handler (getNotify()).
                     */
                    {
                        std::unique_lock<std::mutex> lock(notifyprodmtx);
                        notifyprodidx = header.prodindex;
                    }
                    notify_cv.notify_one();
                }

                {
                    std::unique_lock<std::mutex> lock(trackermtx);
                    trackermap.erase(header.prodindex);
                }

                #ifdef DEBUG2
                    std::string debugmsg = "[MSG] Product #" +
                        std::to_string(tmpidx);
                    debugmsg += " has been completely received";
                    std::cout << debugmsg << std::endl;
                    WriteToLog(debugmsg);
                #elif DEBUG1
                    std::string debugmsg = "[MSG] Product #" +
                        std::to_string(tmpidx);
                    debugmsg += " has been completely received";
                    std::cout << debugmsg << std::endl;
                #endif

                #ifdef MEASURE
                    uint32_t bytes = measure->getsize(header.prodindex);
                    std::string measuremsg = "[SUCCESS] Product #" +
                        std::to_string(tmpidx);
                    measuremsg += ": product received, size = ";
                    measuremsg += std::to_string(bytes);
                    measuremsg += " bytes, elapsed time = ";
                    measuremsg += measure->gettime(header.prodindex);
                    measuremsg += " seconds.";
                    if (measure->getEOPmiss(header.prodindex)) {
                        measuremsg += " EOP is retransmitted";
                    }
                    std::cout << measuremsg << std::endl;
                    WriteToLog(measuremsg);
                    /* remove the measurement if completely received */
                    measure->remove(header.prodindex);
                #endif
            }
        }
        else if (header.flags == VCMTP_RETX_EOP) {
            #ifdef MEASURE
                measure->setRetxClock(header.prodindex);
                measure->setEOPmiss(header.prodindex);
            #endif

            retxEOPHandler(header);
        }
        else if (header.flags == VCMTP_RETX_REJ) {
            (void)rmMisBOPinSet(header.prodindex);
            /*
             * if associated bitmap exists, remove the bitmap. Also avoid
             * duplicated notification if the product's bitmap has
             * already been removed.
             */
            if (!pBlockMNG->isComplete(header.prodindex) &&
                pBlockMNG->rmProd(header.prodindex)) {
                #ifdef MODBASE
                    uint32_t tmpidx = header.prodindex % MODBASE;
                #else
                    uint32_t tmpidx = header.prodindex;
                #endif

                #ifdef DEBUG2
                    std::string debugmsg = "[FAILURE] Product #" +
                        std::to_string(tmpidx);
                    debugmsg += " is not completely received";
                    std::cout << debugmsg << std::endl;
                    WriteToLog(debugmsg);
                #endif

                if (notifier) {
                    notifier->notify_of_missed_prod(header.prodindex);
                }
                else {
                    /**
                     * Updates the most recently acknowledged product and notifies
                     * a dummy notification handler (getNotify()).
                     */
                    {
                        std::unique_lock<std::mutex> lock(notifyprodmtx);
                        notifyprodidx = header.prodindex;
                    }
                    notify_cv.notify_one();
                }
            }
        }
    }

    (void)pthread_setcancelstate(initState, &ignoredState);
}


/**
 * Fetch the requests from an internal message queue and call corresponding
 * handler to send requests respectively. The read operation on the internal
 * message queue will block if the queue is empty itself. The existing request
 * being handled will only be removed from the queue if the handler returns a
 * successful state. Doesn't return until a "shutdown" request is encountered or
 * an error occurs.
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

        if (reqmsg.reqtype == SHUTDOWN)
            break; // leave "shutdown" message in queue

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
bool vcmtpRecvv3::rmMisBOPinSet(uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(BOPSetMtx);
    bool rmsuccess = misBOPset.erase(prodindex);
    int size = misBOPset.size();
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
    #ifdef MODBASE
        uint32_t tmpidx = header.prodindex % MODBASE;
    #else
        uint32_t tmpidx = header.prodindex;
    #endif

    #ifdef DEBUG2
        std::string debugmsg = "[RETX EOP] Product #" +
            std::to_string(tmpidx);
        debugmsg += ": EOP is received";
        std::cout << debugmsg << std::endl;
        WriteToLog(debugmsg);
    #endif

    EOPHandler(header);
}


/**
 * Reads the data portion of a VCMTP data-packet into the location specified
 * by the receiving application given the associated, peeked-at, and decoded
 * VCMTP header.
 *
 * @pre                       The socket contains a VCMTP data-packet.
 * @param[in] header          The associated, peeked-at, and decoded header.
 * @throw std::runtime_error  if an error occurs while reading the multicast
 *                            socket.
 * @throw std::runtime_error  if the packet is invalid.
 */
void vcmtpRecvv3::readMcastData(const VcmtpHeader& header)
{
    ssize_t nbytes = 0;
    void*   prodptr = NULL;
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        if (trackermap.count(header.prodindex)) {
            ProdTracker tracker = trackermap[header.prodindex];
            prodptr = tracker.prodptr;
        }
    }

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
        throw std::runtime_error("vcmtpRecvv3::readMcastData(): readv() EOF.");
    }
    else {
        checkPayloadLen(header, nbytes);

        #ifdef MODBASE
            uint32_t tmpidx = header.prodindex % MODBASE;
        #else
            uint32_t tmpidx = header.prodindex;
        #endif

        #ifdef DEBUG2
            std::string debugmsg = "[MCAST DATA] Product #" +
                std::to_string(tmpidx);
            debugmsg += ": Data block received from multicast. SeqNum = ";
            debugmsg += std::to_string(header.seqnum);
            debugmsg += ", Paylen = ";
            debugmsg += std::to_string(header.payloadlen);
            std::cout << debugmsg << std::endl;
            WriteToLog(debugmsg);
        #endif

        /** receiver should trust the packet from sender is legal */
        pBlockMNG->set(header.prodindex, header.seqnum/VCMTP_DATA_LEN);
    }
}


/**
 * Requests data-packets that lie between the last previously-received
 * data-packet of the current data-product and its most recently-received
 * data-packet.
 *
 * @pre                 The most recently-received data-packet is for the
 *                      current data-product.
 * @param[in] prodindex Product index.
 * @param[in] seqnum    The most recently-received data-packet of the current
 *                      data-product.
 */
void vcmtpRecvv3::requestAnyMissingData(const uint32_t prodindex,
                                        const uint32_t mostRecent)
{
    uint32_t seqnum = 0;
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        if (trackermap.count(prodindex)) {
            ProdTracker tracker = trackermap[prodindex];
            seqnum = tracker.seqnum + tracker.paylen;
        }
    }

    /**
     * requests for missing blocks counting from the last received
     * block sequence number.
     */
    if (seqnum != mostRecent) {
        seqnum = (seqnum / VCMTP_DATA_LEN) * VCMTP_DATA_LEN;

        std::unique_lock<std::mutex> lock(msgQmutex);

        for (; seqnum < mostRecent; seqnum += VCMTP_DATA_LEN) {
            pushMissingDataReq(prodindex, seqnum, VCMTP_DATA_LEN);

            #ifdef MODBASE
                uint32_t tmpidx = prodindex % MODBASE;
            #else
                uint32_t tmpidx = prodindex;
            #endif

            #ifdef DEBUG2
                std::string debugmsg = "[RETX REQ] Product #" +
                    std::to_string(tmpidx);
                debugmsg += ": Data block is missing. SeqNum = ";
                debugmsg += std::to_string(seqnum);
                debugmsg += ". Request retx.";
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
 * @return               1 means everything is okay. 2 means out-of-sequence
 *                       packet is received.
 */
int vcmtpRecvv3::requestMissingBops(const uint32_t prodindex)
{
    uint32_t lastprodidx = 0xFFFFFFFF;
    /* fetches the most recent product index */
    lastprodidx = prodidx_mcast;

    /**
     * When out-of-sequence packet is detected, there could be several possible
     * cases. (1) DATA1-DATA2-EOP1. Here EOP1 goes out of sequence somehow. If
     * BOP1 is received, then it does not matter since mcastEOPHandler() will
     * handle it. But if BOP1 is also missing, then both 1 and 2 do not have
     * BOPs. This causes requestMissingBops() to be called but with lastprodidx
     * already updated to 2. Because lastprodidx is only updated by mcastHandler
     * thread, its value should increase in sequence, identical to the order
     * that mcastHandler is seeing packets. Also, lastprodidx is only updated
     * after corresponding operation. Then this prodindex < lastprodidx case
     * implies whatever needs to be done is done. Thus, upon detecting this
     * case, it directly returns but not to throw any exception in order to
     * the thread running.
     */
    if (prodindex < lastprodidx) {
        #ifdef DEBUG2
            std::string debugmsg = "[ERR PKT SEQ] vcmtpRecvv3::"
                "requestMissingBops() out of sequence packet received, current"
                " lastprodidx = " + std::to_string(lastprodidx) +
                ", passed in prodindex = " + std::to_string(prodindex);
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
        #endif
        return 2;
    }

    for (uint32_t i = (lastprodidx + 1); i != (prodindex + 1); i++) {
        if (addUnrqBOPinSet(i)) {
            pushMissingBopReq(i);
        }
    }

    return 1;
}


/**
 * Handles a multicast VCMTP data-packet given the associated peeked-at and
 * decoded VCMTP header. Directly store and check for missing blocks.
 *
 * @pre                       The socket contains a VCMTP data-packet.
 * @param[in] header          The associated, peeked-at and decoded header.
 * @throw std::runtime_error  if `seqnum + payloadlen` is out of boundary.
 * @throw std::runtime_error  if the packet is invalid.
 * @throw std::runtime_error   if an error occurs while reading the socket.
 */
void vcmtpRecvv3::recvMemData(const VcmtpHeader& header)
{
    int state = 0;
    uint32_t prodsize = 0;
    {
        std::unique_lock<std::mutex> lock(trackermtx);
        if (trackermap.count(header.prodindex)) {
            ProdTracker tracker = trackermap[header.prodindex];
            prodsize = tracker.prodsize;
        }
    }

    if ((prodsize > 0) && (header.seqnum + header.payloadlen > prodsize)) {
        throw std::runtime_error(
            std::string("vcmtpRecvv3::recvMemData() block out of boundary: ") +
            "seqnum=" + std::to_string(header.seqnum) + ", payloadlen=" +
            std::to_string(header.payloadlen) + ", prodsize=" +
            std::to_string(prodsize));
    }

    /**
     * If prodsize > 0, the BOP of the currently receiving product is
     * received, otherwise, it is either removed or not even received.
     * Since this function is called by multicast thread, it is likely
     * to be the first time a product arrives. So BOP loss is the only
     * possibility.
     */
    if (prodsize > 0) {
        readMcastData(header);
        {
            std::unique_lock<std::mutex> lock(antiracemtx);
            requestAnyMissingData(header.prodindex, header.seqnum);
            /* update most recent seqnum and payloadlen */
            {
                std::unique_lock<std::mutex> lock(trackermtx);
                if (trackermap.count(header.prodindex)) {
                    trackermap[header.prodindex].seqnum = header.seqnum;
                    trackermap[header.prodindex].paylen = header.payloadlen;
                }
            }
        }
    }
    else {
        char buf[1];
        (void)recv(mcastSock, buf, 1, 0); // skip unusable datagram
        state = requestMissingBops(header.prodindex);
    }

    /* records the most recent product index */
    if (state == 1) {
        prodidx_mcast = header.prodindex;
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
    if (!getEOPStatus(prodindex)) {
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
    catch (std::runtime_error& e) {
        recvr->taskExit(e);
    }
    return NULL;
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
 * Start the retxRequester thread using a passed-in vcmtpRecvv3 pointer. Called
 * by `pthread_create()`.
 *
 * @param[in] *ptr        A pointer to the pre-defined data structure in the
 *                        caller. Here it's the pointer to a vcmtpRecvv3
 *                        instance.
 */
void* vcmtpRecvv3::StartRetxRequester(void* ptr)
{
    vcmtpRecvv3* const recvr = static_cast<vcmtpRecvv3*>(ptr);
    try {
        recvr->retxRequester();
    }
    catch (std::runtime_error& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Stops the retransmission-request task by adding a "shutdown" request to the
 * associated queue and joins with its thread.
 *
 * @throws std::runtime_error If the retransmission-request thread can't be
 *                            joined.
 */
void vcmtpRecvv3::stopJoinRetxRequester()
{
    {
        std::unique_lock<std::mutex> lock(msgQmutex);
        INLReqMsg reqmsg = {SHUTDOWN};
        msgqueue.push(reqmsg);
        msgQfilled.notify_one();
    }

    int status = pthread_join(retx_rq, NULL);
    if (status) {
        throw std::runtime_error("vcmtpRecvv3::stopJoinRetxRequester() Couldn't "
                "join retransmission-request thread");
    }
}


/**
 * Start the retxHandler thread using a passed-in vcmtpRecvv3 pointer. Called
 * by `pthread_create()`.
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
    catch (std::runtime_error& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Stops the retransmission-reception task by canceling its thread and joining
 * it.
 *
 * @throws std::runtime_error If the retransmission-reception thread can't be
 *                            canceled.
 * @throws std::runtime_error If the retransmission-reception thread can't be
 *                            joined.
 */
void vcmtpRecvv3::stopJoinRetxHandler()
{
    if (!retxHandlerCanceled.test_and_set()) {
        int status = pthread_cancel(retx_t);
        if (status && status != ESRCH) {
            throw std::runtime_error("vcmtpRecvv3::stopJoinMcastHandler() "
                    "Couldn't cancel retransmission-reception thread");
        }
        status = pthread_join(retx_t, NULL);
        if (status && status != ESRCH) {
            throw std::runtime_error("vcmtpRecvv3::stopJoinRetxHandler() "
                    "Couldn't join retransmission-reception thread");
        }
    }
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
    catch (std::runtime_error& e) {
        recvr->taskExit(e);
    }
    return NULL;
}


/**
 * Stops the muticast task by canceling its thread and joining it.
 *
 * @throws std::runtime_error if the multicast thread can't be canceled.
 * @throws std::runtime_error if the multicast thread can't be joined.
 */
void vcmtpRecvv3::stopJoinMcastHandler()
{
    if (!mcastHandlerCanceled.test_and_set()) {
        int status = pthread_cancel(mcast_t);
        if (status && status != ESRCH) {
            throw std::runtime_error("vcmtpRecvv3::stopJoinMcastHandler() "
                    "Couldn't cancel multicast thread");
        }
        status = pthread_join(mcast_t, NULL);
        if (status && status != ESRCH) {
            throw std::runtime_error("vcmtpRecvv3::stopJoinMcastHandler() "
                    "Couldn't join multicast thread");
        }
    }
}


/**
 * Start a Retx procedure, including the retxHandler thread and retxRequester
 * thread. These two threads will be started independently and after the
 * procedure returns, it continues to run the mcastHandler thread.
 *
 * @throws    std::runtime_error If a retransmission-reception thread can't be
 *                               started.
 * @throws    std::runtime_error If a retransmission-request thread can't be
 *                               started.
 */
void vcmtpRecvv3::StartRetxProcedure()
{
    int retval = pthread_create(&retx_t, NULL, &vcmtpRecvv3::StartRetxHandler,
                                this);
    if(retval != 0) {
        throw std::runtime_error(
            "vcmtpRecvv3::StartRetxProcedure() pthread_create() error");
    }

    retval = pthread_create(&retx_rq, NULL, &vcmtpRecvv3::StartRetxRequester,
                            this);
    if(retval != 0) {
        try {
            stopJoinRetxHandler();
        }
        catch (...) {}
        throw std::runtime_error("vcmtpRecvv3::StartRetxProcedure() "
                "pthread_create() failed with retval = "
                + std::to_string(retval));
    }
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
        throw std::runtime_error("vcmtpRecvv3::startTimerThread() "
                "pthread_create() failed with retval = " + std::to_string(retval));
    }
}


/**
 * Stops the timer task by adding a "shutdown" entry to the associated queue and
 * joins with its thread.
 *
 * @throws std::runtime_error if the timer thread can't be joined.
 */
void vcmtpRecvv3::stopJoinTimerThread()
{
    timerParam timerparam;
    {
        std::unique_lock<std::mutex> lock(timerQmtx);
        timerParam timerparam = {0, -1.0};
        timerParamQ.push(timerparam);
        timerQfilled.notify_one();
    }

    int status = pthread_join(timer_t, NULL);
    if (status) {
        throw std::runtime_error("vcmtpRecvv3::stopJoinTimerThread() "
                "Couldn't join timer thread");
    }
}


/**
 * Sets the EOP arrival status to true, which indicates the successful
 * reception of EOP.
 *
 * @param[in] prodindex    Product index which the EOP belongs to.
 */
void vcmtpRecvv3::setEOPStatus(const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(EOPmapmtx);
    EOPmap[prodindex] = true;
}


/**
 * Runs a timer thread to watch for the case of missing EOP. If an expected
 * EOP is not received, the timer should trigger after sleeping. If it is
 * received from the mcast socket, do not trigger to request for retransmission
 * of the EOP. mcastEOPHandler will notify the condition variable to interrupt
 * the timed wait function. Doesn't return unless a "shutdown" entry is
 * encountered or an exception is thrown.
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

        if (timerparam.seconds < 0)
            break; // leave "shutdown" entry in queue

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
            #ifdef MODBASE
                uint32_t tmpidx = timerparam.prodindex % MODBASE;
            #else
                uint32_t tmpidx = timerparam.prodindex;
            #endif

            #ifdef DEBUG2
            std::string debugmsg = "[TIMER] Timer has waken up. Product #" +
                    std::to_string(tmpidx);
                debugmsg += " is still missing EOP. Request retx.";
                std::cout << debugmsg << std::endl;
                WriteToLog(debugmsg);
            #endif
        }
        /**
         * After waking up, the timer checks the EOP arrival status of
         * a product and decides whether to request for re-transmission.
         * Only the timer can clear the EOPmap.
         */
        clearEOPStatus(timerparam.prodindex);
    }
}


/**
 * Task terminator. If an exception is thrown by an independent task executing
 * on an independent thread, then this function will be called. It consequently
 * terminates all the other tasks. Blocks on the exit mutex; otherwise, returns
 * immediately.
 *
 * @param[in] eptr                  Exception pointer
 */
void vcmtpRecvv3::taskExit(const std::runtime_error& e)
{
    {
        std::unique_lock<std::mutex> lock(exitMutex);
        if (!except) {
            /**
             * make_exception_ptr() will make a copy of the exception of the
             * declared type, which is std::runtime_error, instead of what it is
             * actually. Since the passed-in exception can be any derived
             * exception type, this would cause slicing and losing information.
             */
            except = std::make_exception_ptr(e);
        }
        exitCond.notify_one();
    }
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
    /* create logs  directory if it doesn't exist */
    struct stat st = {0};
    if (stat("logs", &st) == -1) {
        mkdir("logs", 0755);
    }
    /* allocate a large enough buffer in case some long hostnames */
    char hostname[1024];
    gethostname(hostname, 1024);
    std::string logpath(hostname);
    logpath = "logs/VCMTPv3_RECEIVER_" + logpath + ".log";

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, 30, "%Y-%m-%d %I:%M:%S  ", timeinfo);
    std::string time(buf);

    std::ofstream logfile(logpath, std::ofstream::out | std::ofstream::app);
    logfile << time << content << std::endl;
    logfile.close();
}
