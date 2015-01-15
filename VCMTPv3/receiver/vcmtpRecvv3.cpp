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
    string                        tcpAddr,
    const unsigned short          tcpPort,
    string                        mcastAddr,
    const unsigned short          mcastPort,
    ReceivingApplicationNotifier* notifier)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    mcastAddr(mcastAddr),
    mcastPort(mcastPort),
    tcprecv(0),
    prodptr(0),
    notifier(notifier),
    mcastSock(0),
    retxSock(0)
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
    string                  tcpAddr,
    const unsigned short    tcpPort,
    string                  mcastAddr,
    const unsigned short    mcastPort)
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
    retxSock(0)
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
    pthread_cond_destroy(&msgQfilled);
    pthread_mutex_destroy(&msgQmutex);
    delete tcprecv;
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
    pthread_cond_init(&msgQfilled, NULL);
    pthread_mutex_init(&msgQmutex, NULL);
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
void vcmtpRecvv3::joinGroup(string mcastAddr, const unsigned short mcastPort)
{
    (void) memset(&mcastgroup, 0, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    mcastgroup.sin_addr.s_addr = inet_addr(mcastAddr.c_str());
    mcastgroup.sin_port = htons(mcastPort);
    if((mcastSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::system_error(errno, std::system_category(),
                "vcmtpRecvv3::joinGroup() creating socket failed.");
    if( bind(mcastSock, (struct sockaddr *) &mcastgroup, sizeof(mcastgroup))
            < 0 )
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


void vcmtpRecvv3::checkPayloadLen(const VcmtpHeader& header, const size_t nbytes)
{
    if (header.payloadlen != nbytes - VCMTP_HEADER_LEN)
        throw std::runtime_error("vcmtpRecvv3::decodeHeader(): Invalid payload length");
}


/**
 * @throw std::system_error   if an I/O error occurs.
 * @throw std::runtime_error  if a packet is invalid.
 */
void vcmtpRecvv3::mcastHandler()
{
    char pktBuf[MAX_VCMTP_PACKET_LEN];
    (void) memset(pktBuf, 0, sizeof(pktBuf));

    while(1)
    {
        VcmtpHeader header;
        char*       payload;
        ssize_t     nbytes = recv(mcastSock, pktBuf, MAX_VCMTP_PACKET_LEN, 0);

        if (nbytes < 0)
            throw std::system_error(errno, std::system_category(),
                    "vcmtpRecvv3::mcastHandler() recvfrom() error.");

        decodeHeader(pktBuf, nbytes, header, &payload);
        checkPayloadLen(header, nbytes);

        if (header.flags & VCMTP_BOP)
        {
            BOPHandler(header, payload);
        }
        else if (header.flags & VCMTP_MEM_DATA)
        {
            recvMemData(header, payload);
        }
        else if (header.flags & VCMTP_EOP)
            EOPHandler();
    }
}


void vcmtpRecvv3::retxRequester()
{
    while(1)
    {
        bool sendsuccess;
        pthread_mutex_lock(&msgQmutex);
        while (msgqueue.empty())
            pthread_cond_wait(&msgQfilled, &msgQmutex);
        INLReqMsg reqmsg;
        reqmsg = msgqueue.front();
        pthread_mutex_unlock(&msgQmutex);
        if (reqmsg.reqtype & MISSING_BOP)
        {
            sendsuccess = sendBOPRetxReq(reqmsg.prodindex);
        }
        else if (reqmsg.reqtype & MISSING_DATA)
        {
            sendsuccess = sendDataRetxReq(reqmsg.prodindex, reqmsg.seqnum,
                                          reqmsg.payloadlen);
        }
        pthread_mutex_lock(&msgQmutex);
        if (sendsuccess)
            msgqueue.pop();
        pthread_mutex_unlock(&msgQmutex);
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
        }
        else if (header.flags & VCMTP_RETX_DATA)
        {
            if(prodptr)
                tcprecv->recvData(NULL, 0, (char*)prodptr + header.seqnum,
                                  header.payloadlen);
                // TODO: handle recv failure.
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
    BOPmsg.metasize = ntohs(*(uint16_t*)VcmtpPacketData+4);
    BOPmsg.metasize = BOPmsg.metasize > AVAIL_BOP_LEN
                      ? AVAIL_BOP_LEN : BOPmsg.metasize;
    if (header.payloadlen - 6 < BOPmsg.metasize)
        throw std::runtime_error("vcmtpRecvv3::BOPHandler(): Metasize too big");
    (void)memcpy(BOPmsg.metadata, VcmtpPacketData+6, BOPmsg.metasize);

    /**
     * Every time a new BOP arrives, save the header to check following data
     * packets.
     */
    vcmtpHeader = header;
    vcmtpHeader.seqnum     = 0;
    vcmtpHeader.payloadlen = 0;

    #ifdef DEBUG
    std::cout << "(BOP) prodindex: " << vcmtpHeader.prodindex;
    std::cout << "    prodsize: " << BOPmsg.prodsize;
    std::cout << "    metasize: " << BOPmsg.metasize << std::endl;
    #endif

    if(notifier)
        notifier->notify_of_bop(BOPmsg.prodsize, BOPmsg.metadata,
                                BOPmsg.metasize, &prodptr);
}


/**
 * Parse data blocks, directly store and check for missing blocks.
 *
 * @param[in] header             The header associated with the packet.
 * @param[in] VcmtpPacketData    Pointer to payload of VCMTP packet.
 */
void vcmtpRecvv3::recvMemData(
        const VcmtpHeader& header,
        const char* const  VcmtpPacketData)
{
    /** missing BOP */
    if (header.prodindex != vcmtpHeader.prodindex)
    {
        /** do not need to care about the last 2 fields if it's BOP */
        INLReqMsg reqmsg = { MISSING_BOP, header.prodindex, 0, 0 };
        /** send a msg to the retx requester and signal it */
        pthread_mutex_lock(&msgQmutex);
        msgqueue.push(reqmsg);
        pthread_cond_signal(&msgQfilled);
        pthread_mutex_unlock(&msgQmutex);
    }
    /**
     * check the packet sequence to detect missing packets. If seqnum is
     * continuous, write the packet directly into product queue. Otherwise,
     * the packet is out of order.
     * */
    else if (vcmtpHeader.seqnum + vcmtpHeader.payloadlen == header.seqnum)
    {
        vcmtpHeader = header;
        if(prodptr)
            memcpy((char*)prodptr + vcmtpHeader.seqnum, VcmtpPacketData,
                   vcmtpHeader.payloadlen);
        else {
            std::cout << "seqnum: " << vcmtpHeader.seqnum;
            std::cout << "    paylen: " << vcmtpHeader.payloadlen << std::endl;
        }
    }
    /** data block out of order */
    else
    {
        uint32_t reqSeqnum = vcmtpHeader.seqnum + vcmtpHeader.payloadlen;
        // TODO: how to calculate payload length? what if 2 packets are missing?
        uint16_t reqPaylen = VCMTP_DATA_LEN;
        INLReqMsg reqmsg = { MISSING_DATA, header.prodindex, reqSeqnum,
                             reqPaylen };
        /** send a msg to the retx thread to request for data retransmission */
        pthread_mutex_lock(&msgQmutex);
        msgqueue.push(reqmsg);
        pthread_cond_signal(&msgQfilled);
        pthread_mutex_unlock(&msgQmutex);
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

    uint32_t prodindex = htonl(vcmtpHeader.prodindex);
    uint32_t seqNum    = htonl(0);
    uint16_t payLen    = htons(0);
    uint16_t flags     = htons(VCMTP_RETX_END);

    memcpy(pktBuf,    &prodindex, 4);
    memcpy(pktBuf+4,  &seqNum,    4);
    memcpy(pktBuf+8,  &payLen,    2);
    memcpy(pktBuf+10, &flags,     2);

    tcprecv->sendData(pktBuf, sizeof(pktBuf), NULL, 0);
}
