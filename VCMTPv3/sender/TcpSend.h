#ifndef TCPSEND_H_
#define TCPSEND_H_

#include <string>
#include <list>
#include "vcmtpBase.h"
#include <pthread.h>

using namespace std;

class TcpSend
{
    public:
        TcpSend(string tcpAddr, unsigned short tcpPort = 0);
        ~TcpSend();
        int acceptConn();
        const list<int>& getConnSockList();
        unsigned short getPortNum();
        int readSock(int retxsockfd, char* pktBuf, int bufSize);
        int parseHeader(int retxsockfd, VcmtpHeader* recvheader);
        int send(int retxsockfd, VcmtpHeader* sendheader, char* payload, size_t paylen);
    private:
        int sockfd;
        struct sockaddr_in servAddr;
        //fd_set master_read_fds;
        list<int> connSockList;
        pthread_mutex_t sockListMutex; /*!< protect operation on shared sockList */
};

#endif /* TCPSEND_H_ */
