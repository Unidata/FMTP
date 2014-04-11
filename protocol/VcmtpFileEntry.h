/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 * @file VcmtpFileEntry.h
 *
 * This file declares a class that represents a file that's being received by
 * the VCMTP layer. Instances contain metadata about their file.
 *
 * @author: Steven R. Emmerson
 */

#ifndef FILEENTRY_H_
#define FILEENTRY_H_

#include "BofResponse.h"
#include "vcmtp.h"

#include <memory>

class VcmtpFileEntry {
public:
    VcmtpFileEntry(const struct VcmtpSenderMessage& msg)
        : msg(msg), bofResponse(false) {}
    ~VcmtpFileEntry();
    /**
     * Returns the size of the file in bytes.
     *
     * @return the size of the file in bytes.
     */
    size_t getSize() const {
        return msg.data_len;
    }
    /**
     * Returns the name of the file.
     *
     * @return the name of the file.
     */
    const char* getName() const {
        return msg.text;
    }
    /**
     * Sets the response to a beginning-of-file notification to ignore the file.
     */
    void setBofResponseToIgnore() {
        bofResponse = BofResponse::getIgnore();
    }
    /**
     * Sets the response from the receiving application to the beginning-of-file
     * notification by the VCMTP layer.
     *
     * @param[in] bofResponse  The receiving application's response to the BOF
     *                         notification by the VCMTP layer.
     */
    void setBofResponse(const BofResponse& bofResponse) {
        this->bofResponse = bofResponse;
    }
    /**
     * Indicates if the file transfer mode is to memory.
     *
     * @retval true   The file transfer mode is to memory.
     * @retval false  The file transfer mode is not to memory.
     */
    bool isMemoryTransfer() const {
        return msg.msg_type == MEMORY_TRANSFER_START ||
               msg.msg_type == TCP_MEMORY_TRANSFER_START;
    }
    /**
     * Indicates if the file should be received.
     *
     * @retval true     if and only if the file should be received.
     */
    bool isWanted() const {
        return bofResponse.isWanted();
    }
    /**
     * Disposes of a portion of the file that's being received.
     *
     * @param[in] offset        The offset, in bytes, from the start of the file
     *                          to the first byte in the given buffer.
     * @param[in] buf           The buffer containing the data to be accepted.
     * @param[in] size          The amount of data in the buffer in bytes.
     * @retval    0             Success.
     * @retval    -1            Failure.
     */
    void dispose(off_t offset, unsigned char* buf, size_t size) const {
        return bofResponse.dispose(offset, buf, size);
    }

private:
    const struct VcmtpSenderMessage     msg;
    BofResponse                         bofResponse;
};

#endif /* FILEENTRY_H_ */
