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

#include <stdlib.h>
#include <sys/types.h>

class BofResponse {
public:
    BofResponse(bool is_wanted);
    virtual ~BofResponse();
    /**
     * Indicates if the file should be received.
     *
     * @retval true     if and only if the file should be received.
     */
    bool isWanted() {
        return this->is_wanted;
    }
    /**
     * Accepts a portion of the file that's being received.
     *
     * @param[in] offset        The offset, in bytes, from the start of the file
     *                          to the first byte in the given buffer.
     * @param[in] buf           The buffer containing the data to be accepted.
     * @param[in] size          The amount of data in the buffer in bytes.
     * @retval    0             Success.
     * @retval    -1            Failure.
     */
    virtual int accept(off_t offset, unsigned char* buf, size_t size) =0;

private:
    bool is_wanted;
};


/**
 * This class is the BOF response for a memory transfer.
 */
class MemoryBofResponse : public BofResponse {
public:
    MemoryBofResponse(bool is_wanted, unsigned char* buf = 0);
    int accept(off_t offset, unsigned char* buf, size_t size);

private:
    unsigned char*      buf;
};

#endif /* BOFRESPONSE_H_ */
