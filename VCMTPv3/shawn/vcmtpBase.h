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


typedef struct VcmtpPacketHeader {
    uint64_t   indexid;    // identify both file and memdata by this indexid.
    uint64_t   seqnum;
    uint64_t   payloadlen;
    uint64_t   flags;
} VcmtpHeader;

const int MAX_VCMTP_PACKET_LEN = 1460;
const int VCMTP_HEADER_LEN = sizeof(VcmtpHeader);
const int VCMTP_DATA_LEN   = MAX_VCMTP_PACKET_LEN - VCMTP_HEADER_LEN;

typedef struct VcmtpBOPMessage {
    uint32_t   prodsize;
    uint16_t   metasize;
    char       metadata[VCMTP_DATA_LEN-8-2];
} BOPMsg;

const uint64_t VCMTP_BOP      = 0x00000001;
const uint64_t VCMTP_EOP      = 0x00000002;
const uint64_t VCMTP_MEM_DATA  = 0x00000004;


class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();

private:
};

#endif /* VCMTPBASE_H_ */
