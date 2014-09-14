#include <iostream>
#include "MulticastComm.h"
#include <string>
#include <linux/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main(int argc, char* argv[])
{
    const string demoServHost = "rivanna.cs.virginia.edu";
    const int demoServPort = 1234;
    char sendDataBuf[256], recvDataBuf[28];

    // clear two buffers
    bzero(sendDataBuf, 256);
    bzero(recvDataBuf, 28);

    struct sockaddr_in demo_sain;
    bzero(&demo_sain, sizeof(demo_sain));
    char* if_name = "eth0";
    demo_sain.sin_family = AF_INET;
    demo_sain.sin_addr.s_addr = inet_addr("224.0.0.1");
    demo_sain.sin_port = htons(demoServPort);

    MulticastComm demoMcast;
    int join_retval = demoMcast.JoinGroup((sockaddr *)demo_sain, sizeof(demo_sain), if_name);
    if(join_retval == 0)
        cout << "UDP socket set, multicast group set." << endl;

    //while(1);
    return 0;
}
