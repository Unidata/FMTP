/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      UdpSend.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 23, 2014
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
 * @brief     Implement the interfaces and structures of sender side UDP layer
 *            abstracted funtions.
 *
 * The UdpSend class includes a set of transmission functions, which are
 * basically the encapsulation of udp system calls themselves. This abstracted
 * new layer acts as the sender side transmission library.
 */


#include "UdpSend.h"

#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <system_error>


#ifndef NULL
    #define NULL 0
#endif


/**
 * Constructor, set the IP address and port of the receiver, TTL and default
 * multicast ingress interface.
 *
 * @param[in] recvAddr     IP address of the receiver.
 * @param[in] recvport     Port number of the receiver.
 * @param[in] ttl          Time to live.
 * @param[in] ifAddr       IP of interface to listen for multicast.
 */
UdpSend::UdpSend(const std::string& recvaddr, const unsigned short recvport,
                 const unsigned char ttl, const std::string& ifAddr)
    : recvAddr(recvaddr), recvPort(recvport), ttl(ttl), ifAddr(ifAddr),
      sock_fd(-1), recv_addr()
{
}


/**
 * Destruct elements within the udpsend entity which needs to be deleted.
 *
 * @param[in] none
 */
UdpSend::~UdpSend()
{
}


/**
 * Initializer. It creates a new UDP socket and sets the address and port from
 * the pre-set parameters. Also it connects to the created socket and if
 * necessary, sets the TTL field with a new value.
 *
 * @throws std::system_error  if socket creation fails.
 * @throws std::system_error  if connecting to socket fails.
 * @throws std::system_error  if setting TTL fails.
 */
void UdpSend::Init()
{
    int newttl = ttl;
    struct in_addr interfaceIP;
    /** create a UDP datagram socket. */
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::system_error(errno, std::system_category(),
                "UdpSend::UdpSend() Couldn't create UDP socket");

    /** clear struct recv_addr. */
    (void) memset(&recv_addr, 0, sizeof(recv_addr));
    /** set connection type to IPv4 */
    recv_addr.sin_family = AF_INET;
    /** set the address to the receiver address passed to the constructor */
    recv_addr.sin_addr.s_addr = inet_addr(recvAddr.c_str());
    /** set the port number to the port number passed to the constructor */
    recv_addr.sin_port = htons(recvPort);

    int reuseaddr = true;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                   sizeof(reuseaddr)) < 0) {
        throw std::system_error(errno, std::system_category(), std::string(
                "UdpSend::Init() Couldn't enable Address reuse"));
    }

#ifdef SO_REUSEPORT
    int reuseport = true;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuseport,
                   sizeof(reuseport)) < 0) {
        throw std::system_error(errno, std::system_category(), std::string(
                "UdpSend::Init() Couldn't enable Port reuse"));
    }
#endif

    if (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &newttl,
                sizeof(newttl)) < 0) {
        throw std::system_error(errno, std::system_category(), std::string(
                "UdpSend::Init() Couldn't set UDP socket time-to-live "
                "option to ") + std::to_string(ttl));
    }

    interfaceIP.s_addr = inet_addr(ifAddr.c_str());
    if (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &interfaceIP,
                   sizeof(interfaceIP)) < 0) {
        throw std::system_error(errno, std::system_category(), std::string(
                "UdpSend::Init() Couldn't set UDP socket default interface"));
    }
}


/**
 * SendData() sends the packet content separated in two different physical
 * locations, which is put together into a io vector structure.
 *
 * @param[in] *header       a constant void type pointer that points to where
 *                          the content of the packet header lies.
 * @param[in] headerlen     length of that packet header.
 * @param[in] *data         a constant void type pointer that points to where
 *                          the piece of memory data to be sent lies.
 * @param[in] datalen       length of that piece of memory data.
 */
ssize_t UdpSend::SendData(void* header, const size_t headerLen, void* data,
                          const size_t dataLen)
{
    struct msghdr msg;
    /** vector including the two memory locations */
    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    msg.msg_name       = &recv_addr;
    msg.msg_namelen    = sizeof(recv_addr);
    msg.msg_iov        = iov;
    msg.msg_iovlen     = 2;
    msg.msg_control    = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags      = 0;

    int ret = sendmsg(sock_fd, &msg, 0);
    return ret;
}


/**
 * Send a piece of memory data given by the buff pointer and len length.
 *
 * @param[in] *buff              a constant void type pointer that points to
 *                               where the piece of memory data to be sent lies.
 * @param[in] len                length of that piece of memory data.
 * @throws    std::system_error  if an error occurs writing to the the UDP
 *                               socket.
 */
ssize_t UdpSend::SendTo(const void* buff, size_t len)
{
    const ssize_t nbytes = sendto(sock_fd, buff, len, 0,
                                  (struct sockaddr *)&recv_addr,
                                  sizeof(recv_addr));

    if (nbytes == -1)
        throw std::system_error(errno, std::system_category(),
                "Couldn't write to UDP socket " + sock_fd);

    return nbytes;
}


/**
 * Gather-send a VCMTP packet.
 *
 * @param[in] iovec              First I/O vector.
 * @param[in] nvec               Number of I/O vectors.
 * @throws    std::system_error  if an error occurs writing to the the UDP
 *                               socket.
 */
int UdpSend::SendTo(struct iovec* const iovec, const int nvec)
{
    struct msghdr msg;

    msg.msg_name       = &recv_addr;
    msg.msg_namelen    = sizeof(recv_addr);
    msg.msg_iov        = iovec;
    msg.msg_iovlen     = nvec;
    msg.msg_control    = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags      = 0;

    int nbytes = sendmsg(sock_fd, &msg, 0);

    if (nbytes == -1)
        throw std::system_error(errno, std::system_category(),
                "Couldn't write to UDP socket " + sock_fd);

    return nbytes;
}
