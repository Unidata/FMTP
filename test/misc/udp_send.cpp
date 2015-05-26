#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>


int main(int argc, char **argv)
{
    int s;
    int ret;
    char buf[] = "hello world!";
    struct sockaddr_in addr;

    if (argc != 3) {
        puts("usage: send ipaddr port");
        exit(1);
    }

    addr.sin_family = AF_INET;
    ret = inet_aton(argv[1], &addr.sin_addr);
    if (ret == 0) {
        perror("inet_aton");
        exit(1);
    }
    addr.sin_port = htons(atoi(argv[2]));

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket");
        exit(1);
    }

    while(1) {
        ret = sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            perror("sendto"); exit(1);
        }
    }

    return 0;
}
