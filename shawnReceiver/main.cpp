#include <iostream>
#include <string>
#include "vcmtpRecv.h"
//#include <unistd.h>

int main(int argc, char* argv[])
{
    string tcpAddr                 = "127.0.0.1";
    const unsigned short tcpPort   = 5000;
    string localAddr               = "0.0.0.0";
    const unsigned short localPort = 5173;

    vcmtpRecv vcmtpRecv(tcpAddr, tcpPort, localAddr, localPort);
    vcmtpRecv.Start();
    while(1);
}
