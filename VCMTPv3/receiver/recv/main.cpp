#include <iostream>
#include <string>
#include "vcmtpRecvv3.h"

int main(int argc, char* argv[])
{
    string tcpAddr                 = "128.143.137.117";
    const unsigned short tcpPort   = 1234;
    string mcastAddr               = "233.0.225.123";
    const unsigned short mcastPort = 5173;

    vcmtpRecvv3 vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr, mcastPort);
    vcmtpRecvv3.Start();
    vcmtpRecvv3.sendRetxReq();
    while(1);
}
