#ifndef TCPRECV_H_
#define TCPRECV_H_

#include <string>

using namespace std;

class TcpRecv
{
    public:
        TcpRecv(string tcpAddr, unsigned short tcpPort);
        ~TcpRecv();
        void sendData();
    private:
        int sockfd;
        struct sockaddr_in servAddr;
};

#endif /* TCPRECV_H_ */
