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

#include <stdexcept>
#include <string>
#include <string.h>

/**
 * Constructs from a memory buffer. The method \c isWanted() of the resulting
 * instance file will return @code{buf != 0}.
 *
 * @param[in] buf       The buffer into which to copy the received data or 0.
 */
MemoryBofResponse::MemoryBofResponse(
    unsigned char*      buf)
:
    BofResponse(buf != 0),
    buf(buf)
{}

int MemoryBofResponse::dispose(
    const off_t                 offset,
    unsigned char* const        buf,
    const size_t                size) const
{
    memcpy(this->buf + offset, buf, size);
    return 0;
}
