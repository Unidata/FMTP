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

#include <stdexcept>


#ifndef NULL
    #define NULL 0
#endif


/**
 * Constructor, set the IP address and port of the receiver.
 *
 * @param[in] recvaddr     IP address of the receiver.
 * @param[in] recvport     Port number of the receiver.
 */
UdpSend::UdpSend(const std::string& recvaddr, unsigned short recvport)
    : recvAddr(recvaddr), recvPort(recvport)
{
}


/**
 * Constructor, set the IP address and port of the receiver.
 * Override the default TTL value (which is 1) using the given ttl parameter.
 *
 * @param[in] recvAddr     IP address of the receiver.
 * @param[in] port         Port number of the receiver.
 * @param[in] newTTL       Time to live.
 */
UdpSend::UdpSend(const std::string& recvaddr, unsigned short recvport,
        unsigned char newTTL)
    : ttl(newTTL)
{
    UdpSend(recvaddr, recvport);
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
 * Connect to the UDP socket.
 *
 * @param[in] none
 * @throw  runtime_error  if socket creation fails.
 */
void UdpSend::Init()
{
    /** create a UDP datagram socket. */
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::runtime_error("UdpSend::UdpSend() create socket error");
    /** clear struct recv_addr. */
    (void) memset(&recv_addr, 0, sizeof(recv_addr));
    /** set connection type to IPv4 */
    recv_addr.sin_family = AF_INET;
    /** set the address to the receiver address passed to the constructor */
    recv_addr.sin_addr.s_addr =inet_addr(recvAddr.c_str());
    /** set the port number to the port number passed to the constructor */
    recv_addr.sin_port = htons(recvPort);
    connect(sock_fd, (struct sockaddr *) &recv_addr, sizeof(recv_addr));
    setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
}


/**
 * Send a piece of memory data given by the buff pointer and len length to the
 * already set destination which is identified by a socket file descriptor.
 *
 * @param[in] *buff         a constant void type pointer that points to where
 *                          the piece of memory data to be sent lies.
 * @param[in] len           length of that piece of memory data.
 */
ssize_t UdpSend::SendTo(const void* buff, size_t len)
{
    return send(sock_fd, buff, len, 0);
}


/**
 * Gather-send a VCMTP packet.
 *
 * @param[in] iovec  First I/O vector.
 * @param[in] nvec   Number of I/O vectors.
 */
int UdpSend::SendTo(
        const struct iovec* const iovec,
        const int                 nvec)
{
    return writev(sock_fd, iovec, nvec);
}


/**
 * SendData() sends the packet content separated in two different physical
 * locations, which is put together into a io vector structure, to the
 * destination identified by a socket file descriptor.
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
    int ret;
    /** vector including the two memory locations */
    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    /** call the gathered writing system call to avoid multiple copies */
    ret = writev(sock_fd, iov, 2);
    return ret;
}
