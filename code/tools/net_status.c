#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>

int net_detect(char* net_name)
{
    int skfd = 0;
    struct ifreq ifr;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0) {
        printf("%s:%d Open socket error!\n", __FILE__, __LINE__);
        return -1;
    }

    strcpy(ifr.ifr_name, net_name);

    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) <0 ) {
        printf("%s:%d IOCTL error!\n", __FILE__, __LINE__);
        printf("Maybe ethernet inferface %s is not valid!\n", ifr.ifr_name);
        close(skfd);
        return -1;
    }

    if(ifr.ifr_flags & IFF_RUNNING) {
        printf("UP\n");
    } else {
        printf("DOWN\n");
    }

    close(skfd);

    return 0;
}

int main()
{
    net_detect("eno16777736");

    return 0;
}
