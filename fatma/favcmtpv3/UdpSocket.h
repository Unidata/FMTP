/*
 * UdpSocket.h
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#ifndef UDPSOCKET_H_
#define UDPSOCKET_H_


#include "vcmtpBase.h"
#include <sys/socket.h>
#include<iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>


using namespace std;

class UdpSocket {
public:
	UdpSocket(const char* recvAddr,ushort port);
	virtual ~UdpSocket();
	ssize_t SendTo(const void* buff, size_t len);
	size_t SendData( void*  header,const size_t headerLen,  void*  data, const size_t dataLen);

private:
	int sock_fd;
	struct sockaddr_in recv_addr;
};



#endif /* UDPSOCKET_H_ */
