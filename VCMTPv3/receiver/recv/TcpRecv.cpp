#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include "TcpRecv.h"

using namespace std;

TcpRecv::TcpRecv(string tcpAddr, unsigned short tcpPort)
{
    bzero((char *) &servAddr, sizeof(servAddr));
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

void TcpRecv::sendData()
{
    char buffer[256] = "hello world";
    int n = write(sockfd, buffer, sizeof(buffer));
    if(n < 0)
        std::cout << "TcpRecv::sendRetxReq() error writing socket" << std::endl;
}
