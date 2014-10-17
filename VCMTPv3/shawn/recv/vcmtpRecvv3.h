/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpRecvv3.h
 *
 * @history:
 *      Created on : Oct 17, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPRECVV3_H_
#define VCMTPRECVV3_H_

#include "vcmtpBase.h"
#include <stdint.h>
#include <string>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

class vcmtpRecvv3 {
public:
    vcmtpRecv(string tcpAddr,
              const unsigned short tcpPort,
              string mcastAddr,
              const unsigned short mcastPort,
              ReceivingApplicationNotifier& notifier);
    ~vcmtpRecv();

    void    Start(); // initialize the private variables
    void    Stop();

private:
    string           tcpAddr;           /* Address of TCP server for missed data     */
    unsigned short   tcpPort;           /* Port number of TCP server for missed data */
    string           mcastAddr;
    unsigned short   mcastPort;
    int              max_sock_fd;
    int              mcast_sock;
    int              retx_tcp_sock;
    pthread_t        recv_thread;
    pthread_t        retx_thread;
    fd_set           read_sock_set;
    struct sockaddr_in  mcastgroup;
    VcmtpHeader      vcmtpHeader;      /* store header for each vcmtp packet */
    BOPMsg           BOPmsg;
    ReceivingApplicationNotifier& notifier;
    void*            prodptr;          // void pointer obtained from receiving application indicating where to save the incoming data

    static void*  StartReceivingThread(void* ptr);
    void    StartReceivingThread();
    void    RunReceivingThread();
    void    joinGroup(string senderAddr, const unsigned short port);
    void    McastPacketHandler();
    void    BOPHandler(char* VcmtpPacket);
    void    EOPHandler();
    void    recvMemData(char* VcmtpPacket);
};

#endif /* VCMTPRECVV3_H_ */
