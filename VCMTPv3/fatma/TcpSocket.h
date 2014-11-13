/*
 * TcpSocket.h
 *
 *  Created on: Oct 24, 2014
 *      Author: fatmaal-ali
 */

#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <list>
#include <unistd.h>
#include <sys/uio.h>
using namespace std;

class vcmtpSendv3;

class TcpSocket {
public:
	TcpSocket(const char*  tcpAddr, const ushort tcpPort, vcmtpSendv3* Sender);
	 ~TcpSocket();
	void Accept();
	void AcceptClients();
	static void* StartCoordinatorThread(void* ptr);
	int Receive(int sock_fd, void* buffer, size_t length);
	size_t SendData( void*  header, const size_t headerLen,  void*  data, const size_t dataLen, int sock_fd);
	ushort getTcpPort();
	int getNumOfReceivers();




private:
	struct sockaddr_in	server_addr;
	int server_sock;
	pthread_t coordinator_thread;
	vcmtpSendv3* sender;
	list<int> 	conn_sock_list;
	pthread_mutex_t sock_list_mutex;
	int         num_receivers;

};

#endif /* TCPSOCKET_H_ */
