/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: vcmtpSendv3.h
 * @author: Steven R. Emmerson
 *
 * This file ...
 */

#ifndef VCMTPSENDV3_H_
#define VCMTPSENDV3_H_

#include <sys/types.h>

class vcmtpSendv3 {
public:
    vcmtpSendv3();
    virtual ~vcmtpSendv3();

    void sendProdStream(const char* streamName, uint32_t initProdIndex);

    void startGroup(const char* addr, unsigned port);

    /**
     * Transfers a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     */
    void sendProduct(void* data, size_t dataSize) {
        sendProduct(data, dataSize, 0, 0);
    };

    /**
     * Transfers Application-specific metadata and a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @param[in] metadata  Application-specific metadata to be sent before the
     *                      data. May be 0, in which case no metadata is sent.
     * @param[in] metaSize  Size of the metadata in bytes. Must be less than
     *                      1428. May be 0, in which case no metadata is sent.
     */
    void sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);
};

#endif /* VCMTPSENDV3_H_ */
