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
    StartRetxHandler();
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


void vcmtpRecvv3::StartRetxHandler()
{
    pthread_t retx_t;
    pthread_create(&retx_t, NULL, &vcmtpRecvv3::StartRetxHandler, this);
    pthread_detach(retx_t);
}


void* vcmtpRecvv3::StartRetxHandler(void* ptr)
{
    (static_cast<vcmtpRecvv3*>(ptr))->retxHandler();
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
 * @throw std::runtime_error  if the packet has in invalid payload length.
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

    if (header.payloadlen != nbytes - VCMTP_HEADER_LEN)
        throw std::runtime_error("vcmtpRecvv3::decodeHeader(): Invalid payload length");

    *payload = packet + VCMTP_HEADER_LEN;
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
        ssize_t     nbytes = recvfrom(mcastSock, pktBuf, MAX_VCMTP_PACKET_LEN,
                                      0, NULL, NULL);

        if (nbytes < 0)
            throw std::system_error(errno, std::system_category(),
                    "vcmtpRecvv3::mcastHandler() recvfrom() error.");

        decodeHeader(pktBuf, nbytes, header, &payload);

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


void vcmtpRecvv3::retxHandler()
{
    /*
    sleep(5);
    std::cout << "retxHandler wakes up" << std::endl;
    */
    sendRetxEnd();
    sleep(3);
    sendRetxReq();
    sleep(1);
    recvRetxData();
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
        /** send a msg to the retx thread to request for BOP retransmission */
        msgqueue.push(reqmsg);
    }
    /** check the packet sequence to detect missing packets */
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
        // TODO: drop duplicate blocks
        uint32_t reqSeqnum = vcmtpHeader.seqnum + vcmtpHeader.payloadlen;
        // TODO: how to calculate payload length?
        uint16_t reqPaylen = VCMTP_DATA_LEN;
        INLReqMsg reqmsg = { MISSING_DATA, header.prodindex, reqSeqnum,
                             reqPaylen };
        /** send a msg to the retx thread to request for data retransmission */
        msgqueue.push(reqmsg);
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


void vcmtpRecvv3::sendRetxReq()
{
    char pktBuf[VCMTP_HEADER_LEN];
    (void) memset(pktBuf, 0, sizeof(pktBuf));

    //uint32_t prodindex = htonl(vcmtpHeader.prodindex);
    uint32_t prodindex = htonl(0);
    uint32_t seqNum    = htonl(1448);
    uint16_t payLen    = htons(1448);
    uint16_t flags     = htons(VCMTP_RETX_REQ);

    memcpy(pktBuf,    &prodindex, 4);
    memcpy(pktBuf+4,  &seqNum,    4);
    memcpy(pktBuf+8,  &payLen,    2);
    memcpy(pktBuf+10, &flags,     2);

    tcprecv->sendData(pktBuf, sizeof(pktBuf), NULL, 0);
}


void vcmtpRecvv3::recvRetxData()
{
	VcmtpHeader tmpheader;
    char pktBuf[MAX_VCMTP_PACKET_LEN];
    (void) memset(pktBuf, 0, sizeof(pktBuf));
    char* pktbufhead = pktBuf;
    char* pktbufpay = pktBuf + VCMTP_HEADER_LEN;

    tcprecv->recvData(pktbufhead, VCMTP_HEADER_LEN, pktbufpay, VCMTP_DATA_LEN);
    memcpy(&tmpheader.prodindex,  pktbufhead,    4);
    memcpy(&tmpheader.seqnum, 	   pktbufhead+4,  4);
    memcpy(&tmpheader.payloadlen, pktbufhead+8,  2);
    memcpy(&tmpheader.flags,      pktbufhead+10, 2);

    tmpheader.prodindex  = ntohl(tmpheader.prodindex);
    tmpheader.seqnum     = ntohl(tmpheader.seqnum);
    tmpheader.payloadlen = ntohs(tmpheader.payloadlen);
    tmpheader.flags      = ntohs(tmpheader.flags);
    cout << "(Retx) prodindex: " << tmpheader.prodindex << endl;
    cout << "(Retx) seqnum: " << tmpheader.seqnum << endl;
    cout << "(Retx) payloadlen: " << tmpheader.payloadlen << endl;
    cout << "(Retx) flags: " << tmpheader.flags << endl;

#ifdef DEBUG
    char c0, c1;
    memcpy(&c0, pktbufpay, 1);
    memcpy(&c1, pktbufpay+1, 1);
    printf("%X ", c0);
    printf("%X", c1);
    printf("\n");
#endif
}
