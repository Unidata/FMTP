#ifndef TCPSEND_H_
#define TCPSEND_H_

#include <string>

using namespace std;

class TcpSend
{
    public:
        TcpSend(string tcpAddr, unsigned short tcpPort);
        ~TcpSend();
        void acceptConn();
        void readSock(char* pktBuf);
    private:
        int sockfd;
        int newsockfd;
        struct sockaddr_in servAddr;
};

#endif /* TCPSEND_H_ */
