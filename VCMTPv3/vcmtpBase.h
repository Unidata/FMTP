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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief     Define the interfaces of VCMTPv3 basics.
 *
 * Definition of control message definition, header struct and message length.
 */


#ifndef VCMTP_VCMTPV3_VCMTPBASE_H_
#define VCMTP_VCMTPV3_VCMTPBASE_H_


#include <stdint.h>
#include <string.h>
#include <strings.h>

#define PRODNUM 207684
#define PRODNUM 500
#define MODBASE 207684

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
const int AVAIL_BOP_LEN        = VCMTP_DATA_LEN - 4 - 2;
const int RETX_REQ_LEN         = sizeof(RetxReqMsg);


/**
 * structure of Begin-Of-Product message
 */
typedef struct VcmtpBOPMessage {
    uint32_t   prodsize;     /*!< support 4GB maximum */
    uint16_t   metasize;
    char       metadata[AVAIL_BOP_LEN];
    /* Be aware this default constructor could implicitly create a new BOP */
    VcmtpBOPMessage() : prodsize(0), metasize(0), metadata() {}
} BOPMsg;


const uint16_t VCMTP_BOP       = 0x0001;
const uint16_t VCMTP_EOP       = 0x0002;
const uint16_t VCMTP_MEM_DATA  = 0x0004;
const uint16_t VCMTP_RETX_REQ  = 0x0008;
const uint16_t VCMTP_RETX_REJ  = 0x0010;
const uint16_t VCMTP_RETX_END  = 0x0020;
const uint16_t VCMTP_RETX_DATA = 0x0040;
const uint16_t VCMTP_BOP_REQ   = 0x0080;
const uint16_t VCMTP_RETX_BOP  = 0x0100;
const uint16_t VCMTP_EOP_REQ   = 0x0200;
const uint16_t VCMTP_RETX_EOP  = 0x0400;


/** For communication between mcast thread and retx thread */
const int MISSING_BOP  = 1;
const int MISSING_DATA = 2;
const int MISSING_EOP  = 3;
const int SHUTDOWN     = 4;
typedef struct recvInternalRetxReqMessage {
    int reqtype;
    uint32_t prodindex;
    uint32_t seqnum;
    uint16_t payloadlen;
} INLReqMsg;


/** a structure defining parameters for each product, prodindex : sleeptime */
typedef struct timerParameter {
    uint32_t prodindex;
    double   seconds;
} timerParam;


class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();

private:
};


#endif /* VCMTP_VCMTPV3_VCMTPBASE_H_ */
