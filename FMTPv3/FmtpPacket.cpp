/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Packet.cpp
 * @author: Steven R. Emmerson
 *
 * This file implements a FMTP packet.
 */

#include "config.h"

#include "FmtpPacket.h"

#include <errno.h>
#include <sys/socket.h>

/**
 * Constructs from a multicast socket.
 *
 * @param[in] mcastSock      The multicast socket from which to decode the
 *                           packet.
 * @throw std::system_error  if an I/O error occurs.
 */
FmtpPacket::FmtpPacket(
        const int mcastSock)
{
    ssize_t nbytes = recv(mcastSock, packet.encoded, sizeof(packet.encoded), 0);

    if (nbytes < 0)
        throw std::system_error(errno, std::system_category(),
                "Couldn't read from multicast socket");

    decode(nbytes);
}

FmtpPacket::~FmtpPacket() {
    // TODO Auto-generated destructor stub
}

/**
 * Decodes a FMTP packet.
 *
 * @param[in]  nbytes         The size of the raw packet in bytes.
 * @throw std::runtime_error  if the packet is too small.
 * @throw std::runtime_error  if the packet has in invalid payload length.
 */
void FmtpPacket::decode(
        const size_t nbytes)
{
    if (nbytes < FMTP_HEADER_LEN)
        throw std::runtime_error("fmtpRecvv3::decodeHeader(): Packet is too small");

    packet.decoded.header.prodindex  = ntohl(packet.decoded.header.prodindex);
    packet.decoded.header.seqnum     = ntohl(packet.decoded.header.seqnum);
    packet.decoded.header.payloadlen = ntohs(packet.decoded.header.payloadlen);
    packet.decoded.header.flags      = ntohs(packet.decoded.header.flags);

    if (packet.decoded.header.payloadlen != nbytes - FMTP_HEADER_LEN)
        throw std::runtime_error("fmtpRecvv3::decodeHeader(): Invalid payload length");
}
