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
	faVCMTPSender(u_int64_t id,const char* recvAddr,const ushort recvPort);
	virtual ~faVCMTPSender();
	void SendBOFMessage(uint64_t dataLength, const char* fileName);
	void SendBOMDMessage(uint64_t prodSize, char* prodName,int sizeOfProdName);
	void sendFile(uint64_t dataLength, const char* fileName);
	void sendMemoryData(void* data, uint64_t prodSize, string &prodName);
	void DoMemoryTransfer(void* data, size_t length,u_int32_t start_seq_num);
	void sendEOMDMessage();
private:
	UdpComm* updSocket;
	uint64_t prodId;

};

#endif /* faVCMTPSender_H_ */

