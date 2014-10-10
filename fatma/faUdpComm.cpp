/*
 * UpdComm.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: fatmaal-ali
 */

/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: UdpComm.cpp
 *
 * @history:
 *      Created on : Jul 21, 2011
 *      Author     : jie
 *      Modified   : Sep 21, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */
#include"faUdpComm.h"

#include <aio.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netdb.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <sys/uio.h>

#include<iostream>
using namespace std;
/*****************************************************************************
 * Class Name: UdpComm
 * Function Name: UdpComm()
 *
 * Description: Constructor. Create a udp socket with the receiver address and port
 *
 * Input:  port     port number to listen.
 * Output: none
 ****************************************************************************/
UdpComm::UdpComm(const char* recvAddr,ushort port)
 {
    // create a UDP datagram socket.
	if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cout<<"UdpComm::socket() creating error";
	}
    // clear struct recv_addr.
	bzero(&recv_addr, sizeof(recv_addr));
    // set connection type to IPv4
	recv_addr.sin_family = AF_INET;
	//set the address to the receiver address passed to the constructor
	recv_addr.sin_addr.s_addr =inet_addr(recvAddr);
    //set the port number to the port number passed to the constructor
	recv_addr.sin_port = htons(port);
}


/*****************************************************************************
 * Class Name: UdpComm
 * Function Name: ~UdpComm()
 *
 * Description: Default destructor.
 *
 * Input:  none
 * Output: none
 ****************************************************************************/
UdpComm::~UdpComm() {
}


/*****************************************************************************
 * Class Name: UdpComm
 * Function Name: SendTo()
 *
 * Description: Send data in buff to a specific receiver address
 *
 * Input:  *buff       data buffer
 *         len         buffer length
 *         flags       transferring flags
 * Output: none
 ****************************************************************************/
ssize_t UdpComm::SendTo(const void* buff, size_t len, int flags)
{
	//return sendto(sock_fd, buff, len, flags, to_addr, to_len);
	return sendto(sock_fd, buff, len, flags, (struct sockaddr *) &recv_addr, sizeof(recv_addr));

}

size_t UdpComm::SendData( void*  header, const size_t headerLen,  void*  data, const size_t dataLen)
{
    struct iovec iov[2];//vector including the two memory locations

    iov[0].iov_base = header;
    iov[0].iov_len  = headerLen;
    iov[1].iov_base = data;
    iov[1].iov_len  = dataLen;

    return writev(sock_fd, iov, 2);
}
