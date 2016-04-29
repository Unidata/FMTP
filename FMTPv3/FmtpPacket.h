/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: FmtpPacket.h
 * @author: Steven R. Emmerson
 *
 * This file declares a FMTP packet.
 */

#ifndef FMTP_PACKET_H_
#define FMTP_PACKET_H_

#include "fmtpBase.h"

class FmtpPacket {
public:
    FmtpPacket(const int mcastSock);
    ~FmtpPacket();

private:
    /**
     * Decodes a FMTP packet.
     *
     * @param[in]  packet         The raw packet.
     * @param[in]  nbytes         The size of the raw packet in bytes.
     * @throw std::runtime_error  if the packet is too small.
     * @throw std::runtime_error  if the packet has an invalid payload length.
     */
    void FmtpPacket::decode(
            const size_t nbytes);

    struct {
        FmtpHeader header;
        char        payload[1];
    } decoded;
    char    encoded[MAX_FMTP_PACKET_LEN];
};

#endif /* FMTP_PACKET_H_ */
