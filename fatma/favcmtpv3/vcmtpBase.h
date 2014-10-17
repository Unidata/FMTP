/*
 * vcmtpBase.h
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#ifndef VCMTPBASE_H_
#define VCMTPBASE_H_


#include <stdint.h>
#include <strings.h>
#include <string.h>


typedef struct VcmtpPacketHeader {
    uint32_t   indexid;    // identify both file and memdata by this indexid.
    uint32_t   seqnum;
    uint16_t   payloadlen;
    uint16_t   flags;
} VcmtpHeader;


const int VCMTP_PACKET_LEN = 1460;
const int VCMTP_HEADER_LEN = sizeof(VcmtpHeader);
const int VCMTP_DATA_LEN   = VCMTP_PACKET_LEN - VCMTP_HEADER_LEN;

typedef struct VcmtpBOPMessage {
    uint32_t   prodsize; //max size is 4GB
    uint16_t   metaSize;
    char       metadata[VCMTP_DATA_LEN-4-2];
} BOPMsg;

const uint64_t VCMTP_BOF       = 0x00000001;
const uint64_t VCMTP_BOP       = 0x00000002;
const uint64_t VCMTP_EOP       = 0x00000004;
const uint64_t VCMTP_EOMD      = 0x00000008;
const uint64_t VCMTP_FILE_DATA = 0x00000010;
const uint64_t VCMTP_MEM_DATA  = 0x00000020;


class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();
	
private:
};

#endif /* VCMTPBASE_H_ */
