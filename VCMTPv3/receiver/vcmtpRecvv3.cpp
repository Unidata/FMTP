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
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <memory.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <system_error>
#include <unistd.h>
#include <sys/uio.h>

using namespace std;

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
    string               tcpAddr,
    const unsigned short tcpPort,
    string               mcastAddr,
    const unsigned short mcastPort,
    RecvAppNotifier*     notifier)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    tcprecv(0),
    prodptr(0),
    notifier(notifier),
    mcastSock(0),
    retxSock(0),
    msgQfilled(),
    msgQmutex(),
    BOPListMutex(),
    bitmap(0)
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
    string               tcpAddr,
    const unsigned short tcpPort,
    string               mcastAddr,
    const unsigned short mcastPort)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    tcprecv(0),
    prodptr(0),
    notifier(0),   /*!< constructor called by independent test program will
                    set notifier to NULL */
    mcastSock(0),
    retxSock(0),
    msgQfilled(),
    msgQmutex(),
    BOPListMutex(),
    bitmap(0)
{
}


/**
 * Destructs the receiver side instance.
 *
 * @param[in] none
 */
vcmtpRecvv3::~vcmtpRecvv3()
{
    // TODO: close all the sock_fd
    // make sure all resources are released
    delete tcprecv;
    // TODO: clear the misBOPlist
    delete bitmap;
}


/**
 * Join given multicast group (defined by mcastAddr:mcastPort) to receive
 * multicasting products and start receiving thread to listen on the socket.
 * Doesn't return until `vcmtpRecvv3::~vcmtpRecvv3()` or `vcmtpRecvv3::Stop()`
 * is called or an exception is thrown.
 *
 * @throw std::system_error  if the multicast group couldn't be joined.
 * @throw std::system_error  if an I/O error occurs.
 */
