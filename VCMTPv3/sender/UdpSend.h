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
 * @brief     Define the interfaces and structures of sender side UDP layer
 *            abstracted funtions.
 *
 * The UdpSend class includes a set of transmission functions, which are
 * basically the encapsulation of udp system calls themselves. This abstracted
 * new layer acts as the sender side transmission library.
 */



#ifndef VCMTP_SENDER_UDPSOCKET_H_
#define VCMTP_SENDER_UDPSOCKET_H_


#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>


class UdpSend {
public:
    UdpSend(const char* recvAddr, unsigned short port);
    UdpSend(const char* recvAddr, unsigned short port, unsigned char ttl);
    ~UdpSend();
    /**
     * SendTo() sends a piece of message to a destination identified by a
     * socket file descriptor.
     */
    ssize_t SendTo(const void* buff, size_t len);
    /**
     * Gather send a VCMTP packet.
     *
     * @param[in] iovec  First I/O vector.
     * @param[in] nvec   Number of I/O vectors.
     */
    int SendTo(
            const struct iovec* const iovec,
            const int                 nvec);
    /**
     * SendData() sends the packet content separated in two different physical
     * locations, which is put together into a io vector structure, to the
     * destination identified by a socket file descriptor.
     */
    ssize_t SendData(void* header, const size_t headerLen, void* data,
                     const size_t dataLen);

private:
    int sock_fd;
    struct sockaddr_in recv_addr;
};



#endif /* VCMTP_SENDER_UDPSOCKET_H_ */

