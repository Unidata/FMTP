#include <iostream>
#include <string>
#include "vcmtpRecvv3.h"

int main(int argc, char* argv[])
{
    string tcpAddr                 = "127.0.0.1";
    const unsigned short tcpPort   = 5000;
    //string mcastAddr               = "233.0.225.123";
    string mcastAddr               = "0.0.0.0";
    const unsigned short mcastPort = 388;

    vcmtpRecvv3 vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr, mcastPort);
    vcmtpRecvv3.Start();
    while(1);
}
