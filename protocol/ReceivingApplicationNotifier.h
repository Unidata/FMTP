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
 * This abstract base class notifies a receiving application about files.
 */
class ReceivingApplicationNotifier {
public:
    virtual ~ReceivingApplicationNotifier();

    /**
     * Notifies the receiving application about the beginning of a file.
     *
     * @retval  true    If and only if the application wants the file.
     */
    virtual bool notify_of_bof();

    /**
     * Notifies the receiving application about the complete reception of a
     * file.
     */
    virtual void notify_of_eof();
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
