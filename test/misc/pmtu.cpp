#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

int main(int argc, char* argv[])
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "eth0");
    if(!ioctl(sock, SIOCGIFMTU, &ifr)) {
        printf("%d\n", ifr.ifr_mtu); // Contains current mtu value
    }
    return 0;
}
