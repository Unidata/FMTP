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
 * @brief     Define the entity of VCMTPv3 method function.
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
    notifier(notifier)
{
    max_sock_fd = 0;
    mcast_sock = 0;
    retx_tcp_sock = 0;
    recv_thread = 0;
    retx_thread = 0;
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
    notifier(0)    /*!< constructor called by independent test program will
                    set notifier to NULL */
{
    max_sock_fd = 0;
    mcast_sock = 0;
    retx_tcp_sock = 0;
    recv_thread = 0;
    retx_thread = 0;
}


/**
 * Destructs the receiver side instance.
 *
 * @param[in] none
 */
vcmtpRecvv3::~vcmtpRecvv3()
{
    /** destructor should call Stop() to terminate all processes */
}


/**
 * Join given multicast group (defined by mcastAddr:mcastPort) to receive
 * multicasting products and start receiving thread to listen on the socket.
 *
 * @param[in] none
 */
void vcmtpRecvv3::Start()
{
    joinGroup(mcastAddr, mcastPort);
    StartReceivingThread();
}


/**
 * Stop current running threads, close all sockets and release resources.
 *
 * @param[in] none
 */
void vcmtpRecvv3::Stop()
{
    // close all the sock_fd
    // call pthread_join()
    // make sure all resources are released
}


/**
 * Join multicast group specified by mcastAddr:mcastPort.
 *
 * @param[in] mcastAddr     Udp multicast address for receiving data products.
 * @param[in] mcastPort     Udp multicast port for receiving data products.
 */
void vcmtpRecvv3::joinGroup(string mcastAddr, const unsigned short mcastPort)
{
    bzero(&mcastgroup, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    mcastgroup.sin_addr.s_addr = inet_addr(mcastAddr.c_str());
    mcastgroup.sin_port = htons(mcastPort);
    if( (mcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("vcmtpRecvv3::joinGroup() creating socket failed.");
    if( bind(mcast_sock, (struct sockaddr *) &mcastgroup, sizeof(mcastgroup)) < 0 )
        perror("vcmtpRecvv3::joinGroup() binding socket failed.");
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if( setsockopt(mcast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0 )
        perror("vcmtpRecvv3::joinGroup() setsockopt failed.");
    FD_SET(mcast_sock, &read_sock_set);
}


/**
 * Create a new thread to run StartReceivingThread().
 *
 * @param[in] none
 */
void vcmtpRecvv3::StartReceivingThread()
{
    pthread_create(&recv_thread, NULL, &vcmtpRecvv3::StartReceivingThread, this);
}


/**
 * Run receiving thread in a newly created thread.
 *
 * @param[in] ptr        A pointer points to this thread itself.
 */
void* vcmtpRecvv3::StartReceivingThread(void* ptr)
{
    ((vcmtpRecvv3*)ptr)->RunReceivingThread();
    return NULL;
}


/**
 * Listen on both multicast thread and unicast thread to handle receiving and
 * retransmission.
 *
 * @param[in] none
 */
void vcmtpRecvv3::RunReceivingThread()
{
    fd_set  read_set;
    while(true) {
        read_set = read_sock_set;
        /*
        if (select(max_sock_fd + 1, &read_set, NULL, NULL, NULL) == -1) {
            //SysError("TcpServer::SelectReceive::select() error");
            break;
        }
        */

        // tests to see if multicast_sock is part of the set
        if (FD_ISSET(mcast_sock, &read_set)) {
            mcastMonitor();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retx_tcp_sock, &read_set)) {
        }
    }
    pthread_exit(0);
}


/**
 * Listen on both multicast thread and unicast thread to handle receiving and
 * retransmission. Check flag field and can corresponding handler.
 *
 * @param[in] none
 */
void vcmtpRecvv3::mcastMonitor()
{
    static char packet_buffer[MAX_VCMTP_PACKET_LEN];
    bzero(packet_buffer, sizeof(packet_buffer));
    VcmtpHeader* header = (VcmtpHeader*) packet_buffer;

    if ( recvfrom(mcast_sock, packet_buffer, MAX_VCMTP_PACKET_LEN, 0, NULL,
         NULL) < 0 )
        perror("vcmtpRecvv3::HandleMulticastPacket() recv error");
    if ( ntohs(header->flags) & VCMTP_BOP ) {
        BOPHandler(packet_buffer);
    }
    else if ( ntohs(header->flags) & VCMTP_MEM_DATA ) {
        recvMemData(packet_buffer);
    }
    else if ( ntohs(header->flags) & VCMTP_EOP ) {
        EOPHandler();
    }
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
    vcmtpHeader.seqnum     = ntohl(vcmtpHeader.seqnum);
    vcmtpHeader.payloadlen = ntohs(vcmtpHeader.payloadlen);
    vcmtpHeader.flags      = ntohs(vcmtpHeader.flags);
    BOPmsg.prodsize        = ntohl(BOPmsg.prodsize);

    #ifdef DEBUG
    std::cout << "(BOP) prodindex: " << vcmtpHeader.prodindex;
    std::cout << "    prodsize: " << BOPmsg.prodsize;
    std::cout << "    metasize: " << BOPmsg.metasize << std::endl;
    #endif

    if (notifier)
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
    char*        VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;
    VcmtpHeader  tmpVcmtpHeader;

    memcpy(&tmpVcmtpHeader.prodindex,  VcmtpPacketHeader,      4);
    memcpy(&tmpVcmtpHeader.seqnum,     (VcmtpPacketHeader+4),  4);
    memcpy(&tmpVcmtpHeader.payloadlen, (VcmtpPacketHeader+8),  2);
    memcpy(&tmpVcmtpHeader.flags,      (VcmtpPacketHeader+10), 2);
    tmpVcmtpHeader.prodindex  = ntohl(tmpVcmtpHeader.prodindex);
    tmpVcmtpHeader.seqnum     = ntohl(tmpVcmtpHeader.seqnum);
    tmpVcmtpHeader.payloadlen = ntohs(tmpVcmtpHeader.payloadlen);
    tmpVcmtpHeader.flags      = ntohs(tmpVcmtpHeader.flags);

    /** check for the first mem data packet */
    if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
       tmpVcmtpHeader.seqnum == 0)
    {
    #ifdef DEBUG2
        uint8_t testvar1;
        uint8_t testvar2;
        uint8_t testvar3;
        uint8_t testvar4;
        memcpy(&testvar1, VcmtpPacketData, 1);
        memcpy(&testvar2, VcmtpPacketData+1, 1);
        memcpy(&testvar3, VcmtpPacketData+2, 1);
        memcpy(&testvar4, VcmtpPacketData+3, 1);
        printf("%x ", testvar1);
        printf("%x ", testvar2);
        printf("%x ", testvar3);
        printf("%x ", testvar4);
        printf("...\n");
    #endif

        vcmtpHeader.seqnum     = tmpVcmtpHeader.seqnum;
        vcmtpHeader.payloadlen = tmpVcmtpHeader.payloadlen;
        vcmtpHeader.flags      = tmpVcmtpHeader.flags;
    }
    /** check for the packet sequence to detect missing packets */
    else if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
            vcmtpHeader.seqnum + vcmtpHeader.payloadlen == tmpVcmtpHeader.seqnum)
    {
    #ifdef DEBUG2
        uint8_t testvar1;
        uint8_t testvar2;
        uint8_t testvar3;
        uint8_t testvar4;
        memcpy(&testvar1, VcmtpPacketData, 1);
        memcpy(&testvar2, VcmtpPacketData+1, 1);
        memcpy(&testvar3, VcmtpPacketData+2, 1);
        memcpy(&testvar4, VcmtpPacketData+3, 1);
        printf("%x ", testvar1);
        printf("%x ", testvar2);
        printf("%x ", testvar3);
        printf("%x ", testvar4);
        printf("...\n");
    #endif

        vcmtpHeader.seqnum     = tmpVcmtpHeader.seqnum;
        vcmtpHeader.payloadlen = tmpVcmtpHeader.payloadlen;
    }
    else {
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
    std::cout << "(EOP) data-product completely received." << std::endl;
}
