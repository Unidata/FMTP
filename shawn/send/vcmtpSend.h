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
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <netdb.h>
#include <stdio.h>

using namespace std;

class vcmtpSend : public vcmtpBase {
public:
    vcmtpSend(uint64_t id,
              const char* recvAddr,
              const unsigned short recvPort);
    ~vcmtpSend();
    void SendBOMDMsg(uint64_t prodSize, char* prodName, int sizeOfProdName);
    void CreateUDPSocket(const char* recvName, unsigned short int recvPort);
    void sendMemData(void* data, uint64_t prodSize, string &prodName);
    void DoMemTransfer(void* data, size_t length, uint32_t start_seq_num);
    void sendEOMDMsg();
    ssize_t SendTo(const void* buff, size_t len, int flags);
    ssize_t SendData(void* header, const size_t headerLen, void* data, const size_t dataLen);

private:
    int      sock_fd;
    uint64_t prodId;
    struct sockaddr_in recv_addr;
};

#endif /* VCMTPSEND_H_ */
