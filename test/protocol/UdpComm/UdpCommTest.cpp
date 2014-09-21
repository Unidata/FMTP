#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "UdpComm.h"

int main(int argc, char* argv[])
{
    char sendbuf[] = "Potomac says hello!";
    char recvbuf[8192];
    struct sockaddr_in dst_addr;
	bzero(&dst_addr, sizeof(dst_addr));
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(5001);
    inet_pton(AF_INET, "128.143.137.117", &dst_addr.sin_addr.s_addr);

    UdpComm demoUDP(5000);
    demoUDP.SetSocketBufferSize(8192);
    demoUDP.SendTo(sendbuf, sizeof(sendbuf), 0, dst_addr, sizeof(dst_addr));
    return 0;
}
