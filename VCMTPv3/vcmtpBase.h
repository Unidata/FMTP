/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      vcmtpBase.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Oct 7, 2014
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     Define the interfaces of VCMTPv3 basics.
 *
 * Definition of control message definition, header struct and message length.
 */


#ifndef VCMTPBASE_H_
#define VCMTPBASE_H_


#include <stdint.h>
#include <strings.h>
#include <string.h>


/**
 * struct of Vcmtp header
 */
typedef struct VcmtpPacketHeader {
    uint32_t   prodindex;    /*!< identify both file and memdata by prodindex. */
    uint32_t   seqnum;
    uint16_t   payloadlen;
    uint16_t   flags;
} VcmtpHeader;

/**
 * struct of Vcmtp retx-request-message
 */
typedef struct VcmtpRetxReqMessage {
    uint32_t startpos;
    uint16_t length;
} RetxReqMsg;


const int MAX_VCMTP_PACKET_LEN = 1460;
const int VCMTP_HEADER_LEN     = sizeof(VcmtpHeader);
const int VCMTP_DATA_LEN       = MAX_VCMTP_PACKET_LEN - VCMTP_HEADER_LEN;
const int AVAIL_BOP_LEN        = VCMTP_DATA_LEN - 4 - 2; /*!< prosize is 4 bytes, metasize is 2 bytes */
const int RETX_REQ_LEN         = sizeof(RetxReqMsg);

/**
 * structure of Begin-Of-Product message
 */
typedef struct VcmtpBOPMessage {
    uint32_t   prodsize;     /*!< support 4GB maximum */
    uint16_t   metasize;
    char       metadata[AVAIL_BOP_LEN];
} BOPMsg;

const uint16_t VCMTP_BOP       = 0x00000001;
const uint16_t VCMTP_EOP       = 0x00000002;
const uint16_t VCMTP_MEM_DATA  = 0x00000004;
const uint16_t VCMTP_RETX_REQ  = 0x00000008;
const uint16_t VCMTP_RETX_REJ  = 0x00000010;
const uint16_t VCMTP_RETX_END  = 0x00000020;
const uint16_t VCMTP_BOP_REQ   = 0x00000040;


class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();

private:
};

#endif /* VCMTPBASE_H_ */
