/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecv.cpp
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
#include <fcntl.h>
#include <unistd.h>


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

void vcmtpRecv::Stop()
{
    // close all the sock_fd
    // call pthread_join()
    // make sure all resources are released
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
            McastPacketHandler();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retx_tcp_sock, &read_set)) {
            //HandleUnicastPacket();
        }
    }
    pthread_exit(0);
}

void vcmtpRecv::McastPacketHandler()
{
    static char packet_buffer[VCMTP_PACKET_LEN];
    bzero(packet_buffer, sizeof(packet_buffer));
    VcmtpHeader* header = (VcmtpHeader*) packet_buffer;

    if (recvfrom(mcast_sock, packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
        perror("vcmtpRecv::HandleMulticastPacket() recv error");
    if ( be64toh(header->flags) & VCMTP_BOF ) {
        BOFHandler(packet_buffer);
    }
    else if ( be64toh(header->flags) & VCMTP_BOMD ) {
        BOMDHandler(packet_buffer);
    }
    else if ( be64toh(header->flags) & VCMTP_FILE_DATA ) {
        recvFile(packet_buffer);
    }
    else if ( be64toh(header->flags) & VCMTP_MEM_DATA ) {
        recvMemData(packet_buffer);
    }
    else if ( be64toh(header->flags) & VCMTP_EOF ) {
        EOFHandler(packet_buffer);
    }
    else if ( be64toh(header->flags) & VCMTP_EOMD ) {
        EOMDHandler();
    }
}

void vcmtpRecv::BOFHandler(char* VcmtpPacket)
{
    uint64_t fileSize;
    uint8_t  transType;
    char     fileName[256];
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    memcpy(&vcmtpHeader.indexid,     VcmtpPacketHeader,    8);
    memcpy(&vcmtpHeader.seqnum,     (VcmtpPacketHeader+8),  8);
    memcpy(&vcmtpHeader.payloadlen, (VcmtpPacketHeader+16), 8);
    memcpy(&vcmtpHeader.flags,      (VcmtpPacketHeader+24), 8);
    memcpy(&transType,  VcmtpPacketData, 1);
    memcpy(&fileSize,   (VcmtpPacketData+1), 8);
    memcpy(fileName,    (VcmtpPacketData+9), 256);
    vcmtpHeader.indexid = be64toh(vcmtpHeader.indexid);
    vcmtpHeader.seqnum = be64toh(vcmtpHeader.seqnum);
    vcmtpHeader.payloadlen = be64toh(vcmtpHeader.payloadlen);
    vcmtpHeader.flags = be64toh(vcmtpHeader.flags);
    /*
    std::cout << "(VCMTP Header)fileID: " << vcmtpHeader.fileid << std::endl;
    std::cout << "(VCMTP Header)Seq Num: " << vcmtpHeader.seqnum << std::endl;
    std::cout << "(VCMTP Header)payloadLen: " << vcmtpHeader.payloadlen << std::endl;
    std::cout << "(VCMTP Header)flags: " << vcmtpHeader.flags << std::endl;
    */
    //std::cout << "(BOF) transfer type: " << transType << std::endl;
    printf("(BOF) transfer type: %i\n", transType);
    std::cout << "(BOF) fileSize: " << fileSize << std::endl;
    std::cout << "(BOF) fileName: " << fileName << std::endl;
    fileDescriptor = open( fileName, O_RDWR | O_CREAT | O_TRUNC );
    if(fileDescriptor < 0)
        perror("BOFHandler() open file error");
}

void vcmtpRecv::recvFile(char* VcmtpPacket)
{
    char*        VcmtpPacketHeader = VcmtpPacket;
    char*        VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;
    VcmtpHeader  tmpVcmtpHeader;

    memcpy(&tmpVcmtpHeader.indexid,     VcmtpPacketHeader,    8);
    memcpy(&tmpVcmtpHeader.seqnum,     (VcmtpPacketHeader+8),  8);
    memcpy(&tmpVcmtpHeader.payloadlen, (VcmtpPacketHeader+16), 8);
    memcpy(&tmpVcmtpHeader.flags,      (VcmtpPacketHeader+24), 8);
    tmpVcmtpHeader.indexid = be64toh(tmpVcmtpHeader.indexid);
    tmpVcmtpHeader.seqnum = be64toh(tmpVcmtpHeader.seqnum);
    tmpVcmtpHeader.payloadlen = be64toh(tmpVcmtpHeader.payloadlen);
    tmpVcmtpHeader.flags = be64toh(tmpVcmtpHeader.flags);
    // check if it's the same file that BOF declaimed
    if(tmpVcmtpHeader.indexid == vcmtpHeader.indexid &&
       vcmtpHeader.seqnum + vcmtpHeader.payloadlen == tmpVcmtpHeader.seqnum)
    {
        if(write(fileDescriptor, VcmtpPacketData, tmpVcmtpHeader.payloadlen) < 0)
            perror("vcmtpRecv::recvFile() write to file error.");
        vcmtpHeader.seqnum     = tmpVcmtpHeader.seqnum;
        vcmtpHeader.payloadlen = tmpVcmtpHeader.payloadlen;
        vcmtpHeader.flags      = tmpVcmtpHeader.flags;
    }
}

void vcmtpRecv::EOFHandler(char* VcmtpPacket)
{
    // just for udp demo only, should be removed when having retx
    close(fileDescriptor);
}

void vcmtpRecv::BOPHandler(char* VcmtpPacket)
{
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    // every time a new BOMD arrives, save the header to check following data packets
    memcpy(&vcmtpHeader.indexid,    VcmtpPacketHeader,    8);
    memcpy(&vcmtpHeader.seqnum,     (VcmtpPacketHeader+8),  8);
    memcpy(&vcmtpHeader.payloadlen, (VcmtpPacketHeader+16), 8);
    memcpy(&vcmtpHeader.flags,      (VcmtpPacketHeader+24), 8);
    // every time a new BOMD arrives, save the msg to check following data packets
    memcpy(&BOMDmsg.prodsize,   VcmtpPacketData, 8);
    memcpy(BOMDmsg.prodid,      (VcmtpPacketData+8), 256);

    vcmtpHeader.indexid    = be64toh(vcmtpHeader.indexid);
    vcmtpHeader.seqnum     = be64toh(vcmtpHeader.seqnum);
    vcmtpHeader.payloadlen = be64toh(vcmtpHeader.payloadlen);
    vcmtpHeader.flags      = be64toh(vcmtpHeader.flags);
    BOMDmsg.prodsize       = be64toh(BOMDmsg.prodsize);

    #ifdef DEBUG
    std::cout << "(VCMTP Header) indexID: " << vcmtpHeader.indexid << std::endl;
    std::cout << "(VCMTP Header) Seq Num: " << vcmtpHeader.seqnum << std::endl;
    std::cout << "(VCMTP Header) payloadLen: " << vcmtpHeader.payloadlen << std::endl;
    std::cout << "(VCMTP Header) flags: " << vcmtpHeader.flags << std::endl;
    std::cout << "(BOP) prodSize: " << BOPmsg.prodsize << std::endl;
    std::cout << "(BOP) metaSize: " << BOPmsg.metasize << std::endl;
    #endif

    notifier.notify_of_bop(BOPmsg.prodsize, BOPmsg.metadata,
                           BOPmsg.metaSize, &prodptr);
}

void vcmtpRecv::recvMemData(char* VcmtpPacket)
{
    char*        VcmtpPacketHeader = VcmtpPacket;
    char*        VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;
    VcmtpHeader  tmpVcmtpHeader;

    memcpy(&tmpVcmtpHeader.indexid,     VcmtpPacketHeader,    8);
    memcpy(&tmpVcmtpHeader.seqnum,     (VcmtpPacketHeader+8),  8);
    memcpy(&tmpVcmtpHeader.payloadlen, (VcmtpPacketHeader+16), 8);
    memcpy(&tmpVcmtpHeader.flags,      (VcmtpPacketHeader+24), 8);
    tmpVcmtpHeader.indexid = be64toh(tmpVcmtpHeader.indexid);
    tmpVcmtpHeader.seqnum = be64toh(tmpVcmtpHeader.seqnum);
    tmpVcmtpHeader.payloadlen = be64toh(tmpVcmtpHeader.payloadlen);
    tmpVcmtpHeader.flags = be64toh(tmpVcmtpHeader.flags);

    // check for the first mem data packet
    if(tmpVcmtpHeader.indexid == vcmtpHeader.indexid &&
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
    else if(tmpVcmtpHeader.indexid == vcmtpHeader.indexid && vcmtpHeader.seqnum
            + vcmtpHeader.payloadlen == tmpVcmtpHeader.seqnum)
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

void vcmtpRecv::EOMDHandler()
{
    std::cout << "Mem data completely received." << std::endl;
}
