/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file ReceivingApplicationNotifier.h
 *
 * This file declares the API for classes that notify a receiving application
 * about files.
 *
 * @author: Steven R. Emmerson
 */

#ifndef RECEIVING_APPLICATION_NOTIFIER_H_
#define RECEIVING_APPLICATION_NOTIFIER_H_

/**
 * This base class notifies a receiving application about events.
 */
class ReceivingApplicationNotifier {
public:
    virtual ~ReceivingApplicationNotifier() {};

    /**
     * Notifies the receiving application about the beginning of a product.
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
     * previous product.
     */
    virtual void notify_of_eop() = 0;

    virtual void notify_of_missed_prod(uint32_t prodIndex);
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
