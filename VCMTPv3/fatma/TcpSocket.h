#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_

#include <string>

using namespace std;

class TcpSocket
{
    public:
        TcpSocket(string tcpAddr, unsigned short tcpPort);
        ~TcpSocket();
        void acceptConn();
    private:
        int sockfd;
        int newsockfd;
        struct sockaddr_in servAddr;
};

#endif /* TCPSOCKET_H_ */
