/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpSendv3.h
 * @author    Fatma Alali <fha6np@virginia.edu>
 *            Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 16, 2014
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
 * @brief     Define the interfaces of VCMTPv3 sender side method function.
 *
 * Sender side of VCMTPv3 protocol. It multicasts packets out to multiple
 * receivers and retransmits missing blocks to the receivers.
 */


#ifndef VCMTPSENDV3_H_
#define VCMTPSENDV3_H_


#include <sys/types.h>
#include "UdpSend.h"
#include "TcpSend.h"
#include "vcmtpBase.h"
#include "senderMetadata.h"
#include "SendingApplicationNotifier.h"
#include <map>
#include <set>
#include <pthread.h>
#include <ctime>

class vcmtpSendv3;

/**
 * To contain multiple types of necessary information and transfer to the
 * StartRetxThread() as one single parameter.
 */
struct StartRetxThreadInfo
{
    /**
     * A pointer to the vcmtpSendv3 instance itself which starts the
     * StartNewRetxThread().
     */
    vcmtpSendv3*    retxmitterptr;
    /** The particular retx socket this running thread is listening on */
    int             retxsockfd;
};


/**
 * To contain multiple types of necessary information and transfer to the
 * StartTimerThread() as one single parameter.
 */
struct StartTimerThreadInfo
{
    uint32_t        prodindex; /*!< product index */
    vcmtpSendv3*    sender;    /*!< a poniter to the vcmtpSendv3 instance */
};


/**
 * sender side class handling the multicasting, restransmission and timeout.
 */
class vcmtpSendv3
{
public:
    vcmtpSendv3(
            const char*          tcpAddr,
            const unsigned short tcpPort,
            const char*          mcastAddr,
            const unsigned short mcastPort);
    vcmtpSendv3(
            const char*                 tcpAddr,
            const unsigned short        tcpPort,
            const char*                 mcastAddr,
            const unsigned short        mcastPort,
            uint32_t                    initProdIndex,
            SendingApplicationNotifier* notifier);
    vcmtpSendv3(
            const char*                 tcpAddr,
            const unsigned short        tcpPort,
            const char*                 mcastAddr,
            const unsigned short        mcastPort,
            uint32_t                    initProdIndex,
            float                       timeoutRatio,
            SendingApplicationNotifier* notifier);
    ~vcmtpSendv3();

    uint32_t sendProduct(char* data, size_t dataSize);
    uint32_t sendProduct(char* data, size_t dataSize, char* metadata,
                         unsigned metaSize);
    void startCoordinator();
    unsigned short getTcpPortNum();

private:
    uint32_t                    prodIndex;
    /** underlying udp layer instance */
    UdpSend*                    udpsend;
    /** underlying tcp layer instance */
    TcpSend*                    tcpsend;
    /** maintaining metadata for retx use. */
    senderMetadata*             sendMeta;
    /** sending application callback hook */
    SendingApplicationNotifier* notifier;
    float                       retxTimeoutRatio;
    /** first: socket fd;  second: pthread_t pointer */
    map<int, pthread_t*> retxSockThreadMap;
    /** first: socket fd;  second: retransmission finished indicator */
    map<int, bool>	retxSockFinishMap;
    /** first: socket fd;  second: pointer to the retxThreadInfo struct */
    map<int, StartRetxThreadInfo*> retxSockInfoMap;
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();
    void StartNewRetxThread(int newtcpsockfd);
    void startTimerThread(uint32_t prodindex);
    /** new coordinator thread */
    static void* coordinator(void* ptr);
    /** new retranmission thread */
    static void* StartRetxThread(void* ptr);
    /** new timer thread */
    static void* runTimerThread(void* ptr);
    void RunRetxThread(int retxsockfd);

    /* Prevent copying because it's meaningless */
    vcmtpSendv3(vcmtpSendv3&);
    vcmtpSendv3& operator=(const vcmtpSendv3&);
};

#endif /* VCMTPSENDV3_H_ */