void vcmtpRecvv3::Start()
{
    joinGroup(mcastAddr, mcastPort);
    tcprecv = new TcpRecv(tcpAddr, tcpPort);
    StartRetxProcedure();
    mcastHandler();
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
        string               mcastAddr,
        const unsigned short mcastPort)
{
    (void) memset(&mcastgroup, 0, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    mcastgroup.sin_addr.s_addr = inet_addr(mcastAddr.c_str());
    mcastgroup.sin_port = htons(mcastPort);
    if((mcastSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() creating socket failed.");
    if (::bind(mcastSock, (struct sockaddr *) &mcastgroup, sizeof(mcastgroup))
            < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() binding socket failed.");
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if( setsockopt(mcastSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(mreq)) < 0 )
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() setsockopt() failed.");
}


void vcmtpRecvv3::StartRetxProcedure()
{
    pthread_t retx_t, retx_rq;
    pthread_create(&retx_t, NULL, &vcmtpRecvv3::StartRetxHandler, this);
    pthread_detach(retx_t);
    pthread_create(&retx_rq, NULL, &vcmtpRecvv3::StartRetxRequester, this);
    pthread_detach(retx_rq);
}


void* vcmtpRecvv3::StartRetxHandler(void* ptr)
{
    (static_cast<vcmtpRecvv3*>(ptr))->retxHandler();
    return NULL;
}


void* vcmtpRecvv3::StartRetxRequester(void* ptr)
{
    (static_cast<vcmtpRecvv3*>(ptr))->retxRequester();
    return NULL;
}


/**
 * Decodes the header of a VCMTP packet in-place.
 *
 * @param[in,out] header  The VCMTP header to be decoded.
 */
void vcmtpRecvv3::decodeHeader(
        VcmtpHeader& header)
{
    header.prodindex  = ntohl(header.prodindex);
    header.seqnum     = ntohl(header.seqnum);
    header.payloadlen = ntohs(header.payloadlen);
    header.flags      = ntohs(header.flags);
}


/**
 * Decodes a VCMTP packet header.
 *
 * @param[in]  packet         The raw packet.
 * @param[in]  nbytes         The size of the raw packet in bytes.
 * @param[out] header         The decoded packet header.
 * @param[out] payload        Payload of the packet.
 * @throw std::runtime_error  if the packet is too small.
 */
void vcmtpRecvv3::decodeHeader(
        char* const  packet,
        const size_t nbytes,
        VcmtpHeader& header,
        char** const payload)
{
    if (nbytes < VCMTP_HEADER_LEN)
        throw std::runtime_error("vcmtpRecvv3::decodeHeader(): Packet is too small");

    header.prodindex  = ntohl(*(uint32_t*)packet);
    header.seqnum     = ntohl(*(uint32_t*)packet+4);
    header.payloadlen = ntohs(*(uint16_t*)packet+8);
    header.flags      = ntohs(*(uint16_t*)packet+10);

    *payload = packet + VCMTP_HEADER_LEN;
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
 * Handles multicast packets.
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

        if (nbytes < 0)
            throw std::system_error(errno, std::system_category(),
                    "vcmtpRecvv3::mcastHandler() recv() error.");
        if (nbytes != sizeof(header))
            throw std::runtime_error("Invalid packet length");

        decodeHeader(header);

        if (header.flags & VCMTP_BOP) {
            BOPHandler(header);
        }
        else if (header.flags & VCMTP_MEM_DATA) {
            recvMemData(header);
        }
        else if (header.flags & VCMTP_EOP) {
            EOPHandler();
        }
    }
}


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

        if (((reqmsg.reqtype & MISSING_BOP) &&
                sendBOPRetxReq(reqmsg.prodindex)) ||
            ((reqmsg.reqtype & MISSING_DATA) &&
                    sendDataRetxReq(reqmsg.prodindex, reqmsg.seqnum,
                            reqmsg.payloadlen))) {
            std::unique_lock<std::mutex> lock(msgQmutex);
            msgqueue.pop();
        }
    }
}


void vcmtpRecvv3::retxHandler()
{
    char pktHead[VCMTP_HEADER_LEN];
    (void) memset(pktHead, 0, sizeof(pktHead));
    VcmtpHeader header;

    while(1)
    {
        /** temp buffer, do not access in case of out of bound issues */
        char* paytmp;
        ssize_t nbytes = recv(retxSock, pktHead, VCMTP_HEADER_LEN, 0);

        decodeHeader(pktHead, nbytes, header, &paytmp);

        if (header.flags & VCMTP_BOP)
        {
            tcprecv->recvData(NULL, 0, paytmp, header.payloadlen);
            BOPHandler(header, paytmp);

            /** remove the BOP from missing list */
            (void)rmMisBOPinList(header.prodindex);
        }
        else if (header.flags & VCMTP_RETX_DATA)
        {
            if(prodptr)
                tcprecv->recvData(NULL, 0, (char*)prodptr + header.seqnum,
                                  header.payloadlen);

                if (bitmap && bitmap->isComplete()) {
                    notifier->notify_of_eop();
                }
        }
    }
}


/**
 * Parse BOP message and call notifier to notify receiving application.
 *
 * @param[in] header           Header associated with the packet.
 * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
 * @throw std::runtime_error   if the payload is too small.
 * @throw std::runtime_error   if the amount of metadata is invalid.
 */
void vcmtpRecvv3::BOPHandler(
        const VcmtpHeader& header,
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
        unique_lock<std::mutex> lock(vcmtpHeaderMutex);
        vcmtpHeader = header;
        vcmtpHeader.seqnum     = 0;
        vcmtpHeader.payloadlen = 0;
    }

    #ifdef DEBUG
    std::cout << "(BOP) prodindex: " << vcmtpHeader.prodindex;
    std::cout << "    prodsize: " << BOPmsg.prodsize;
    std::cout << "    metasize: " << BOPmsg.metasize << std::endl;
    #endif

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
}


bool vcmtpRecvv3::rmMisBOPinList(uint32_t prodindex)
{
    bool rmsuccess;
    list<uint32_t>::iterator it;
    unique_lock<std::mutex>  lock(BOPListMutex);

    for(it=misBOPlist.begin(); it!=misBOPlist.end(); ++it)
    {
        if (*it == prodindex)
        {
            misBOPlist.erase(it);
            rmsuccess = true;
            break;
        }
        else
            rmsuccess = false;
    }
    return rmsuccess;
}


bool vcmtpRecvv3::addUnrqBOPinList(uint32_t prodindex)
{
    bool addsuccess;
    list<uint32_t>::iterator it;
    unique_lock<std::mutex>  lock(BOPListMutex);
    for(it=misBOPlist.begin(); it!=misBOPlist.end(); ++it)
    {
        if (*it == prodindex)
        {
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
void vcmtpRecvv3::BOPHandler(
        const VcmtpHeader& header)
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
void vcmtpRecvv3::readMcastData(
        const VcmtpHeader& header)
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
            // TODO: throw exception here
            std::cout << "seqnum: " << header.seqnum;
            std::cout << "    paylen: " << header.payloadlen << std::endl;
        }
        else {
            /** receiver should trust the packet from sender is legal */
            bitmap->set(header.seqnum/VCMTP_DATA_LEN);
        }
    }
}


