#include <iostream>
#include <string>
#include "vcmtpRecvv3.h"
#include <unistd.h>

int main(int argc, char* argv[])
{
    string tcpAddr                 = "127.0.0.1";
    const unsigned short tcpPort   = 1234;
    //string mcastAddr               = "128.143.137.117"; /** IP addr of rivanna */
    string mcastAddr               = "233.0.225.123";
    //string mcastAddr               = "172.25.98.111";
    const unsigned short mcastPort = 5173;

    vcmtpRecvv3 vcmtpRecvv3(tcpAddr, tcpPort, mcastAddr, mcastPort);
    vcmtpRecvv3.Start();
    vcmtpRecvv3.sendRetxReq();
    sleep(2);
    vcmtpRecvv3.recvRetxData();
    while(1);
}
