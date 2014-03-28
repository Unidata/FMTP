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
 * Returns a beginning-of-file response that causes the file to be ignored
 * by the VCMTP layer.
 *
 * @return A BOF response that causes the file to be ignored.
 */
const BofResponse& BofResponse::getIgnore()
{
    /**
     * This class is the BOF response for ignoring a file.
     */
    class IgnoreBofResponse : public BofResponse {
    public:
       IgnoreBofResponse() : BofResponse(false) {}
       int dispose(off_t offset, unsigned char* buf, size_t size) const {
           return -1;
       }
    };
    static IgnoreBofResponse ignore;
    return ignore;
}

/**
 * Constructs from a memory buffer. The method \c isWanted() of the resulting
 * instance file will return @code{buf != 0}.
 *
 * @param[in] buf                       The buffer into which to copy the
 *                                      received data.
 * @throws    std::invalid_argument     if @code{buf == 0}.
 */
MemoryBofResponse::MemoryBofResponse(
    unsigned char*      buf)
:
    BofResponse(true),
    buf(buf)
{
    if (0 == buf)
        throw std::invalid_argument(std::string("NULL buffer argument"));
}

int MemoryBofResponse::dispose(
    const off_t                 offset,
    unsigned char* const        buf,
    const size_t                size) const
{
    memcpy(this->buf + offset, buf, size);
    return 0;
}