/**
 * Pushes a request for a BOP-packet onto the retransmission-request queue.
 *
 * @param[in] prodindex  Index of the associated data-product.
 */
void vcmtpRecvv3::pushMissingBopReq(
        const uint32_t prodindex)
{
    std::unique_lock<std::mutex> lock(msgQmutex);
    INLReqMsg                    reqmsg = {MISSING_BOP, prodindex, 0, 0};
    msgqueue.push(reqmsg);
    msgQfilled.notify_one();
}


/**
 * Pushes a request for a data-packet onto the retransmission-request queue.
 *
 * @pre                  The retransmission-request queue is locked.
 * @param[in] prodindex  Index of the associated data-product.
 * @param[in] seqnum     Sequence number of the data-packet.
 * @param[in] datalen    Amount of data in bytes.
 */
void vcmtpRecvv3::pushMissingDataReq(
        const uint32_t prodindex,
        const uint32_t seqnum,
        const uint16_t datalen)
{
    INLReqMsg reqmsg = {MISSING_DATA, prodindex, seqnum, datalen};
    msgqueue.push(reqmsg);
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
void vcmtpRecvv3::requestAnyMissingData(
        const uint32_t mostRecent)
{
    unique_lock<std::mutex> lock(vcmtpHeaderMutex);
    uint32_t seqnum = vcmtpHeader.seqnum + vcmtpHeader.payloadlen;
    uint32_t prodindex = vcmtpHeader.prodindex;
    lock.unlock();

    if (seqnum != mostRecent) {
        seqnum = (seqnum / VCMTP_DATA_LEN) * VCMTP_DATA_LEN;
        /*
         * The data-packet associated with the VCMTP header is out-of-order.
         */
        std::unique_lock<std::mutex> lock(msgQmutex);

        for (; seqnum < mostRecent; seqnum += VCMTP_DATA_LEN)
            pushMissingDataReq(prodindex, seqnum, VCMTP_DATA_LEN);

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
void vcmtpRecvv3::requestMissingBops(
        const uint32_t prodindex)
{
    // Careful! Product-indexes wrap around!
    unique_lock<std::mutex> lock(vcmtpHeaderMutex);
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
void vcmtpRecvv3::recvMemData(
        const VcmtpHeader& header)
{
    unique_lock<std::mutex> lock(vcmtpHeaderMutex);
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
 * Report a successful received EOP.
 *
 * @param[in] none
 */
void vcmtpRecvv3::EOPHandler()
{
    // TODO: better do a integrity check of the received data blocks.
    std::cout << "(EOP) data-product completely received." << std::endl;
    // notify EOP
    if (bitmap) {
        if (bitmap->isComplete())
            notifier->notify_of_eop();
    }
    else
        throw std::runtime_error("vcmtpRecvv3::EOPHandler() has no valid bitmap");
}


bool vcmtpRecvv3::sendBOPRetxReq(uint32_t prodindex)
{
    VcmtpHeader header;
    header.prodindex  = htonl(prodindex);
    header.seqnum     = 0;
    header.payloadlen = 0;
    header.flags      = htons(VCMTP_BOP_REQ);

    return (-1 != tcprecv->sendData(&header, sizeof(VcmtpHeader), NULL, 0));
}


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


void vcmtpRecvv3::sendRetxEnd()
{
    char pktBuf[VCMTP_HEADER_LEN];
    (void) memset(pktBuf, 0, sizeof(pktBuf));

    unique_lock<std::mutex> lock(vcmtpHeaderMutex);
    uint32_t prodindex = htonl(vcmtpHeader.prodindex);
    lock.unlock();
    uint32_t seqNum    = htonl(0);
    uint16_t payLen    = htons(0);
    uint16_t flags     = htons(VCMTP_RETX_END);

    memcpy(pktBuf,    &prodindex, 4);
    memcpy(pktBuf+4,  &seqNum,    4);
    memcpy(pktBuf+8,  &payLen,    2);
    memcpy(pktBuf+10, &flags,     2);

    tcprecv->sendData(pktBuf, sizeof(pktBuf), NULL, 0);
}
