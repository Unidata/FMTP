/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      Tcp.h
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
 * @brief     Define the interfaces of Tcp class.
 *
 * Base class for the vcmtpRecvv3 and vcmtpSendv3 classes. It handles
 * TCP connections.
 */

#ifndef VCMTP_TCP_H_
#define VCMTP_TCP_H_

#include <sys/types.h>

class TcpBase
{
public:
protected:
    TcpBase();
    ~TcpBase();

    /**
     * Attempts to read a given number of bytes from a given streaming socket.
     * Returns when that number is read or the end-of-file is encountered.
     *
     * @param[in] sock     The streaming socket.
     * @param[in] buf      Pointer to a buffer.
     * @param[in] nbytes   Number of bytes to attempt to read.
     * @return             Number of bytes read. If less than `nbytes` and
     *                     `nbytes > 0`, then EOF was encountered.
     * @throws std::system_error  if an error is encountered reading from the
     *                            socket.
     */
   size_t recvall(const int sock, void* const buf, const size_t len);

    /**
     * Attempts to read a given number of bytes. Returns when that number is read
     * or the end-of-file is encountered.
     *
     * @param[in] buf      Pointer to a buffer.
     * @param[in] nbytes   Number of bytes to attempt to read.
     * @return             Number of bytes read. If less than `nbytes` and
     *                     `nbytes > 0`, then EOF was encountered.
     * @throws std::system_error  if an error is encountered reading from the
     *                            socket.
     */
   size_t recvall(void* const buf, const size_t len);

    /**
     * Writes a given number of bytes to a given streaming socket. Returns when that
     * number is written or an error occurs.
     *
     * @param[in] sock    The streaming socket.
     * @param[in] buf     Pointer to a buffer.
     * @param[in] nbytes  Number of bytes to write.
     * @throws std::system_error  if an error is encountered writing to the
     *                            socket.
     */
    void sendall(const int sock, void* const buf, size_t nbytes);

    /**
     * Writes a given number of bytes. Returns when that number is written or an
     * error occurs.
     *
     * @param[in] buf     Pointer to a buffer.
     * @param[in] nbytes  Number of bytes to write.
     * @throws std::system_error  if an error is encountered writing to the
     *                            socket.
     */
    void sendall(void* const buf, const size_t nbytes);

    int                     sockfd;
};

#endif /* VCMTP_TCP_H_ */
