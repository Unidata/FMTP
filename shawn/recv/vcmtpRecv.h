/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecv.h
 *
 * @history:
 *      Created on : Oct 2, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPRECV_H_
#define VCMTPRECV_H_

#include "vcmtpBase.h"
#include <stdint.h>
#include <string>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

class vcmtpRecv : public vcmtpBase {
public:
    vcmtpRecv(string tcpAddr,
              const unsigned short tcpPort,
              string localAddr,
              const unsigned short localPort);
    ~vcmtpRecv();

    void    Start();
    void    StartReceivingThread();
    void    RunReceivingThread();
    int     udpBindIP2Sock(string senderAddr, const unsigned short port);
    void    McastPacketHandler();
    void    BOFHandler(char* VcmtpPacket);
    void    BOMDHandler(char* VcmtpPacket);
    void    EOFHandler(char* VcmtpPacket);
    void    EOMDHandler();
    void    recvFile(char* VcmtpPacket);
    void    recvMemData(char* VcmtpPacket);

private:
    string           tcpAddr;           /* Address of TCP server for missed data     */
    unsigned short   tcpPort;           /* Port number of TCP server for missed data */
    string           localAddr;
    unsigned short   localPort;
    int              max_sock_fd;
    int              mcast_sock;
    int              retx_tcp_sock;
    pthread_t        recv_thread;
    pthread_t        retx_thread;
    fd_set           read_sock_set;
    struct sockaddr_in  sender;
    int              fileDescriptor;   /* file descriptor where to store a received file */
    VcmtpHeader      vcmtpHeader;      /* store header for each vcmtp packet */
    BOFMsg           BOFmsg;
    BOMDMsg          BOMDmsg;

    static void*  StartReceivingThread(void* ptr);
};

#endif /* VCMTPRECV_H_ */
