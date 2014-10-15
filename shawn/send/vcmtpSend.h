/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpSend.h
 *
 * @history:
 *      Created on : Oct 14, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPSEND_H_
#define VCMTPSEND_H_


#include "vcmtpBase.h"
#include "faUdpComm.h"
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>


class vcmtpSend : public vcmtpBase {
public:
	vcmtpSend(uint64_t id, const char* recvAddr, const unsigned short recvPort);
	~vcmtpSend();
	void SendBOFMessage(uint64_t dataLength, const char* fileName);
	void SendBOMDMessage(uint64_t prodSize, char* prodName,int sizeOfProdName);
	void CreateUDPSocket(const char* recvName,unsigned short int recvPort);
	void sendFile(uint64_t dataLength, const char* fileName);
	void sendMemData(void* data, uint64_t prodSize, string &prodName);
	void DoMemTransfer(void* data, size_t length,uint32_t start_seq_num);
	void sendEOMDMessage();
private:
	UdpComm* updSocket;
	uint64_t prodId;
};

#endif /* VCMTPSEND_H_ */
