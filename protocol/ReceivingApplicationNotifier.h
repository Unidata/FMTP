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

#include <VCMTPReceiver.h>

/**
 * This class is the abstract base class for notify a receiving application
 * about files. It abstracts this functionality so that a VCMTP receiver doesn't
 * have to worry about how it's done.
 */
class ReceivingApplicationNotifier {
public:
    ReceivingApplicationNotifier();
    virtual ~ReceivingApplicationNotifier();

    /**
     * Notify the receiving application about the beginning of a file.
     *
     * @retval  true    If and only if the file should be received.
     */
    virtual bool notify_of_bof();

    /**
     * Notify the receiving application about the complete reception of a file.
     */
    virtual void notify_of_eof();
};


/**
 * This class notifies the receiving application on a per-file basis.
 */
class PerFileNotifier: public ReceivingApplicationNotifier {
public:
   static PerFileNotifier& get_instance(
        VCMTP_BOF_Function&             bof_func,
        VCMTP_Recv_Complete_Function&   eof_func);

private:
    /**
     * This class was not designed for inheritance.
     * @param   bof_func        The function to call when a beginning-of-file
     *                          has been seen.
     * @param   eof_func        The function to call when a file has been
     *                          completely received.
     */
    PerFileNotifier(
        VCMTP_BOF_Function&             bof_func,
        VCMTP_Recv_Complete_Function&   eof_func);

    VCMTP_BOF_Function&                 bof_func;
    VCMTP_Recv_Complete_Function&       eof_func;
};


/**
 * This class notifies the receiving application via the notification queue of
 * the VCMTP receiver.
 */
class BatchedNotifier: public ReceivingApplicationNotifier  {
public:

private:
    /**
     * This class was not designed for inheritance.
     * @param   receiver        The VCMTP receiver whose notification queue to
     *                          use.
     */
    BatchedNotifier(VCMTPReceiver& receiver);

    VCMTPReceiver&      receiver;
};

#endif /* RECEIVING_APPLICATION_NOTIFIER_H_ */
