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

#include "../vcmtpBase.h"
#include "ReceivingApplicationNotifier.h"

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
              string mcastAddr,
              const unsigned short mcastPort,
              ReceivingApplicationNotifier& notifier);
    ~vcmtpRecv();

    void    Start(); // initialize the private variables
    void    Stop();

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
    BOPMsg           BOMDmsg;
    ReceivingApplicationNotifier& notifier;
    void*            prodptr; // void pointer obtained from receiving application indicating where to save the incoming data

    static void*  StartReceivingThread(void* ptr);
    void    StartReceivingThread();
    void    RunReceivingThread();
    // basically it's joinGroup(), and should be called by Start()
    int     udpBindIP2Sock(string senderAddr, const unsigned short port);
    void    McastPacketHandler();
    void    BOPHandler(char* VcmtpPacket);
    void    EOPHandler();
    void    recvMemData(char* VcmtpPacket);
};

#endif /* VCMTPRECV_H_ */
