#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <net/if.h>
#include <netdb.h>
#include <errno.h>
#include <linux/netlink.h>  
#include <linux/rtnetlink.h>  

void parseBinaryNetlinkMessage(struct nlmsghdr *nh) {  
    int len = nh->nlmsg_len - sizeof(*nh);  
    struct ifinfomsg *ifi;  

    if (sizeof(*ifi) > (size_t) len) {  
        printf("Got a short RTM_NEWLINK message\n");  
        return;  
    }  

    ifi = (struct ifinfomsg *)NLMSG_DATA(nh);  
    if ((ifi->ifi_flags & IFF_LOOPBACK) != 0) {  
        return;  
    }  

    struct rtattr *rta = (struct rtattr *)  
        ((char *) ifi + NLMSG_ALIGN(sizeof(*ifi)));  
    len = NLMSG_PAYLOAD(nh, sizeof(*ifi));  

    while(RTA_OK(rta, len)) {  
        switch(rta->rta_type) {  
            case IFLA_IFNAME:  
                {  
                    char ifname[IFNAMSIZ];  
                    char *action;  
                    snprintf(ifname, sizeof(ifname), "%s",  
                            (char *) RTA_DATA(rta));  
                    //action = (ifi->ifi_flags & IFF_LOWER_UP) ? "up" : "down";  
                    action = (ifi->ifi_flags & IFF_RUNNING) ? "up" : "down";  
                    printf("%s link %s\n", ifname, action);  
                }  
        }  

        rta = RTA_NEXT(rta, len);  
    }  
}  

void parseNetlinkAddrMsg(struct nlmsghdr *nlh, int new)  
{  
    struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);  
    struct rtattr *rth = IFA_RTA(ifa);  
    int rtl = IFA_PAYLOAD(nlh);  

    while (rtl && RTA_OK(rth, rtl)) {  
        if (rth->rta_type == IFA_LOCAL) {  
            uint32_t ipaddr = htonl(*((uint32_t *)RTA_DATA(rth)));  
            char name[IFNAMSIZ];  
            if_indextoname(ifa->ifa_index, name);  
            printf("%s %s address %d.%d.%d.%d\n",  
                    name, (new != 0)?"add":"del",  
                    (ipaddr >> 24) & 0xff,  
                    (ipaddr >> 16) & 0xff,  
                    (ipaddr >> 8) & 0xff,  
                    ipaddr & 0xff);  
        }  
        rth = RTA_NEXT(rth, rtl);  
    }  
}  

void printf_ip(const char *str, unsigned int ipv4) {
    printf("%s : %d.%d.%d.%d\n", str,
    (ipv4>>24)&0xff, (ipv4>>16)&0xff, (ipv4>>8)&0xff, ipv4&0xff);

    return;
}

void parse_netlink_add_route(struct nlmsghdr *nlh)  
{  
    unsigned int dst = 0;
    unsigned int dst_ifindex = 0;
    unsigned int gateway = 0;
    unsigned int prefsrc = 0;
    unsigned int rt_table = 0;
    int len = 0;
    struct rtmsg *rt = NULL;
    struct rtattr *rta = NULL;
    char ifname[IFNAMSIZ];  

    len = nlh->nlmsg_len - sizeof(*nlh);  
    if (sizeof(*rt) > (size_t)len) {  
        printf("Got a short RTM_NEWROUTE message\n");  
        return;  
    }  

    rt = (struct rtmsg*)NLMSG_DATA(nlh);  
    rta = (struct rtattr *)((char *) rt + NLMSG_ALIGN(sizeof(*rt)));  
    len = NLMSG_PAYLOAD(nlh, sizeof(*rt));  

    while(RTA_OK(rta, len)) {  
        printf("rta_type:%d\n", rta->rta_type);
        switch (rta->rta_type) {
        case RTA_DST:
            //dst = htonl(*((unsigned int*)RTA_DATA(rth)));  
            dst = *((unsigned int*)RTA_DATA(rta));
            printf_ip("ADD_ROUTE dst ip", dst);
            break;
        case RTA_OIF:
            dst_ifindex = *((unsigned int*)RTA_DATA(rta));
            if (NULL == if_indextoname(dst_ifindex, ifname)) {
                printf("ifindex to name failed.\n");
                return;
            }
            printf("ADD_ROUTE OUTIF ifindex:%d ifname:%s\n", dst_ifindex, ifname);
            break;
        case RTA_GATEWAY:
            gateway = *((unsigned int*)RTA_DATA(rta));
            printf_ip("ADD_ROUTE gateway", gateway);
            break;
        case RTA_PREFSRC:
            prefsrc = *((unsigned int*)RTA_DATA(rta));
            printf_ip("ADD_ROUTE prefsrc ", prefsrc);
            break;
        case RTA_TABLE:
            rt_table = *((unsigned int*)RTA_DATA(rta));
            printf("ADD_ROUTE rt_table = %d\n", rt_table);
            break;
        }
        rta = RTA_NEXT(rta, len);  
    }  

    return;
}  

int main(int argc, char* argv[])  
{  
    int nl_sk = 1, len = 0;  
    char buffer[4096];  
    struct nlmsghdr *nlh = NULL;;  
    struct sockaddr_nl addr;  

    if ((nl_sk = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {  
        perror("couldn't open NETLINK_ROUTE nl_sket");  
        return 1;  
    }  

    memset(&addr, 0, sizeof(addr));  
    addr.nl_family = AF_NETLINK;  
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;  

    if (bind(nl_sk, (struct sockaddr *)&addr, sizeof(addr)) == -1) {  
        perror("couldn't bind");  
        goto error;
    }  

    while ((len = recv(nl_sk, buffer, sizeof(buffer), 0)) > 0) {  
        nlh = (struct nlmsghdr *)buffer;  
        while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {  
            printf("nsmsgg_type=%d\n", nlh->nlmsg_type);
            if (nlh->nlmsg_type == RTM_NEWADDR) {  
                parseNetlinkAddrMsg(nlh, 1);  
            } else if(nlh->nlmsg_type == RTM_DELADDR) {  
                parseNetlinkAddrMsg(nlh, 0);  
            } else if (nlh->nlmsg_type == RTM_NEWLINK) {  
                parseBinaryNetlinkMessage(nlh);  
            } else if (nlh->nlmsg_type == RTM_NEWROUTE) {
                printf("========================\n");
                parse_netlink_add_route(nlh);
                printf("========================\n");
            } else if (nlh->nlmsg_type == RTM_DELROUTE) {
            }  
            nlh = NLMSG_NEXT(nlh, len);  
        }  
    }  
    close(nl_sk);  

    return 0;  

error:

    if (nl_sk > 0) {
        close(nl_sk);  
        nl_sk = -1;
    }
    return -1;
}  
