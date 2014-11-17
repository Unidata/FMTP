#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <iostream>
#include <unistd.h>
#include "TcpSend.h"

using namespace std;

TcpSend::TcpSend(string tcpAddr, unsigned short tcpPort)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        std::cout << "TcpSend::TcpSend() error creating socket" << std::endl;
    bzero((char *) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    //servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_addr.s_addr = inet_addr(tcpAddr.c_str());
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
    char buffer[256];
    bzero(buffer,256);

    struct sockaddr_in cliAddr;
    socklen_t clilen = sizeof(cliAddr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cliAddr, &clilen);
    if(newsockfd < 0)
        std::cout << "TcpSend::accept() error accepting new connection" << std::endl;

    int n = read(newsockfd, buffer, 255);
    std::cout << "Received msg: " << buffer << std::endl;
}
