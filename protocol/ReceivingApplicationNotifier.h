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

/**
 * This base class notifies a receiving application about file events.
 */
class ReceivingApplicationNotifier {
public:
    virtual ~ReceivingApplicationNotifier() {};

    /**
     * Notifies the receiving application about the beginning of a file.
     *
     * @retval  true    If and only if the application wants the file.
     */
    virtual bool notify_of_bof(VcmtpSenderMessage& msg) {return false;};

    /**
     * Notifies the receiving application about the complete reception of a
     * file.
     */
    virtual void notify_of_eof(VcmtpSenderMessage& msg) {};

    /**
     * Notifies the receiving application about a missed file.
     */
    virtual void notify_of_missed_file(VcmtpSenderMessage& msg) {};
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
