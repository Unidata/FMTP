/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpRecvv3.h
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
 * @brief     Define the interfaces of VCMTPv3 receiver.
 *
 * Receiver side of VCMTPv3 protocol. It handles incoming multicast packets
 * and issues retransmission requests to the sender side.
 */


#ifndef VCMTPRECVV3_H_
#define VCMTPRECVV3_H_

#include "vcmtpBase.h"
#include "ReceivingApplicationNotifier.h"
#include <stdint.h>
#include <string>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include "TcpRecv.h"

using namespace std;

class vcmtpRecvv3 {
public:
    vcmtpRecvv3(string tcpAddr,
                const unsigned short tcpPort,
                string mcastAddr,
                const unsigned short mcastPort,
                ReceivingApplicationNotifier* notifier);
    vcmtpRecvv3(string tcpAddr,
                const unsigned short tcpPort,
                string mcastAddr,
                const unsigned short mcastPort);
    ~vcmtpRecvv3();

    void    Start();

private:
    string           tcpAddr;
    unsigned short   tcpPort;
    string           mcastAddr;
    unsigned short   mcastPort;
    int              mcastSock;
    int              retxSock;
    struct sockaddr_in  mcastgroup;
    struct ip_mreq   mreq;          /*!< struct of multicast object */
    VcmtpHeader      vcmtpHeader;   /*!< temporary header buffer for each vcmtp packet */
    BOPMsg           BOPmsg;        /*!< begin of product struct */
    ReceivingApplicationNotifier* notifier; /*!< callback function of the receiving application */
    void*            prodptr;       /*!< pointer to a start point in product queue */
    TcpRecv*         tcprecv;
    queue<INLReqMsg> msgqueue;

    void    joinGroup(string mcastAddr, const unsigned short mcastPort);
    static void*  StartRetxHandler(void* ptr);
    void    StartRetxHandler();
    void    mcastHandler();
    void    retxHandler();
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
    void decodeHeader(
            char* const  packet,
            const size_t nbytes,
            VcmtpHeader& header,
            char** const payload);
    /**
     * Parse BOP message and call notifier to notify receiving application.
     *
     * @param[in] header           Header associated with the packet.
     * @param[in] VcmtpPacketData  Pointer to payload of VCMTP packet.
     * @throw std::runtime_error   if the payload is too small.
     */
    void BOPHandler(
            const VcmtpHeader& header,
            const char* const  VcmtpPacketData);
    void    EOPHandler();
    /**
     * Parse data blocks, directly store and check for missing blocks.
     *
     * @param[in] header             The header associated with the packet.
     * @param[in] VcmtpPacket        Pointer to received vcmtp packet in buffer.
     */
    void recvMemData(
            const VcmtpHeader& header,
            const char* const  VcmtpPacket);

    void    sendRetxEnd();
    void    sendRetxReq();
    void    recvRetxData();
};

#endif /* VCMTPRECVV3_H_ */
