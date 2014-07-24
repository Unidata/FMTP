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
 * This base class notifies a receiving application about file events.
 */
class ReceivingApplicationNotifier {
public:
    virtual ~ReceivingApplicationNotifier() {};

    /**
     * Notifies the receiving application about the beginning of a file.
     *
     * @param[in,out] file_entry        The VCMTP entry for the file.
     * @retval        0                 Success.
     * @retval        -1                Failure.
     */
    /*
     * TODO: Have this method return a BofResponse that the VCMTPReceiver will
     * subsequently use to dispose of data packets.
     */
    virtual void notify_of_bof(VcmtpFileEntry& file_entry) const {};

    /**
     * Notifies the receiving application about the complete reception of a
     * file.
     */
    virtual void notify_of_eof(VcmtpFileEntry& file_entry) {};

    /**
     * Notifies the receiving application about a missed file.
     */
    virtual void notify_of_missed_file(VcmtpFileEntry& file_entry) {};
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
