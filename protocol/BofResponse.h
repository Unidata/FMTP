/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file BofResponse.h
 *
 * This file declares the response by a receiving application to a
 * beginning-of-file notification from the VCMTP layer.
 *
 * @author: Steven R. Emmerson
 */

#ifndef BOFRESPONSE_H_
#define BOFRESPONSE_H_

#include "vcmtp.h"

#include <stdlib.h>
#include <sys/types.h>

class BofResponse {
public:
    BofResponse(bool is_wanted) : is_wanted(is_wanted) {};
    virtual ~BofResponse() {};
    /**
     * Indicates if the file should be received.
     * @retval true     if and only if the file should be received.
     */
    bool isWanted() const {
        return this->is_wanted;
    }
    /**
     * Disposes of a portion of the file that's being received.
     *
     * This default method attempts to read the given number of bytes from the
     * socket but does nothing with them.
     * @param[in] sock               The socket on which to receive the data.
     * @param[in] offset             The offset, in bytes, from the start of the
     *                               file to the first byte read from the
     *                               socket.
     * @param[in] nbytes             The amount of data to receive in bytes.
     * @retval    0                  The socket is closed.
     * @return                       The number of bytes read from the socket.
     * @throws    invalid_argument   If @code{offset < 0 || nbytes > VCMTP_PACKET_LEN}.
     * @throws    runtime_exception  If an I/O error occurs on the socket.
     */
    virtual size_t dispose(int sock, off_t offset, size_t nbytes) const;
    /**
     * Returns a beginning-of-file response that will cause the file to be
     * ignored.
     * @return A BOF response that will cause the file to be ignored.
     */
    static const BofResponse& getIgnore(void);

private:
    bool                 is_wanted;
    /**
     * Buffer for ignoring received data.
     */
    static unsigned char ignoreBuf[VCMTP_PACKET_LEN];
};


/**
 * This class is the BOF response for a transfer to memory.
 */
class MemoryBofResponse : public BofResponse {
public:
    MemoryBofResponse(unsigned char* buf, size_t size);
    size_t dispose(int sock, off_t offset, size_t size) const;

private:
    unsigned char*      buf;
    size_t              size;
};

#endif /* BOFRESPONSE_H_ */
