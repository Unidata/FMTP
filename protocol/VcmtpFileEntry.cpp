/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file FileEntry.cpp
 *
 * This file implements the API for a file that's being received by the VCMTP
 * layer.
 *
 * @author: Steven R. Emmerson
 */

#include "VcmtpFileEntry.h"

static const std::shared_ptr<BofResponse>& VcmtpFileEntry::ignoreBofResponse() {
    static std::shared_ptr<BofResponse> ignore(BofResponse::getIgnore());
    return ignore;
}

