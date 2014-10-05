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
#include <arpa/inet.h>
#include <endian.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include "faUdpComm.h"
#include "favcmtp.h"


class faVCMTPSender {
public:
	faVCMTPSender(uint64_t id);
	virtual ~faVCMTPSender();
	void SendBOFMessage(uint64_t dataLength, const char* fileName);
	void CreateUPDSocket(const char* recvName,unsigned short int recvPort);
	void error(const char *msg);

private:
	UdpComm* updSocket;
	uint64_t      fileId;
};

#endif /* faVCMTPSender_H_ */
