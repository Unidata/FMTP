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
    void    Stop();
    void    sendRetxEnd();
    void    sendRetxReq();
    void	recvRetxData();

private:
    string           tcpAddr;
    unsigned short   tcpPort;
    string           mcastAddr;
    unsigned short   mcastPort;
    int              max_sock_fd;
    int              mcast_sock;
    int              retx_tcp_sock;
    struct sockaddr_in  mcastgroup;
    VcmtpHeader      vcmtpHeader;   /*!< temporary header buffer for each vcmtp packet */
    BOPMsg           BOPmsg;        /*!< begin of product struct */
    ReceivingApplicationNotifier* notifier; /*!< callback function of the receiving application */
    void*            prodptr;       /*!< pointer to a start point in product queue */
    struct ip_mreq   mreq;          /*!< struct of multicast object */
    TcpRecv*         tcprecv;

    void    joinGroup(string mcastAddr, const unsigned short mcastPort);
    static void*  StartRetxHandler(void* ptr);
    void    StartRetxHandler();
    void    mcastHandler();
    void    retxHandler();
    void    BOPHandler(char* VcmtpPacket);
    void    EOPHandler();
    void    recvMemData(char* VcmtpPacket);
};

#endif /* VCMTPRECVV3_H_ */
