#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/uio.h>
#include "TcpRecv.h"

using namespace std;

TcpRecv::TcpRecv(string tcpAddr, unsigned short tcpPort)
{
    (void) memset((char *) &servAddr, 0, sizeof(servAddr));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        std::cout << "TcpRecv::TcpRecv() error creating socket" << std::endl;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(tcpAddr.c_str());
    servAddr.sin_port = htons(tcpPort);
    if(connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        std::cout << "TcpRecv:TcpRecv() error connecting to sender" << std::endl;
}


TcpRecv::~TcpRecv()
{
    close(sockfd);
}


ssize_t TcpRecv::sendData(char* header, size_t headLen, char* payload, size_t payLen)
{
    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    int retval = writev(sockfd, iov, 2);
    return retval;
}


ssize_t TcpRecv::recvData(char* header, size_t headLen, char* payload, size_t payLen)
{
    struct iovec iov[2];
    iov[0].iov_base = header;
    iov[0].iov_len  = headLen;
    iov[1].iov_base = payload;
    iov[1].iov_len  = payLen;

    int retval = readv(sockfd, iov, 2);
    return retval;
}
