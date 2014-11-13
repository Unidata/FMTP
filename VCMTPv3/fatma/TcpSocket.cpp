/*
 * TcpSocket.cpp
 *
 *  Created on: Oct 24, 2014
 *      Author: fatmaal-ali
 */

#include "TcpSocket.h"


// if tcpPort is 0, it means the OS will choose a port number
TcpSocket::	TcpSocket(const char*  tcpAddr, const ushort tcpPort,vcmtpSendv3* Sender)
 {
	coordinator_thread=0;
	num_receivers=0;
	sender=Sender;

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		cout<<"TcpSocket::TcpSocket()::socket() error";

	int optval = 1;
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(tcpAddr);
	server_addr.sin_port = htons(tcpPort);
	//get function for the port number

	while (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		cout << "TcpSocket::TcpSocket()::bind() error" <<endl;
		sleep(10);
	}

	pthread_mutex_init(&sock_list_mutex, 0);

	//run the coordinator thread
	if ((pthread_create(&coordinator_thread, 0, &TcpSocket::StartCoordinatorThread, this)) != 0)
			cout<<"TcpSocket::Start()::pthread_create() error"<<endl;

 }

TcpSocket::~TcpSocket() {
	list<int>::iterator it;
	for (it = conn_sock_list.begin(); it != conn_sock_list.end(); it++) {
		close(*it);
	}
	close(server_sock);

	pthread_mutex_destroy(&sock_list_mutex);
}

ushort TcpSocket::getTcpPort()
{
	return ntohs(server_addr.sin_port);
}

// if the connection is broken we need to check
// Receive data from a given socket
int TcpSocket::Receive(int sock_fd, void* buffer, size_t length) {

	int res = recv(sock_fd, buffer, length, MSG_WAITALL);
	if (errno == EINTR)
		Receive(sock_fd, buffer, length);

	//if the connection is broken
	if (res <= 0) {
		cout<<"TcpSocket::Receive() error"<<endl;
		pthread_mutex_lock(&sock_list_mutex);
		conn_sock_list.remove(sock_fd);
		close(sock_fd);
		cout << "TcpSocket::Receive()::One broken socket deleted: " << sock_fd << endl;
		pthread_mutex_unlock(&sock_list_mutex);
	}
	return res;
}

size_t TcpSocket::SendData( void*  header, const size_t headerLen,  void*  data, const size_t dataLen,int sock_fd)
{
    pthread_mutex_lock(&sock_list_mutex);

	int res;
	struct iovec iov[2];//vector including the two memory locations
    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    res= writev(sock_fd, iov, 2);

    if (res <= 0 || res != headerLen + dataLen) {
		close(sock_fd);
		conn_sock_list.remove(sock_fd);
		num_receivers--;
		cout << "TcpSocket::SelectSend()::One socket deleted: " << sock_fd << endl;
	}
	pthread_mutex_unlock(&sock_list_mutex);

    return res;
}

int TcpSocket::getNumOfReceivers()
{
    pthread_mutex_lock(&sock_list_mutex);
	return num_receivers;
	pthread_mutex_unlock(&sock_list_mutex);
}
//====================== A separate thread that accepts new client requests =======================

 void* TcpSocket::StartCoordinatorThread(void* ptr) {
	((TcpSocket*)ptr)->AcceptClients();
	pthread_exit(NULL);
}


void TcpSocket::AcceptClients() {
	if (listen(server_sock, 200) < 0)// max connection 200
		cout<<"TcpSocket::Listen()::listen() error"<<endl;

	while (true) {
		Accept();
	}
}

void TcpSocket::Accept()
{
	int conn_sock;
	if ( (conn_sock = accept(server_sock, (struct sockaddr*)NULL, NULL))< 0) {
		cout<<"TcpSocket::Accept()::accept() error"<<endl;
	}

	pthread_mutex_lock(&sock_list_mutex);
	num_receivers++;
	conn_sock_list.push_back(conn_sock);
	pthread_mutex_unlock(&sock_list_mutex);

	// start the retransmission thread in the vcmtpSendv3 process
	sender->RunRetransThread(conn_sock);

	//return conn_sock;

}

