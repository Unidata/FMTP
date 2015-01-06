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
    /** destructor should call Stop() to terminate all processes */
    Stop();
}


/**
 * Join given multicast group (defined by mcastAddr:mcastPort) to receive
 * multicasting products and start receiving thread to listen on the socket.
 * Doesn't return until `vcmtpRecvv3::Stop()` is called or an exception is
 * thrown.
 *
 * @param[in] none
 */
void vcmtpRecvv3::Start()
{
    joinGroup(mcastAddr, mcastPort);
    tcprecv = new TcpRecv(tcpAddr, tcpPort);
    StartRetxHandler();
    mcastHandler();
}


/**
 * Stop current running threads, close all sockets and release resources.
 *
 * @param[in] none
 */
void vcmtpRecvv3::Stop()
{
    // TODO: close all the sock_fd
    // call pthread_join()
    // make sure all resources are released
}


/**
 * Join multicast group specified by mcastAddr:mcastPort.
 *
 * @param[in] mcastAddr      Udp multicast address for receiving data products.
 * @param[in] mcastPort      Udp multicast port for receiving data products.
 * @throw std::system_error  {if the socket couldn't be created.}
 * @throw std::system_error  {if the socket couldn't be bound.}
 * @throw std::system_error  {if the socket couldn't join the multicast group.}
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
 * @throw std::system_error  {if an I/O error occurs.}
 */
void vcmtpRecvv3::mcastHandler()
{
    char pktBuf[MAX_VCMTP_PACKET_LEN];
    (void) memset(pktBuf, 0, sizeof(pktBuf));
    VcmtpHeader* header = (VcmtpHeader*) pktBuf;

    while(1)
    {
        if (recvfrom(mcastSock, pktBuf, MAX_VCMTP_PACKET_LEN, 0, NULL, NULL)
                < 0)
            throw std::system_error(errno, std::system_category(),
                    "vcmtpRecvv3::mcastHandler() recvfrom() error.");

        if (ntohs(header->flags) & VCMTP_BOP)
        {
            BOPHandler(pktBuf);
        }
        else if (ntohs(header->flags) & VCMTP_MEM_DATA)
        {
            recvMemData(pktBuf);
        }
        else if (ntohs(header->flags) & VCMTP_EOP)
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
 * @param[in] VcmtpPacket        Pointer to received vcmtp packet in buffer.
 */
void vcmtpRecvv3::BOPHandler(char* VcmtpPacket)
{
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    /** every time a new BOP arrives, save the header to check following data packets */
    memcpy(&vcmtpHeader.prodindex,  VcmtpPacketHeader,      4);
    memcpy(&vcmtpHeader.seqnum,     VcmtpPacketHeader+4,    4);
    memcpy(&vcmtpHeader.payloadlen, VcmtpPacketHeader+8,    2);
    memcpy(&vcmtpHeader.flags,      VcmtpPacketHeader+10,   2);

    /** every time a new BOP arrives, save the msg to check following data packets */
    memcpy(&BOPmsg.prodsize,  VcmtpPacketData,   4);
    memcpy(&BOPmsg.metasize,  (VcmtpPacketData+4), 2);
    BOPmsg.metasize = ntohs(BOPmsg.metasize);
    BOPmsg.metasize = BOPmsg.metasize > AVAIL_BOP_LEN ? AVAIL_BOP_LEN : BOPmsg.metasize;
    memcpy(BOPmsg.metadata,   VcmtpPacketData+6, BOPmsg.metasize);

    vcmtpHeader.prodindex  = ntohl(vcmtpHeader.prodindex);
    vcmtpHeader.seqnum     = 0;
    vcmtpHeader.payloadlen = 0;
    vcmtpHeader.flags      = ntohs(vcmtpHeader.flags);
    BOPmsg.prodsize        = ntohl(BOPmsg.prodsize);

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
 * @param[in] VcmtpPacket        Pointer to received vcmtp packet in buffer.
 */
void vcmtpRecvv3::recvMemData(char* VcmtpPacket)
{
    char*        VcmtpPacketHeader = VcmtpPacket;
    char*        VcmtpPacketData   = VcmtpPacket + VCMTP_HEADER_LEN;
    VcmtpHeader  tmpVcmtpHeader;

    memcpy(&tmpVcmtpHeader.prodindex,  VcmtpPacketHeader,      4);
    memcpy(&tmpVcmtpHeader.seqnum,     (VcmtpPacketHeader+4),  4);
    memcpy(&tmpVcmtpHeader.payloadlen, (VcmtpPacketHeader+8),  2);
    memcpy(&tmpVcmtpHeader.flags,      (VcmtpPacketHeader+10), 2);
    tmpVcmtpHeader.prodindex  = ntohl(tmpVcmtpHeader.prodindex);
    tmpVcmtpHeader.seqnum     = ntohl(tmpVcmtpHeader.seqnum);
    tmpVcmtpHeader.payloadlen = ntohs(tmpVcmtpHeader.payloadlen);
    tmpVcmtpHeader.flags      = ntohs(tmpVcmtpHeader.flags);

    /** check the packet sequence to detect missing packets */
    if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
            vcmtpHeader.seqnum + vcmtpHeader.payloadlen ==
            tmpVcmtpHeader.seqnum)
    {
        vcmtpHeader.seqnum     = tmpVcmtpHeader.seqnum;
        vcmtpHeader.payloadlen = tmpVcmtpHeader.payloadlen;
        vcmtpHeader.flags      = tmpVcmtpHeader.flags;
        if(prodptr)
            memcpy((char*)prodptr + vcmtpHeader.seqnum, VcmtpPacketData,
                   vcmtpHeader.payloadlen);
        else {
            std::cout << "seqnum: " << vcmtpHeader.seqnum;
            std::cout << "    paylen: " << vcmtpHeader.payloadlen << std::endl;
        }
    }
    /** missing BOP */
    else if(tmpVcmtpHeader.prodindex != vcmtpHeader.prodindex)
    {
        /** handle missing block */
    }
    /** data block out of order */
    else if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
            vcmtpHeader.seqnum + vcmtpHeader.payloadlen !=
            tmpVcmtpHeader.seqnum)
    {
        // TODO: drop duplicate blocks
        /** handle missing block */
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
