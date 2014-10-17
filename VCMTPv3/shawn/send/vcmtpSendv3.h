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
    /**
     * Constructs an instance.
     *
     * @param[in] addr           Internet address in dotted decimal format.
     * @param[in] port           Port number.
     */
    vcmtpSendv3(const char* addr, unsigned port);
    /**
     * Constructs an instance.
     *
     * @param[in] addr           Internet address in dotted decimal format.
     * @param[in] port           Port number.
     * @param[in] initProdIndex  Initial value for the product index. Supports
     *                           VCMTP handling a stream of products and
     *                           communication with the application layer.
     */
    vcmtpSendv3(const char* addr, unsigned port, uint32_t initProdIndex);

    ~vcmtpSendv3();

    /**
     * Transfers a contiguous block of memory.
     *
     * @param[in] data      Memory data to be sent.
     * @param[in] dataSize  Size of the memory data in bytes.
     * @return              Index of the product.
     */
    uint32_t sendProduct(void* data, size_t dataSize) {
        return sendProduct(data, dataSize, 0, 0);
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
     * @return              Index of the product.
     */
    uint32_t sendProduct(void* data, size_t dataSize, void* metadata,
            unsigned metaSize);
};

#endif /* VCMTPSENDV3_H_ */
