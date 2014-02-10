/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file ReceivingApplicationNotifier.cpp
 *
 * This file defines the API for classes that notify the receiving application
 * about files.
 *
 * @author: Steven R. Emmerson
 */

#include "VCMTPReceiver.h"
#include "ReceivingApplicationNotifier.h"

PerFileNotifier& PerFileNotifier::get_instance(
        VCMTP_BOF_Function             bof_func,
        VCMTP_Recv_Complete_Function   eof_func) {
    return *new PerFileNotifier(bof_func, eof_func);
}

PerFileNotifier::PerFileNotifier(
        VCMTP_BOF_Function             bof_func,
        VCMTP_Recv_Complete_Function   eof_func)
:   bof_func(bof_func),
    eof_func(eof_func) {
}

bool PerFileNotifier::notify_of_bof() {
    return false;
}

void PerFileNotifier::notify_of_eof() {
}


BatchedNotifier& BatchedNotifier::get_instance(
        VCMTPReceiver&      receiver) {
    return *new BatchedNotifier(receiver);
}

BatchedNotifier::BatchedNotifier(
        VCMTPReceiver& receiver)
:   receiver(receiver) {
}

bool BatchedNotifier::notify_of_bof() {
    return false;
}

void BatchedNotifier::notify_of_eof() {
}
