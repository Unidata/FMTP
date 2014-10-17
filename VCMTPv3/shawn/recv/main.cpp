#include <iostream>
#include <string>
#include "vcmtpRecvv3.h"

int main(int argc, char* argv[])
{
    string tcpAddr                 = "127.0.0.1";
    const unsigned short tcpPort   = 5000;
    string mcastAddr               = "0.0.0.0";
    const unsigned short mcastPort = 5173;

    vcmtpRecv vcmtpRecv(tcpAddr, tcpPort, mcastAddr, mcastPort);
    vcmtpRecv.Start();
    while(1);
}
