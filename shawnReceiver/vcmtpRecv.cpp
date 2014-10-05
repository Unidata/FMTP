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

//#include <stdint.h>
#include "vcmtpRecv.h"
#include <stdio.h>

using namespace std;

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
    multicast_sock = 0;
    retrans_tcp_sock = 0;
    recv_thread = 0;
    retrans_thread = 0;
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
    if( (multicast_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        std::cout << "udpBindIP2Sock() creating socket failed." << std::endl;
    if( bind(multicast_sock, (sockaddr*)&sender, sizeof(sender)) < 0 )
        std::cout << "udpBindIP2Sock() binding socket failed." << std::endl;
    FD_SET(multicast_sock, &read_sock_set);
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
        if (FD_ISSET(multicast_sock, &read_set)) {
            HandleMulticastPacket();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retrans_tcp_sock, &read_set)) {
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

    if (recvfrom(multicast_sock, packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
        std::cout << "vcmtpRecv::HandleMulticastPacket() recv error" << std::endl;
    if (header->flags & VCMTP_BOF) {
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

    unsigned char b1,b2,b3,b4,b5,b6,b7,b8;
    memcpy(&b1, VcmtpPacketHeader,   1);
    memcpy(&b2, VcmtpPacketHeader+1, 1);
    memcpy(&b3, VcmtpPacketHeader+2, 1);
    memcpy(&b4, VcmtpPacketHeader+3, 1);
    memcpy(&b5, VcmtpPacketHeader+4, 1);
    memcpy(&b6, VcmtpPacketHeader+5, 1);
    memcpy(&b7, VcmtpPacketHeader+6, 1);
    memcpy(&b8, VcmtpPacketHeader+7, 1);
    printf("%u ", b1);
    printf("%u ", b2);
    printf("%u ", b3);
    printf("%u ", b4);
    printf("%u ", b5);
    printf("%u ", b6);
    printf("%u ", b7);
    printf("%u ", b8);
    printf("\n");

    memcpy(&fileID,     VcmtpPacketHeader,    8);
    memcpy(&seqNum,     (VcmtpPacketHeader+8),  8);
    memcpy(&payloadLen, (VcmtpPacketHeader+16), 8);
    memcpy(&flags,      (VcmtpPacketHeader+24), 8);
    memcpy(&transType,  VcmtpPacketData, 8);
    memcpy(&fileSize,   (VcmtpPacketData+8), 8);
    memcpy(fileName,    (VcmtpPacketData+16), 256);
    std::cout << "(VCMTP Header)fileID: " << fileID << std::endl;
    std::cout << "(VCMTP Header)Seq Num: " << seqNum << std::endl;
    std::cout << "(VCMTP Header)payloadLen: " << payloadLen << std::endl;
    std::cout << "(VCMTP Header)flags: " << flags << std::endl;
    std::cout << "(BOF) transfer type: " << transType << std::endl;
    std::cout << "(BOF) fileSize: " << fileSize << std::endl;
    std::cout << "(BOF) fileName: " << fileName << std::endl;
}
