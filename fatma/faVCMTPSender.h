/*
 * fafaVCMTPSender.h
 *
 *  Created on: Oct 4, 2014
 *      Author: fatmaal-ali
 */
/*
 * faVCMTPSender.h
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */

#ifndef faVCMTPSender_H_
#define faVCMTPSender_H_


#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include "faUdpComm.h"
#include "vcmtpBase.h"


class faVCMTPSender {
public:
	faVCMTPSender(u_int64_t id);
	virtual ~faVCMTPSender();
	void SendBOFMessage(uint64_t dataLength, const char* fileName);
	void SendBOMDMessage(uint64_t fileSize, char* prodId,int sizeOfProdId);
	void CreateUPDSocket(const char* recvName,unsigned short int recvPort);
	void sendFile(uint64_t dataLength, const char* fileName);
	void sendMemoryData(void* data, uint64_t fileSize, string &prodId);
	void DoMemoryTransfer(void* data, size_t length,u_int32_t start_seq_num);
	
private:
	UdpComm* updSocket;
	uint64_t fileId;
	char  fileName[256];
	uint64_t fileSize;
	uint64_t seq_num;
	char prodId[256];
};

#endif /* faVCMTPSender_H_ */

