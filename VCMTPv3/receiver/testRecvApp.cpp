/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      testRecvApp.cpp
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Feb 14, 2015
 *
 * @section   LICENSE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief     A testing application to use the receiver side protocol.
 *
 * Since the LDM could be to heavy to use for testing purposes only. This
 * testRecvApp is written as a replacement. It can create an instance of the
 * vcmtpRecvv3 instance and mock the necessary components to get it functioning.
 */


#include "vcmtpRecvv3.h"

#include <iostream>
#include <thread>

#include <netinet/tcp.h>
#include <unistd.h>


/**
 * A separate thread to run VCMTP receiver.
 *
 * @param[in] *ptr    A pointer to a vcmtpRecvv3 object.
 */
void runVCMTP(void* ptr)
{
    vcmtpRecvv3 *recv = static_cast<vcmtpRecvv3*>(ptr);
    recv->Start();
}


int tcpconn(char* tcpaddr)
{
    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return -1;
    }

    int flag = 1;
    int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
                            (char *) &flag, sizeof(int));
    if (result < 0) {
        printf("set TCP_NODELAY failed");
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(50000);
    if (inet_pton(AF_INET, tcpaddr, &serv_addr.sin_addr) <= 0) {
        printf("\n inet_pton error occured\n");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed \n");
        return -1;
    }

    return sockfd;
}


void tcpsend(int connfd, char* sendBuff, int bufsize)
{
    int n = write(connfd, sendBuff, bufsize);
    if (n == 1) {
        std::cout << "MSG sent" << std::endl;
    }
}


/**
 * Since the LDM could be too heavy to use for testing purposes only. This main
 * function is a light weight replacement of the LDM receiving application. It
 * sets up the whole environment and call Start() to start receiving. All the
 * arguments are passed in through command line.
 *
 * @param[in] tcpAddr      IP address of the sender.
 * @param[in] tcpPort      Port number of the sender.
 * @param[in] mcastAddr    multicast address of the group.
 * @param[in] mcastPort    Port number of the multicast group.
 * @param[in] ifAddr       IP of the interface to listen for multicast packets.
 */
int main(int argc, char* argv[])
{
    uint32_t index1, index2;
    if (argc < 5) {
        std::cerr << "ERROR: Insufficient arguments." << std::endl;
        return 1;
    }
    std::string tcpAddr(argv[1]);
    const unsigned short tcpPort = (unsigned short)atoi(argv[2]);
    std::string mcastAddr(argv[3]);
    const unsigned short mcastPort = (unsigned short)atoi(argv[4]);
    std::string ifAddr(argv[5]);

    char sendBuff[1];
    memset(sendBuff, 'Z', sizeof(sendBuff));
    int tcpfd = tcpconn(argv[1]);

    vcmtpRecvv3* recv = new vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr,
                                        mcastPort, NULL, ifAddr);
    recv->SetLinkSpeed(1000000000);
    std::thread t(runVCMTP, recv);
    t.detach();

    /* double check the product index to ensure transmission is finished */
    do {
        index1 = recv->getLastProdindex();
        sleep(10);
        index2 = recv->getLastProdindex();
    } while ((index1 != index2) || (index2 != 2));
    std::cout << "Received product: " << index2 << std::endl;
    recv->Stop();

    tcpsend(tcpfd, sendBuff, sizeof(sendBuff));
    close(tcpfd);

    delete recv;
    return 0;
}
