/*
 * VCMTPSender.h
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */

#ifndef VCMTPSENDER_H_
#define VCMTPSENDER_H_


#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include "UdpComm.h"
#include "vcmtp.h"
//#include "VCMTPSenderMetaData.h"


class VCMTPSender {
public:
	VCMTPSender(u_int32_t id);
	virtual ~VCMTPSender();
	void SendMemoryData(void* data,uint64_t dataLength, const char* fileName);
	void CreateUPDSocket(const char* recvName,unsigned short int recvPort);
	void error(const char *msg);
	//int getFileId();
	//void setFileId(int fileId);

//private:
	UdpComm* updSocket;
	u_int32_t      fileId;
};

#endif /* VCMTPSENDER_H_ */
