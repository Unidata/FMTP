#include <iostream>
#include "MulticastComm.h"
#include <errno.h>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    const string demoServHost = "rivanna.cs.virginia.edu";
    const int demoServPort = 1234;
    char sendDataBuf[256], recvDataBuf[28];

    // clear two buffers
    bzero(sendDataBuf, 256);
    bzero(recvDataBuf, 28);

    MulticastComm demoMcast;

    cout<<"UDP socket file descriptor is: "<<demoMcast.GetSocket()<<endl;
    //while(1);
    return 0;
}
