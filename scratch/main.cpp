#include <iostream>
#include <string>
#include "vcmtpReceiver.h"
#include <unistd.h>

int main(int argc, char* argv[])
{
    string tcpAddr               = "127.0.0.1";
    const unsigned short tcpPort = 5000;
    VCMTPReceiver vcmtpRecv(tcpAddr, tcpPort);
    vcmtpRecv.Start();
    sleep(1);
    pthread_join()
    return 0;
}
