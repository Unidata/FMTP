/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file BofResponse.cpp
 *
 * This file defines the response by a receiving application to a
 * beginning-of-file notification from the VCMTP layer.
 *
 * @author: Steven R. Emmerson
 */

#include "BofResponse.h"
#include "vcmtp.h"

#include <errno.h>
#include <stdexcept>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

size_t BofResponse::dispose(
    const int    sock,
    const off_t  offset,
    const size_t nbytes) const
{
    ssize_t len;
    /*
     * Buffer for ignoring data.
     */
    static unsigned char ignoreBuf[VCMTP_PACKET_LEN];

    if (offset < 0)
        throw std::invalid_argument("Offset argument is negative");

    if (nbytes > sizeof(ignoreBuf))
        throw std::invalid_argument("Number of bytes argument is too large");

    len = recv(sock, ignoreBuf, nbytes, MSG_WAITALL);

    if (len < 0)
        throw std::runtime_error(std::string("Couldn't read from socket: ") +
                strerror(errno));

    return len;
};

/**
 * Returns a beginning-of-file response that will cause the file to be ignored.
 * @return A BOF response that will cause the file to be ignored.
 */
const BofResponse& BofResponse::getIgnore(void)
{
    static BofResponse ignore(false);
    return ignore;
}

/**
 * Constructs from a memory buffer.
 *
 * @param[in] buf                       The buffer into which to copy the
 *                                      received data.
 * @param[in] size                      The size of the buffer in bytes.
 * @throws    std::invalid_argument     if @code{buf == 0}.
 */
MemoryBofResponse::MemoryBofResponse(
    unsigned char*      buf,
    const size_t        size)
:
    BofResponse(true),
    buf(buf),
    size(size)
{
    if (0 == buf)
        throw std::invalid_argument(std::string("NULL buffer argument"));
}

size_t MemoryBofResponse::dispose(
    const int           sock,
    const off_t         offset,
    const size_t        nbytes) const
{
    ssize_t len;

    if (offset < 0)
        throw std::invalid_argument("Offset argument is negative");

    if (offset+nbytes > size)
        throw std::invalid_argument("Number of bytes argument is too large");

    len = recv(sock, buf+offset, nbytes, MSG_WAITALL);

    if (len < 0)
        throw std::runtime_error(std::string("Couldn't read from socket: ") +
                strerror(errno));

    return len;
}
