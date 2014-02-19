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
 * Constructs.
 *
 * @param[in] is_wanted              Whether the file should be received by
 *                                   the VCMTP layer.
 * @param[in] buf                    The buffer into which to copy the
 *                                   received data. May not be 0 if \c
 *                                   is_wanted is true.
 * @throws    std::invalid_argument  @code{is_wanted && 0 == buf}.
 */
MemoryBofResponse::MemoryBofResponse(
    const bool                  is_wanted,
    unsigned char* const        buf)
:
    BofResponse(is_wanted),
    buf(buf)
{
    if (is_wanted && 0 == buf)
        throw std::invalid_argument(std::string("NULL buffer argument"));
}

int MemoryBofResponse::accept(
    const off_t                 offset,
    unsigned char* const        buf,
    const size_t                size)
{
    memcpy(this->buf + offset, buf, size);
    return 0;
}
