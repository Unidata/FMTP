/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpReceiver.h
 *
 * @history:
 *      Created on : Oct 2, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPRECV_H_
#define VCMTPRECV_H_

#include "vcmtp.h"
#include "MulticastComm.h"
#include "RawSocketComm.h"
#include "InetComm.h"
#include "VCMTPComm.h"
#include "TcpClient.h"
#include "CommUtil/StatusProxy.h"
#include <pthread.h>


//class vcmtpRecv : public VCMTPComm {
class vcmtpRecv {
public:
    vcmtpRecv(string tcpAddr, const unsigned short tcpPort);
    ~vcmtpRecv();

    void    init();
    void    Start();
    void    StartReceivingThread();
    void    RunReceivingThread();
    int     udpBindIP2Sock(string senderAddr, ushort port);
    void    HandleMulticastPacket();
    void    HandleBofMessage(char* VcmtpPacket);

private:
    string           tcpAddr;    /* Address of TCP server for missed data     */
    unsigned short   tcpPort;    /* Port number of TCP server for missed data */
    int              max_sock_fd;
    int              multicast_sock;
    int              retrans_tcp_sock;
    pthread_t        recv_thread;
    pthread_t        retrans_thread;
    fd_set           read_sock_set;
    struct sockaddr_in  sender;

    static void*  StartReceivingThread(void* ptr);
};

#endif /* VCMTPRECEIVER_H_ */
