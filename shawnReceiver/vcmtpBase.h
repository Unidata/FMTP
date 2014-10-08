/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpBase.h
 *
 * @history:
 *      Created on : Oct 7, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPBASE_H_
#define VCMTPBASE_H_

#include <stdint.h>

#define VCMTP_PACKET_LEN 1460
#define VCMTP_HEADER_LEN sizeof(VcmtpHeader)


const uint64_t VCMTP_DATA               = 0x00000000;
const uint64_t VCMTP_BOF                = 0x00000001;
const uint64_t VCMTP_EOF                = 0x00000002;
const uint64_t VCMTP_BOMD               = 0x00000004;
const uint64_t VCMTP_EOMD               = 0x00000008;


typedef struct VcmtpPacketHeader {
    uint64_t   fileid;
    uint64_t   seqnum;
    uint64_t   payloadlen;
    uint64_t   flags;
} VcmtpHeader;


class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();

private:
};

#endif /* VCMTPBASE_H_ */
