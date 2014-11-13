/*
 * UdpSocket.cpp
 *
 *  Created on: Oct 16, 2014
 *      Author: fatmaal-ali
 */

#include "UdpSocket.h"

/**
 * Set the IP address and port of the receiver and connect to the upd socket.
 *
 * @param[in] mcastAddr     IP address of the receiver.
 * @param[in] port         Port number of the receiver.
 */
UdpSocket::UdpSocket(const char* mcastAddr,ushort port) {
	// create a UDP datagram socket.
	if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cout<<"UdpSocket::UdpSocket error";
	}
	// clear struct mcastAddr.
	bzero(&mcast_addr, sizeof(mcast_addr));
	// set connection type to IPv4
	mcast_addr.sin_family = AF_INET;
	//set the address to the receiver address passed to the constructor
	mcast_addr.sin_addr.s_addr =inet_addr(mcastAddr);
	//set the port number to the port number passed to the constructor
	mcast_addr.sin_port = htons(port);

	connect(sock_fd,(struct sockaddr *) &mcast_addr, sizeof(mcast_addr));

}

UdpSocket::~UdpSocket() {
	// TODO Auto-generated destructor stub
}

ssize_t UdpSocket::SendTo(const void* buff, size_t len)
{
	return sendto(sock_fd, buff, len, 0, (struct sockaddr *) &mcast_addr, sizeof(mcast_addr));
}

size_t UdpSocket::SendData( void*  header, const size_t headerLen,  void*  data, const size_t dataLen)
{
	struct iovec iov[2];//vector including the two memory locations
    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    return writev(sock_fd, iov, 2);
}

