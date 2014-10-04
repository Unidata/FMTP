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

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "vcmtpReceiver.h"

using namespace std;

VCMTPReceiver::VCMTPReceiver(
    string&                 tcpAddr,
    const unsigned short    tcpPort)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort)
{
    init();
}

VCMTPReceiver::~VCMTPReceiver()
{
    delete retrans_tcp_client;
    pthread_mutex_destroy(&retrans_list_mutex);
}

void VCMTPReceiver::init()
{
    retrans_tcp_client = 0;
    max_sock_fd = 0;
    //multicast_sock = ptr_multicast_comm->GetSocket();
    retrans_tcp_sock = 0;
    packet_loss_rate = 0;
    session_id = 0;
    //status_proxy = 0;
    time_diff_measured = false;
    time_diff = 0;
    read_ahead_header = (VcmtpHeader*)read_ahead_buffer;
    read_ahead_data = read_ahead_buffer + VCMTP_HLEN;
    recv_thread = 0;
    retrans_thread = 0;
    keep_retrans_alive = false;
    vcmtp_seq_num = 0;
    total_missing_bytes = 0;
    received_retrans_bytes = 0;
    is_multicast_finished = false;
    retrans_switch = true;
    read_ahead_header->session_id = -1;
}

void VCMTPReceiver::Start()
{
    StartReceivingThread();
}

void VCMTPReceiver::StartReceivingThread()
{
    pthread_create(&recv_thread, NULL, &VCMTPReceiver::StartReceivingThread, this);
}

void* VCMTPReceiver::StartReceivingThread(void* ptr)
{
    ((VCMTPReceiver*)ptr)->RunReceivingThread();
    return NULL;
}

void VCMTPReceiver::RunReceivingThread()
{
    fd_set  read_set;
    while(true) {
        std::cout << "RunReceivingThread() running" << std::endl;
        read_set = read_sock_set;
        if (select(max_sock_fd + 1, &read_set, NULL, NULL, NULL) == -1) {
            //SysError("TcpServer::SelectReceive::select() error");
            break;
        }

        // tests to see if multicast_sock is part of the set
        if (FD_ISSET(multicast_sock, &read_set)) {
            std::cout << "enter HandleMulticastPacket()" << std::endl;
            HandleMulticastPacket();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retrans_tcp_sock, &read_set)) {
            //HandleUnicastPacket();
        }
    }
    pthread_exit(0);
}

int VCMTPReceiver::udpBindIP2Sock(string localAddr, const unsigned short localPort)
{
    //struct sockaddr_in sender;
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
    //socklen_t sendAddrSize = sizeof(sender);
    //recvfrom(multicast_sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *) &sender, &sendAddrSize);
}

int VCMTPReceiver::ConnectSenderOnTCP()
{
    return 0;
}

void VCMTPReceiver::HandleMulticastPacket()
{
    static char packet_buffer[VCMTP_PACKET_LEN];
    static VcmtpHeader* header = (VcmtpHeader*) packet_buffer;
    static char* packet_data = packet_buffer + VCMTP_HLEN;

    static VcmtpSenderMessage *ptr_sender_msg = (VcmtpSenderMessage *) packet_data;
    //static map<uint, MessageReceiveStatus>::iterator it;

    //if (ptr_multicast_comm->RecvData(packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
    if (recvfrom(multicast_sock, packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
        std::cout << "VCMTPReceiver::HandleMulticastPacket() recv error" << std::endl;
    if (header->flags & VCMTP_BOF) {
        HandleBofMessage(packet_buffer);
    }
}

void VCMTPReceiver::HandleBofMessage(char* VcmtpPacket)
{
    uint64_t fileID, seqNum, payloadLen, flags;
    uint64_t fileSize;
    char     fileName[256];
    VcmtpHeader* VcmtpPacketHeader = (VcmtpHeader*) VcmtpPacket;
    char* VcmtpPacketData = VcmtpPacket + VCMTP_HLEN;

    memcpy(&fileID, VcmtpPacketHeader, 8);
    memcpy(&seqNum, VcmtpPacketHeader + 8, 8);
    memcpy(&payloadLen, VcmtpPacketHeader + 16, 8);
    memcpy(&flags, VcmtpPacketHeader + 24, 8);
    memcpy(&fileSize, VcmtpPacketData + 1, 8);
    memcpy(fileName, VcmtpPacketData + 9, 256);
    std::cout << fileID << std::endl;
    std::cout << fileName << std::endl;
    std::cout << fileSize << std::endl;
}
