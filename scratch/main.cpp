#include <iostream>
#include <string>
#include <vcmtpReceiver.h>

int main(int argc, char* argv[])
{
    string tcpAddr               = "127.0.0.1";
    const unsigned short tcpPort = 5000;
    VCMTPReceiver vcmtpRecv(tcpAddr, tcpPort);
    vcmtpRecv.Start();
    return 0;
}
