/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecvv3v3.cpp
 *
 * @history:
 *      Created on : Oct 17, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
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
    notifier(0)
{
    max_sock_fd = 0;
    mcast_sock = 0;
    retx_tcp_sock = 0;
    recv_thread = 0;
    retx_thread = 0;
}

vcmtpRecvv3::~vcmtpRecvv3()
{
}

void vcmtpRecvv3::Start()
{
    joinGroup(mcastAddr, mcastPort);
    StartReceivingThread();
}

void vcmtpRecvv3::Stop()
{
    // close all the sock_fd
    // call pthread_join()
    // make sure all resources are released
}

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
    /*
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if( setsockopt(mcast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0 )
        perror("vcmtpRecvv3::joinGroup() setsockopt failed.");
    */
    FD_SET(mcast_sock, &read_sock_set);
}

void vcmtpRecvv3::StartReceivingThread()
{
    pthread_create(&recv_thread, NULL, &vcmtpRecvv3::StartReceivingThread, this);
}

void* vcmtpRecvv3::StartReceivingThread(void* ptr)
{
    ((vcmtpRecvv3*)ptr)->RunReceivingThread();
    return NULL;
}

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

void vcmtpRecvv3::BOPHandler(char* VcmtpPacket)
{
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    // every time a new BOP arrives, save the header to check following data packets
    memcpy(&vcmtpHeader.prodindex,  VcmtpPacketHeader,      4);
    memcpy(&vcmtpHeader.seqnum,     VcmtpPacketHeader+4,    4);
    memcpy(&vcmtpHeader.payloadlen, VcmtpPacketHeader+8,    2);
    memcpy(&vcmtpHeader.flags,      VcmtpPacketHeader+10,   2);

    // every time a new BOP arrives, save the msg to check following data packets
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
    std::cout << "(VCMTP Header) prodindex: " << vcmtpHeader.prodindex << std::endl;
    std::cout << "(VCMTP Header) Seq Num: " << vcmtpHeader.seqnum << std::endl;
    std::cout << "(VCMTP Header) payloadLen: " << vcmtpHeader.payloadlen << std::endl;
    std::cout << "(VCMTP Header) flags: " << vcmtpHeader.flags << std::endl;
    std::cout << "(BOP) prodsize: " << BOPmsg.prodsize << std::endl;
    std::cout << "(BOP) metasize: " << BOPmsg.metasize << std::endl;
    std::cout << "(BOP) metadata: " << BOPmsg.metadata << std::endl;
    #endif

    if (notifier)
        notifier->notify_of_bop(BOPmsg.prodsize, BOPmsg.metadata,
                               BOPmsg.metasize, &prodptr);
}

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

    // check for the first mem data packet
    if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
       tmpVcmtpHeader.seqnum == 0)
    {
    #ifdef DEBUG
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
    // check for the packet sequence to detect missing packets
    else if(tmpVcmtpHeader.prodindex == vcmtpHeader.prodindex &&
            vcmtpHeader.seqnum + vcmtpHeader.payloadlen == tmpVcmtpHeader.seqnum)
    {
    #ifdef DEBUG
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
}

void vcmtpRecvv3::EOPHandler()
{
    std::cout << "Mem data completely received." << std::endl;
}
