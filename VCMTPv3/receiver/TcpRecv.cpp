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


#include "TcpRecv.h"

#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>


using namespace std;


/**
 * Constructor of TcpRecv. It establishes a TCP connection to the sender.
 *
 * @param[in] tcpAddr           The address of the TCP server: either an IPv4
 *                              address in dotted-decimal format or an Internet
 *                              host name.
 * @param[in] tcpPort           The port number of the TCP connection in host
 *                              byte-order.
 * @throw std::invalid_argument if `tcpAddr` is invalid.
 * @throw std::system_error     if a TCP connection can't be established.
 */
TcpRecv::TcpRecv(const string& tcpAddr, unsigned short tcpPort)
{
    (void) memset((char *) &servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    in_addr_t inAddr = inet_addr(tcpAddr.c_str());
    if ((in_addr_t)-1 == inAddr) {
        const struct hostent* hostEntry = gethostbyname(tcpAddr.c_str());
        if (hostEntry == NULL)
            throw std::invalid_argument(
                    std::string("Invalid TCP-server identifier: \"") +
                    tcpAddr + "\"");
        if (hostEntry->h_addrtype != AF_INET || hostEntry->h_length != 4)
            throw std::invalid_argument( std::string("TCP-server \"") + tcpAddr
                    + "\" doesn't have an IPv4 address");
        inAddr = *(in_addr_t*)hostEntry->h_addr_list[0];
    }
    servAddr.sin_addr.s_addr = inAddr;
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
    close(sockfd);
}


/**
 * Sends a header and a payload on the TCP connection. Blocks until the packet
 * is sent or a severe error occurs.
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
    struct iovec iov[2];

    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    return writev(sockfd, iov, 2);
}


/**
 * Receives a header and a payload on the TCP connection. Blocks until a packet
 * is received or an error occurs.
 *
 * @param[in] header   Header.
 * @param[in] headLen  Length of the header in bytes.
 * @param[in] payload  Payload.
 * @param[in] payLen   Length of the payload in bytes.
 * @return             Number of bytes received.
 * @throw std::logic_error  if the socket was closed while being read.
 * @throw std::system_error if the socket couldn't be read.
 */
ssize_t TcpRecv::recvData(void* header, size_t headLen, char* payload,
                          size_t payLen)
{
    struct iovec iov[2];

    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    const ssize_t nbytes = readv(sockfd, iov, 2);

    if (nbytes > 0)
        return nbytes;

    std::string sockStr = std::to_string(static_cast<long long>(sockfd));

    if (nbytes == 0)
        throw std::logic_error("TcpRecv::recvData(): Socket " + sockStr +
                " was closed");

    throw std::system_error(errno, std::system_category(),
            "TcpRecv::recvData(): Couldn't read from socket " + sockStr);
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

#if 0
    cout << std::string("TcpRecv::initSocket(): Connecting TCP socket ").
            append(std::to_string(sockfd)).append(" to ").
            append(inet_ntoa(servAddr.sin_addr)).append(":").
            append(std::to_string(ntohs(servAddr.sin_port))).append("\n");
#endif
    while (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))) {
        if (errno == ECONNREFUSED || errno == ETIMEDOUT ||
                errno == ECONNRESET || errno == EHOSTUNREACH) {
            if (sleep(30))
                throw std::system_error(EINTR, std::system_category(),
                    "TcpRecv:TcpRecv() sleep() interrupted");
        }
        else {
            throw std::system_error(errno, std::system_category(),
                    "TcpRecv:TcpRecv() Error connecting to " + servAddr);
        }
    }
}
