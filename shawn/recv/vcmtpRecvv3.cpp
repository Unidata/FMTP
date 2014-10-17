/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecvv3.cpp
 *
 * @history:
 *      Created on : Oct 17, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include "vcmtpRecvv3.h"
#include <stdio.h>
//#include <iostream>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <endian.h>
//#include <strings.h>
//#include <memory.h>
//#include <pthread.h>
//#include <fcntl.h>
//#include <unistd.h>


vcmtpRecvv3::vcmtpRecvv3()
{
}

vcmtpRecvv3::~vcmtpRecvv3()
{
}

void vcmtpRecvv3::joinGroup(const char* mcastAddr, unsigned short mcastPort)
{
    bzero(&mcastgroup, sizeof(mcastgroup));
    mcastgroup.sin_family = AF_INET;
    mcastgroup.sin_addr.s_addr = inet_addr(mcastAddr);
    mcastgroup.sin_port = htons(mcastPort);
    if( (mcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
        perror("vcmtpRecvv3::joinGroup() creating socket failed.");
    if( bind(mcast_sock, (sockaddr*)&mcastgroup, sizeof(mcastgroup)) < 0 )
        perror("vcmtpRecvv3::joinGroup() binding socket failed.");
}

void vcmtpRecvv3::BOPHandler(char* msgptr, size_t msgsize)
{
    char*    VcmtpPacketHeader = VcmtpPacket;
    char*    VcmtpPacketData = VcmtpPacket + VCMTP_HEADER_LEN;

    // every time a new BOP arrives, save the header to check following data packets
    memcpy(&vcmtpHeader.indexid,    VcmtpPacketHeader,    8);
    memcpy(&vcmtpHeader.seqnum,     (VcmtpPacketHeader+8),  8);
    memcpy(&vcmtpHeader.payloadlen, (VcmtpPacketHeader+16), 8);
    memcpy(&vcmtpHeader.flags,      (VcmtpPacketHeader+24), 8);
    // every time a new BOP arrives, save the msg to check following data packets
    memcpy(&BOPmsg.prodsize,   VcmtpPacketData, 8);
    memcpy(BOPmsg.prodname,    (VcmtpPacketData+8), 256);

    vcmtpHeader.indexid    = be64toh(vcmtpHeader.indexid);
    vcmtpHeader.seqnum     = be64toh(vcmtpHeader.seqnum);
    vcmtpHeader.payloadlen = be64toh(vcmtpHeader.payloadlen);
    vcmtpHeader.flags      = be64toh(vcmtpHeader.flags);
    BOPmsg.prodsize        = be64toh(BOPmsg.prodsize);

    #ifdef DEBUG
    std::cout << "(VCMTP Header) indexID: " << vcmtpHeader.indexid << std::endl;
    std::cout << "(VCMTP Header) Seq Num: " << vcmtpHeader.seqnum << std::endl;
    std::cout << "(VCMTP Header) payloadLen: " << vcmtpHeader.payloadlen << std::endl;
    std::cout << "(VCMTP Header) flags: " << vcmtpHeader.flags << std::endl;
    std::cout << "(BOP) prodsize: " << BOPmsg.prodsize << std::endl;
    std::cout << "(BOP) prodname: " << BOPmsg.prodname << std::endl;
    #endif
}

uint32_t vcmtpRecvv3::recvProduct(void* data, size_t dataSize)
{
    return recvProduct(data, dataSize, 0, 0);
}

uint32_t vcmtpRecvv3::recvProduct(void* data, size_t dataSize, void* metadata,
                                  unsigned metaSize)
{
}
