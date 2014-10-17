/*
 * vcmtpBase.h
 *
 *  Created on: Oct 8, 2014
 *      Author: fatmaal-ali
 */
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
#include <aio.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>

#include <netinet/in.h>
using namespace std;

#ifndef VCMTPBASE_H_
#define VCMTPBASE_H_

#include <stdint.h>



const uint64_t VCMTP_BOF = 0x00000001;
const uint64_t VCMTP_BOMD = 0x00000002;
const uint64_t VCMTP_EOF = 0x00000004;
const uint64_t VCMTP_EOMD = 0x00000008;
const uint64_t VCMTP_FILE_DATA = 0x00000010;
const uint64_t VCMTP_MEM_DATA = 0x00000020;


typedef struct VcmtpPacketHeader {
    uint64_t   fileid;
    uint64_t   seqnum;
    uint64_t   payloadlen;
    uint64_t   flags;
} VcmtpHeader;

const static int VCMTP_PACKET_LEN = 1460;
const static int VCMTP_HEADER_LEN = sizeof(VcmtpHeader);
const static int VCMTP_DATA_LEN = VCMTP_PACKET_LEN - VCMTP_HEADER_LEN;

class vcmtpBase {
public:
    vcmtpBase();
    ~vcmtpBase();

private:
};

#endif /* VCMTPBASE_H_ */

