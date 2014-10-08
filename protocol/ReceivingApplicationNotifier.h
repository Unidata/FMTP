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

#include "vcmtp.h"
#include "VcmtpFileEntry.h"

#include <memory>

/**
 * This base class notifies a receiving application about events.
 */
class ReceivingApplicationNotifier {
public:
    virtual ~ReceivingApplicationNotifier() {};

    /**
     * Notifies the receiving application about the beginning of a file
     * transfer.
     *
     * @param[in,out] file_entry        The VCMTP entry for the file.
     * @retval        0                 Success.
     * @retval        -1                Failure.
     */
    virtual void notify_of_bof(VcmtpMessageInfo& info) = 0;

    /**
     * Notifies the receiving application about the beginning of a memory data
     * transfer.
     *
     * @param[in,out] file_entry        The VCMTP entry for the file.
     * @retval        0                 Success.
     * @retval        -1                Failure.
     */
    virtual void notify_of_bomd(VcmtpMessageInfo& info) = 0;

    /**
     * Notifies the receiving application about the complete reception of a
     * file.
     */
    virtual void notify_of_eof(VcmtpMessageInfo& info) = 0;

    /**
     * Notifies the receiving application about the complete reception of
     * memory data.
     */
    virtual void notify_of_eomd(VcmtpMessageInfo& info) = 0;

    /**
     * Notifies the receiving application about a missed data-product.
     */
    virtual void notify_of_missed_product(uint32_t prodId) = 0;
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
