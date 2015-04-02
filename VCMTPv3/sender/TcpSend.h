/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      TcpSend.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Nov 17, 2014
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
 * @brief     Define the interfaces and structures of sender side TCP layer
 *            abstracted funtions.
 *
 * The TcpSend class includes a set of transmission functions, which are
 * basically the encapsulation of tcp system calls themselves. This abstracted
 * new layer acts as the sender side transmission library.
 */



#ifndef VCMTP_SENDER_TCPSEND_H_
#define VCMTP_SENDER_TCPSEND_H_


#include <arpa/inet.h>
#include <pthread.h>
#include <list>
#include <mutex>
#include <string>

#include "vcmtpBase.h"


class TcpSend
{
public:
    /** source port would be initialized to 0 if not being specified. */
    TcpSend(std::string tcpaddr, unsigned short tcpport = 0);
    ~TcpSend();

    int acceptConn();
    /** return the reference of a socket list */
    const std::list<int>& getConnSockList();
    unsigned short getPortNum();
    void Init(); /*!< start point that upper layer should call */
    /** only parse the header part of a coming packet */
    int parseHeader(int retxsockfd, VcmtpHeader* recvheader);
    /** read any data coming into this given socket */
    int readSock(int retxsockfd, char* pktBuf, int bufSize);
    void rmSockInList(int sockfd);
    /** gathering send by calling io vector system call */
    int send(int retxsockfd, VcmtpHeader* sendheader, char* payload,
        size_t paylen);

private:
    int                sockfd;
    struct sockaddr_in servAddr;
    std::string        tcpAddr;
    unsigned short     tcpPort;
    std::list<int>     connSockList;
    std::mutex         sockListMutex; /*!< to protect shared sockList */
};


#endif /* VCMTP_SENDER_TCPSEND_H_ */
