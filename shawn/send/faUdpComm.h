/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: UdpComm.h
 *
 * @history:
 *      Created on : Jul 21, 2011
 *      Author     : jie
 *      Modified   : Sep 21, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef UDPCOMM_H_
#define UDPCOMM_H_

#include "vcmtpBase.h"
#include <string>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>


class UdpComm {
public:
    UdpComm(const char* recvAddr,ushort port);
    ~UdpComm();

    void SetSocketBufferSize(size_t size);
    ssize_t SendTo(const void* buff, size_t len, int flags);
    size_t SendData( void*  header,const size_t headerLen,  void*  data, const size_t dataLen);

private:
    int sock_fd;
    struct sockaddr_in recv_addr;
};

#endif /* UDPCOMM_H_ */

