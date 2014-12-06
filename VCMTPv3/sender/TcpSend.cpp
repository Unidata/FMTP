#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include "TcpSend.h"
#include "vcmtpBase.h"
#include <sys/uio.h>

#define MAX_CONNECTION 5

using namespace std;


/**
 * Contructor for TcpSend class, accepting tcp address and tcp port.
 *
 * @param[in] tcpAddr     tcp server address
 * @param[in] tcpPort     tcp port number (in host order) specified by sending
 * 						  application. (or 0, meaning system will use random
 * 						  available port)
 */
TcpSend::TcpSend(string tcpAddr, unsigned short tcpPort)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        std::cout << "TcpSend::TcpSend() error creating socket" << std::endl;
    bzero((char *) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(tcpAddr.c_str());
    /** If tcpPort = 0, OS will automatically choose an available port number. */
    servAddr.sin_port = htons(tcpPort);
    if(bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        std::cout << "TcpSend::TcpSend() error binding socket" << std::endl;
    /** listen() returns right away, it's non-blocking */
    listen(sockfd, MAX_CONNECTION);
}


TcpSend::~TcpSend()
{
    // need modified here to close all sockets
    close(sockfd);
}


int TcpSend::acceptConn()
{
    struct sockaddr_in cliAddr;
    socklen_t clilen = sizeof(cliAddr);
    //int newsockfd = accept(sockfd, (struct sockaddr *) &cliAddr, &clilen);
    int newsockfd = accept(sockfd, NULL, NULL);
    if(newsockfd < 0)
    	//TODO: should throw an error here and return right away.
        perror("TcpSend::readSock() error reading from socket");

	pthread_mutex_lock(&sockListMutex);
	connSockList.push_back(newsockfd);
	pthread_mutex_unlock(&sockListMutex);

    return newsockfd;
}


const list<int>& TcpSend::getConnSockList()
{
	return connSockList;
}


/** unnecessary function */
int TcpSend::readSock(int retxsockfd, char* pktBuf, int bufSize)
{
    return read(retxsockfd, pktBuf, bufSize);
}


int TcpSend::parseHeader(int retxsockfd, VcmtpHeader* recvheader)
{
	char recvbuf[VCMTP_HEADER_LEN];
    int retval = read(retxsockfd, recvbuf, sizeof(recvbuf));
    if(retval < 0)
    	return retval;

    memcpy(&recvheader->prodindex,  recvbuf,    4);
    memcpy(&recvheader->seqnum,     recvbuf+4,  4);
    memcpy(&recvheader->payloadlen, recvbuf+8,  2);
    memcpy(&recvheader->flags,      recvbuf+10, 2);
    recvheader->prodindex  = ntohl(recvheader->prodindex);
    recvheader->seqnum	   = ntohl(recvheader->seqnum);
    recvheader->payloadlen = ntohs(recvheader->payloadlen);
    recvheader->flags	   = ntohs(recvheader->flags);
    return retval;
}


int TcpSend::send(int retxsockfd, VcmtpHeader* sendheader, char* payload, size_t paylen)
{
	char headbuf[VCMTP_HEADER_LEN];
    struct iovec iov[2];

    memcpy(headbuf,    &sendheader->prodindex, 	4);
    memcpy(headbuf+4,  &sendheader->seqnum, 	4);
    memcpy(headbuf+8,  &sendheader->payloadlen, 2);
    memcpy(headbuf+10, &sendheader->flags, 		2);

    iov[0].iov_base = headbuf;
    iov[0].iov_len  = sizeof(headbuf);
    iov[1].iov_base = payload;
    iov[1].iov_len  = paylen;

    int retval = writev(retxsockfd, iov, 2);
    return retval;
}


/**
 * Return the local port number.
 *
 * @return 	              The local port number in host byte-order.
 * @throws std::system_error  The port number cannot be obtained.
 */
unsigned short TcpSend::getPortNum()
{
    struct sockaddr_in tmpAddr;
    socklen_t          tmpAddrLen = sizeof(tmpAddr);

    if (getsockname(sockfd, (struct sockaddr*)&tmpAddr, &tmpAddrLen) < 0)
        throw std::system_error(errno, system_category(),
                "TcpSend::getPortNum() error getting port number");

    return ntohs(tmpAddr.sin_port);
}
