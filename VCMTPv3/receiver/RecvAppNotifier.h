/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file RecvAppNotifier.h
 *
 * This file declares the API for classes that notify a receiving application
 * about files.
 *
 * @author: Steven R. Emmerson
 */

#ifndef VCMTP_RECEIVER_RECVAPPNOTIFIER_H_
#define VCMTP_RECEIVER_RECVAPPNOTIFIER_H_

#include <stdint.h>
#include <sys/types.h>

/**
 * This base class notifies a receiving application about events.
 */
class RecvAppNotifier {
public:
    virtual ~RecvAppNotifier() {};        // definition must exist

    /**
     * Notifies the receiving application about the beginning of a product. This
     * method is thread-safe.
     *
     * @param[in]  prodSize  Size of the product in bytes.
     * @param[in]  metadata  Application-level product metadata.
     * @param[in]  metaSize  Size of the metadata in bytes.
     * @param[out] data      Pointer to where VCMTP should write subsequent data.
     */
    virtual void notify_of_bop(size_t prodSize, void* metadata,
            unsigned metaSize, void** data) = 0;

    /**
     * Notifies the receiving application about the complete reception of the
     * previous product. This method is thread-safe.
     */
    virtual void notify_of_eop() = 0;

    /**
     * Notifies the receiving application about a product that the VCMTP layer
     * missed. This method is thread-safe.
     *
     * @param[in] prodIndex  Index of the missed product.
     */
    virtual void notify_of_missed_prod(uint32_t prodIndex) = 0;
};

#endif /* VCMTP_RECEIVER_RECVAPPNOTIFIER_H_ */
