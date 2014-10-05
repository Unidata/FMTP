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
#include <iostream>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
typedef struct VcmtpHeader {
    uint64_t   fileid;
    uint64_t   seqNum;
    uint64_t   payloadLen;
    uint64_t   flags;
};
*/

class vcmtpRecv {
public:
    vcmtpRecv(string tcpAddr, const unsigned short tcpPort,\
              string localAddr, const unsigned short localPort);
    ~vcmtpRecv();

    void    Start();
    void    StartReceivingThread();
    void    RunReceivingThread();
    int     udpBindIP2Sock(string senderAddr, ushort port);
    void    HandleMulticastPacket();
    void    HandleBofMessage(char* VcmtpPacket);

private:
    string           tcpAddr;           /* Address of TCP server for missed data     */
    unsigned short   tcpPort;           /* Port number of TCP server for missed data */
    string           localAddr;
    unsigned short   localPort;
    int              max_sock_fd;
    int              multicast_sock;
    int              retrans_tcp_sock;
    pthread_t        recv_thread;
    pthread_t        retrans_thread;
    fd_set           read_sock_set;
    struct sockaddr_in  sender;

    static void*  StartReceivingThread(void* ptr);
};

#endif /* VCMTPRECV_H_ */
