#include <linux/ethtool.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    int sock;
    struct ifreq ifr;
    struct ethtool_cmd edata;
    int rc;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
    ifr.ifr_data = &edata;

    edata.cmd = ETHTOOL_GSET;

    rc = ioctl(sock, SIOCETHTOOL, &ifr);
    if (rc < 0) {
        perror("ioctl");
        exit(1);
    }
    switch (ethtool_cmd_speed(&edata)) {
        case SPEED_10: printf("10Mbps\n"); break;
        case SPEED_100: printf("100Mbps\n"); break;
        case SPEED_1000: printf("1Gbps\n"); break;
        case SPEED_2500: printf("2.5Gbps\n"); break;
        case SPEED_10000: printf("10Gbps\n"); break;
        default: printf("Speed returned is %d\n", edata.speed);
    }

    return (0);
}
