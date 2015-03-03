/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      TcpRecv.h
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
 * @brief     Define the interfaces of TcpRecv class.
 *
 * Underlying layer of the vcmtpRecvv3 class. It handles communication over
 * TCP connections.
 */


#ifndef VCMTP_RECEIVER_TCPRECV_H_
#define VCMTP_RECEIVER_TCPRECV_H_


#include <arpa/inet.h>
#include <mutex>
#include <netinet/in.h>
#include <string>


class TcpRecv
{
public:
    TcpRecv(const std::string& tcpAddr, unsigned short tcpPort);
    ~TcpRecv();
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
    ssize_t sendData(void* header, size_t headLen, char* payload,
                     size_t payLen);
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
    ssize_t recvData(void* header, size_t headLen, char* payload,
                     size_t payLen);

private:
    int                     sockfd;
    struct sockaddr_in      servAddr;

    /**
     * Initializes the TCP connection. Blocks until the connection is established
     * or a severe error occurs.
     *
     * @throws std::system_error  if a system error occurs.
     */
    void initSocket();
};


inline std::string operator+(const std::string& lhs, const struct sockaddr_in& rhs)
{
    return lhs + inet_ntoa(rhs.sin_addr) + ":" +
            std::to_string(static_cast<long long>(ntohs(rhs.sin_port)));
}


#endif /* VCMTP_RECEIVER_TCPRECV_H_ */
