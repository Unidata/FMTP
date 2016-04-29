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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief     Implement the interfaces of TcpRecv class.
 *
 * Underlying layer of the fmtpRecvv3 class. It handles communication over
 * TCP connections.
 */


#include "TcpRecv.h"

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>


/**
 * Constructor of TcpRecv.
 *
 * @param[in] tcpaddr           The address of the TCP server: either an IPv4
 *                              address in dotted-decimal format or an Internet
 *                              host name.
 * @param[in] tcpport           The port number of the TCP connection in host
 *                              byte-order.
 */
TcpRecv::TcpRecv(const std::string& tcpaddr, unsigned short tcpport)
    : tcpAddr(tcpaddr), tcpPort(tcpport), servAddr()
{
}


/**
 * Establishes a TCP connection to the sender.
 *
 * @param[in] none
 *
 * @throw std::invalid_argument if `tcpAddr` is invalid.
 * @throw std::system_error     if a TCP connection can't be established.
 */
void TcpRecv::Init()
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
 * Receives a header and a payload on the TCP connection. Blocks until a
 * complete packet is received, the end-of-file is encountered, or an error
 * occurs.
 *
 * @param[in] header   Header.
 * @param[in] headLen  Length of the header in bytes.
 * @param[in] payload  Payload.
 * @param[in] payLen   Length of the payload in bytes.
 * @retval    0        EOF encountered.
 * @return             Number of bytes read. Will be `headLen + payLen`.
 * @throws std::system_error  if an error is encountered reading from the
 *                            socket.
 */
size_t TcpRecv::recvData(void* header, size_t headLen, char* payload,
                         size_t payLen)
{
    size_t nread;

    if (header && headLen) {
        nread = recvall(header, headLen);
        if (nread < headLen)
            return 0; // EOF
    }

    if (payload && payLen) {
        nread = recvall(payload, payLen);
        if (nread < payLen)
            return 0; // EOF
    }

    return (headLen + payLen);
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
    sendall(header, headLen);
    sendall(payload, payLen);

    return (headLen + payLen);
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
        if (errno == ECONNREFUSED || errno == ETIMEDOUT ||
                errno == ECONNRESET || errno == EHOSTUNREACH) {
            if (sleep(30)) {
                close(sockfd);
                throw std::system_error(EINTR, std::system_category(),
                    "TcpRecv:TcpRecv() sleep() interrupted");
            }
        }
        else {
            close(sockfd);
            throw std::system_error(errno, std::system_category(),
                    "TcpRecv:TcpRecv() Error connecting to " + servAddr);
        }
    }
}
