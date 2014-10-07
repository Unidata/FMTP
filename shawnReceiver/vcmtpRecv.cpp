/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpReceiver.cpp
 *
 * @history:
 *      Created on : Oct 2, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include "vcmtpRecv.h"
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <endian.h>
#include <strings.h>
#include <memory.h>
#include <pthread.h>


vcmtpRecv::vcmtpRecv(
    string                  tcpAddr,
    const unsigned short    tcpPort,
    string                  localAddr,
    const unsigned short    localPort)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort),
    localAddr(localAddr),
    localPort(localPort)
{
    max_sock_fd = 0;
    mcast_sock = 0;
    retx_tcp_sock = 0;
    recv_thread = 0;
    retx_thread = 0;
}

vcmtpRecv::~vcmtpRecv()
{
}

void vcmtpRecv::Start()
{
    udpBindIP2Sock(localAddr, localPort);
    StartReceivingThread();
}

int vcmtpRecv::udpBindIP2Sock(string localAddr, const unsigned short localPort)
{
    bzero(&sender, sizeof(sender));
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = inet_addr(localAddr.c_str());
    sender.sin_port = htons(localPort);
    if( (mcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("udpBindIP2Sock() creating socket failed.");
    if( bind(mcast_sock, (sockaddr*)&sender, sizeof(sender)) < 0 )
        perror("udpBindIP2Sock() binding socket failed.");
    FD_SET(mcast_sock, &read_sock_set);
    return 0;
}

void vcmtpRecv::StartReceivingThread()
{
    pthread_create(&recv_thread, NULL, &vcmtpRecv::StartReceivingThread, this);
}

void* vcmtpRecv::StartReceivingThread(void* ptr)
{
    ((vcmtpRecv*)ptr)->RunReceivingThread();
    return NULL;
}

void vcmtpRecv::RunReceivingThread()
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
            HandleMulticastPacket();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retx_tcp_sock, &read_set)) {
            //HandleUnicastPacket();
        }
    }
    pthread_exit(0);
}

void vcmtpRecv::HandleMulticastPacket()
{
    static char packet_buffer[VCMTP_PACKET_LEN];
    bzero(packet_buffer, sizeof(packet_buffer));
    VcmtpHeader* header = (VcmtpHeader*) packet_buffer;

    if (recvfrom(mcast_sock, packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
        perror("vcmtpRecv::HandleMulticastPacket() recv error");
    if ( be64toh(header->flags) & VCMTP_BOF) {
        HandleBofMessage(packet_buffer);
    }
}

void vcmtpRecv::HandleBofMessage(char* VcmtpPacket)
{
    uint64_t fileID, seqNum, payloadLen, flags;
    uint64_t fileSize;
    uint8_t  transType;
    char     fileName[256];
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    memcpy(&fileID,     VcmtpPacketHeader,    8);
    memcpy(&seqNum,     (VcmtpPacketHeader+8),  8);
    memcpy(&payloadLen, (VcmtpPacketHeader+16), 8);
    memcpy(&flags,      (VcmtpPacketHeader+24), 8);
    memcpy(&transType,  VcmtpPacketData, 1);
    memcpy(&fileSize,   (VcmtpPacketData+1), 8);
    memcpy(fileName,    (VcmtpPacketData+9), 256);
    std::cout << "(VCMTP Header)fileID: " << be64toh(fileID) << std::endl;
    std::cout << "(VCMTP Header)Seq Num: " << be64toh(seqNum) << std::endl;
    std::cout << "(VCMTP Header)payloadLen: " << be64toh(payloadLen) << std::endl;
    std::cout << "(VCMTP Header)flags: " << be64toh(flags) << std::endl;
    //std::cout << "(BOF) transfer type: " << transType << std::endl;
    printf("(BOF) transfer type: %i\n", transType);
    std::cout << "(BOF) fileSize: " << be64toh(fileSize) << std::endl;
    std::cout << "(BOF) fileName: " << fileName << std::endl;
}
