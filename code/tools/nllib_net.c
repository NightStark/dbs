//compile with: 
//gcc tools/nllib_net.c  -I lib/netlink/ lib/netlink/knetlink.c -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
//#include <net/if.h>
#include <linux/if.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <linux/netlink.h>  
#include <linux/rtnetlink.h>  
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "knetlink.h"

int recv_newlink(struct nlmsghdr *h)
{
    struct ifinfomsg *ifi;
    char iface[IFNAMSIZ];

    ifi = NLMSG_DATA(h);

    if_indextoname(ifi->ifi_index, iface); 

    printf("[%s] >> RTM_NEWLINK - "
            "interface:%s, ifi_flags=%0x, ifi_change=%0x (%s%s%s%s)\n",
            __func__, iface, ifi->ifi_flags, ifi->ifi_change,
            (ifi->ifi_flags & IFF_UP) ? "[UP]" : "",
            (ifi->ifi_flags & IFF_RUNNING) ? "[RUNNING]" : "",
            (ifi->ifi_flags & IFF_LOWER_UP) ? "[LOWER_UP]" : "",
            (ifi->ifi_flags & IFF_DORMANT) ? "[DORMANT]" : "");

    return 0;
}


int event_hander(const struct sockaddr_nl *addr_nl, struct nlmsghdr *h, void *args)
{
    switch (h->nlmsg_type) {
        case RTM_NEWLINK:
            recv_newlink(h);
            break;
        default:
            break;
    }
    return 0;
}

void *net_st_monitor_work_thrd(void *args)
{  
    struct knl_handle kh = { .fd = -1 };
    prctl(PR_SET_NAME, (unsigned long)"if_stat");

    if(knl_open(&kh, RTMGRP_LINK) < 0)
        return NULL;

    knl_listen(&kh, event_hander, NULL);

    return NULL;
}

int main(int argc, char* argv[])  
{  
    pthread_t nm_tid;
    pthread_attr_t _attr;
    
    pthread_attr_init(&_attr);
	pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_DETACHED);
    if (0 != pthread_create(&nm_tid, &_attr, net_st_monitor_work_thrd, NULL)) {
        printf("create net status monitor thrd failed.\n");
        return -1;
    }

    pthread_exit(0);

    return 0;
}
