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
    //servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_addr.s_addr = inet_addr(tcpAddr.c_str());
    /** If tcpPort = 0, OS will automatically choose an available port number. */
    servAddr.sin_port = htons(tcpPort);
    if(bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        std::cout << "TcpSend::TcpSend() error binding socket" << std::endl;
    listen(sockfd, 5);
}


TcpSend::~TcpSend()
{
    // need modified here to close all sockets
    close(newsockfd);
    close(sockfd);
}


void TcpSend::acceptConn()
{
    struct sockaddr_in cliAddr;
    socklen_t clilen = sizeof(cliAddr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cliAddr, &clilen);
    if(newsockfd < 0)
        //std::cout << "TcpSend::accept() error accepting new connection" << std::endl;
        perror("TcpSend::readSock() error reading from socket");
}


void TcpSend::readSock(char* pktBuf)
{
    if(read(newsockfd, pktBuf, VCMTP_HEADER_LEN) < 0)
        perror("TcpSend::readSock() error reading from socket");
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
