/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      TcpRecv.cpp
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
 * @brief     Implement the interfaces of TcpRecv class.
 *
 * Underlying layer of the vcmtpRecvv3 class. It handles communication over
 * TCP connections.
 */


#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/uio.h>
#include "TcpRecv.h"

using namespace std;


/**
 * Constructor of TcpRecv. It establishes a TCP connection to the sender.
 *
 * @param[in] tcpAddr        The address of the TCP connection.
 * @param[in] tcpPort        The port number of the TCP connection.
 */
TcpRecv::TcpRecv(string tcpAddr, unsigned short tcpPort)
:
    mutex()
{
    (void) memset((char *) &servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(tcpAddr.c_str());
    servAddr.sin_port = htons(tcpPort);
    initSocket();
}


/**
 * Destructor of TcpRecv.
 *
 * @param[in] none
 */
TcpRecv::~TcpRecv()
{
    close(getSocket());
}


/**
 * Sends a header and a payload on the TCP connection. Blocks until the packet
 * is sent or a severe error occurs. Re-establishes the TCP connection if
 * necessary.
 *
 * @param[in] header   Header.
 * @param[in] headLen  Length of the header in bytes.
 * @param[in] payload  Payload.
 * @param[in] payLen   Length of the payload in bytes.
 * @retval    -1       O/S failure.
 * @return             Number of bytes sent.
 */
ssize_t TcpRecv::sendData(void* header, size_t headLen, char* payload,
                          size_t payLen)
{
    ssize_t      nbytes;
    struct iovec iov[2];

    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    for (;;) {
        nbytes = writev(getSocket(), iov, 2);
        if (nbytes != -1)
            break;
        reconnect();
    }

    return nbytes; // Eclipse wants to see a return
}


/**
 * Receives a header and a payload on the TCP connection. Blocks until the
 * packet is received or a severe error occurs. Re-establishes the TCP
 * connection if necessary.
 *
 * @param[in] header   Header.
 * @param[in] headLen  Length of the header in bytes.
 * @param[in] payload  Payload.
 * @param[in] payLen   Length of the payload in bytes.
 * @retval    -1       O/S failure.
 * @return             Number of bytes received.
 */
ssize_t TcpRecv::recvData(void* header, size_t headLen, char* payload,
                          size_t payLen)
{
    ssize_t      nbytes = 0;
    struct iovec iov[2];

    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    for (;;) {
        nbytes = readv(getSocket(), iov, 2);
        if (nbytes != -1)
            break;
        reconnect();
    }

    return nbytes;
}


/**
 * Initializes the TCP connection. Blocks until the connection is established
 * or a severe error occurs. The interval between two trials is 30 seconds.
 *
 * @throws std::system_error  if the socket is not created.
 * @throws std::system_error  if connect() returns errors.
 */
void TcpRecv::initSocket()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        throw std::system_error(errno, std::system_category(),
                "TcpRecv::TcpRecv() error creating socket");

    while (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))) {
        if (errno != ECONNREFUSED && errno != ETIMEDOUT &&
                errno != ECONNRESET && errno != EHOSTUNREACH) {
            throw std::system_error(errno, std::system_category(),
                    "TcpRecv:TcpRecv() error connecting to sender");
        }
        sleep(30);
    }
}


/**
 * Ensures that the TCP connection is established. Blocks until the connection
 * is established or a severe error occurs. Does nothing if the connection is
 * OK. This method is thread-safe.
 *
 * @param[in] none
 */
void TcpRecv::reconnect()
{
    int                          status;
    socklen_t                    len = sizeof(status);
    std::unique_lock<std::mutex> lock(mutex);

    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &status, &len) || status) {
        close(sockfd);
        initSocket();
    }
}


/**
 * Returns the socket corresponding to the TCP connection. This method is
 * thread-safe.
 *
 * @return  The corresponding socket.
 */
int TcpRecv::getSocket()
{
    std::unique_lock<std::mutex> lock(mutex);
    return sockfd;
}
