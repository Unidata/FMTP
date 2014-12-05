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
#include "UdpSocket.h"
#include "TcpSend.h"
#include "vcmtpBase.h"
#include "senderMetadata.h"
#include "timer.h"
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
	vcmtpSendv3* 	retxmitterptr;
	/** The particular retx socket this running thread is listening on */
	int				retxsockfd;
    /** prodindex to prodptr map.  Format: <prodindex, prodptr> */
	map<uint32_t, void*>* retxIndexProdptrMap;
};


struct StartTimerThreadInfo
{
	uint32_t		prodindex;
	senderMetadata* sendmeta;
};


class vcmtpSendv3
{
public:
    vcmtpSendv3(
            const char*          tcpAddr,
            const unsigned short tcpPort,
            const char*          mcastAddr,
            const unsigned short mcastPort);
    vcmtpSendv3(
            const char*          tcpAddr,
            const unsigned short tcpPort,
            const char*          mcastAddr,
            const unsigned short mcastPort,
            uint32_t             initProdIndex);

    ~vcmtpSendv3();
    uint32_t sendProduct(char* data, size_t dataSize);
    uint32_t sendProduct(char* data, size_t dataSize, char* metadata,
                         unsigned metaSize);
    void startCoordinator();
    static void* coordinator(void* ptr);
    void initRetxConn();
    unsigned short getTcpPortNum();
    void StartNewRetxThread(int newtcpsockfd);
    static void* StartRetxThread(void* ptr);

private:
    uint32_t 	      prodIndex;
    UdpSocket* 	  udpsocket;
    TcpSend*   	  tcpsend;
    senderMetadata* sendMeta; /*!< maintaining metadata for retx use. */
    //TODO: a more precise timeout mechanism should be studied.
    /** first: socket fd;  second: pthread_t pointer */
	map<int, pthread_t*> retxSockThreadMap;
    /** first: socket fd;  second: retransmission finished indicator */
	map<int, bool>	retxSockFinishMap;
	/** first: socket fd;  second: pointer to the corresponding retxThreadInfo struct */
	map<int, StartRetxThreadInfo*> retxSockInfoMap;
    void SendBOPMessage(uint32_t prodSize, void* metadata, unsigned metaSize);
    void sendEOPMessage();
    void RunRetxThread(int retxsockfd, map<uint, void*>& retxIndexProdptrMap);
	void startTimerThread(uint32_t prodindex);
    static void* runTimerThread(void* ptr);
};

#endif /* VCMTPSENDV3_H_ */
