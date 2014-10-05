/*
 * vcmtp.h
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */
/*
 * vcmtp.h
 *
 *  Created on: Jun 28, 2011
 *      Author: jie
 */

#ifndef VCMTP_H_
#define VCMTP_H_

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

using namespace std;


typedef struct VcmtpHeader {// all the types were changes to 64 to match the dissertation
	u_int64_t 	file_id; //file identifier
	u_int64_t	seq_number; // block number
	u_int64_t	vcmtp_payload_size; //length of the VCMTP payload
	u_int64_t	flags; //indicates the type of VCMTP packet: (i) data block; (ii) retransmitted data block; (iii) BOF message; (iv) EOF message; (v) BOF-Request message; (vi) Retx-Request message; (vii) End-Of-Retx-Reqs message; and (viii) Retx-Reject message.
} VCMTP_HEADER, *PTR_VCMTP_HEADER;


// VCMTP Header Flags
const u_int64_t VCMTP_BOF = 0x00000001;	// begin of file // this is changed to 8 bytes

// transfer types
const uint8_t  MEMORY_TO_MEMORY = '1';
const uint8_t  DISK_TO_DISK	  = '2';

struct VcmtpSenderMessage {
	uint8_t		transfer_type;// transfer type
	uint64_t 	file_size;// the size of the file, this changed to 8 bytes to match jie dissertation
	char       	file_name[256];//the name of the file
};


const static int VCMTP_PACKET_LEN = 1460; //ETH_FRAME_LEN - ETH_HLEN;
const static int VCMTP_HLEN = sizeof(VCMTP_HEADER);
const static int VCMTP_DATA_LEN = VCMTP_PACKET_LEN - sizeof(VCMTP_HEADER); //ETH_FRAME_LEN - ETH_HLEN - sizeof(VCMTP_HEADER);

#endif /* VCMTP_H_ */
