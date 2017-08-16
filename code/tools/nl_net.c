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
#include <sys/ioctl.h>

#define __LOG(level, fmt, ...) \
    do { \
        printf("[%s][%d]"fmt"\n" , __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

typedef enum route_act
{
    ROUTE_ACT_DOWN = 0,
    ROUTE_ACT_UP, 

    ROUTE_ACT_MAX
}ROUTE_ACT_EN;

typedef struct if_addr_info
{
    char ifname[IFNAMSIZ];
    char ifaddr[16];
    char mask[16];
    char dst[16];
    char gw[16];
}IF_ADDR_INFO_ST;

IF_ADDR_INFO_ST ifinfo_apcli0;
IF_ADDR_INFO_ST ifinfo_eth02;

char *ip2str(unsigned int ipv4, char *str_ip, unsigned int str_len)
{
    if (str_ip == NULL || str_len < 16) {
        return NULL;
    }

    sprintf(str_ip, "%d.%d.%d.%d", (ipv4>>24)&0xff, (ipv4>>16)&0xff,
            (ipv4>>8)&0xff, ipv4&0xff);
    return str_ip;
}

char *
get_cmd_result_line(char *cmd, char *value, int value_len)
{
    FILE *fp = NULL;

    if (cmd == NULL || value == NULL) {
        return NULL;
    }

    if (value_len <= 0) {
        __LOG(LOG_DEBUG, "value_len is 0 !\n");
        return NULL;
    }

    if (value[0] != '\0') {
        __LOG(LOG_DEBUG, "value is not empty, clear it, forced\n");
        memset(value, 0, value_len);
    }

    fp = popen(cmd, "r");
    if (fp != NULL ) {
        if(fgets(value, value_len, fp) != NULL) {
            if(value[strlen(value) - 1] == '\n')
                value[strlen(value) - 1] = '\0';
            pclose(fp);
            return value;
        }
        pclose(fp);
    }

    return NULL;
}

void parseBinaryNetlinkMessage(struct nlmsghdr *nh) 
{  
    int len = nh->nlmsg_len - sizeof(*nh);  
    struct ifinfomsg *ifi;  

    if (sizeof(*ifi) > (size_t) len) {  
        __LOG(LOG_DEBUG, "Got a short RTM_NEWLINK message\n");  
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
                    __LOG(LOG_DEBUG, "%s link %s\n", ifname, action);  
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
            uint32_t ipaddr = ntohl(*((uint32_t *)RTA_DATA(rth)));  
            char name[IFNAMSIZ];  
            if_indextoname(ifa->ifa_index, name);  
            __LOG(LOG_DEBUG, "%s %s address %d.%d.%d.%d\n",  
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
    __LOG(LOG_DEBUG, "%s : %d.%d.%d.%d\n", str,
    (ipv4>>24)&0xff, (ipv4>>16)&0xff, (ipv4>>8)&0xff, ipv4&0xff);

    return;
}

static int get_if_addr_infos(IF_ADDR_INFO_ST *ifi)
{
    int sockfd;  
    struct ifreq ifr;  
    char *address = NULL;
    struct sockaddr_in *addr = NULL;  

    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifi->ifname);
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd < 0) {
        __LOG(LOG_DEBUG, "socket create failed.\n");
        return -1;
    }

    if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        __LOG(LOG_DEBUG, "get if addr failed.\n");
        goto error;
    }
    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    address = inet_ntoa(addr->sin_addr);  
    __LOG(LOG_DEBUG, "inet addr: %s\n",address); 
    snprintf(ifi->ifaddr, sizeof(ifi->ifaddr), "%s", address);

    if(ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
        __LOG(LOG_DEBUG, "get if mask failed.\n");
        goto error;
    }
    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    address = inet_ntoa(addr->sin_addr);  
    __LOG(LOG_DEBUG, "inet mask: %s\n",address); 
    snprintf(ifi->mask, sizeof(ifi->mask), "%s", address);

    #if 0
    if(ioctl(sockfd, SIOCGIFDSTADDR , &ifr) < 0) {
        __LOG(LOG_DEBUG, "get if dst addr  failed.\n");
        goto error;
    }
    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    address = inet_ntoa(addr->sin_addr);  
    __LOG(LOG_DEBUG, "inet dst addr: %s\n",address); 
    #endif

    close(sockfd);
    sockfd = -1;
    return 0;

error:
    if (sockfd > 0) {
        close(sockfd);
        sockfd = -1;
    }
    return -1;
}

/**/
static int route_item_adjust(const char *ifname, ROUTE_ACT_EN rt_act) 
{
    char cmd_buf[256];   
    char gwif_buf[32] = {0};
    IF_ADDR_INFO_ST *ifi = NULL;

    if (rt_act == ROUTE_ACT_DOWN) {
        get_cmd_result_line("cat /proc/net/route | awk '{if($2==\"00000000\"){print $1}}'",
                gwif_buf, sizeof(gwif_buf)); 

        if (gwif_buf[0] == '\0') {
            __LOG(LOG_DEBUG, "no route now, check up if!!!\n");
            return -1;
        }

        __LOG(LOG_DEBUG, "gwif : %s\n", gwif_buf);
        if (strncmp(ifname, gwif_buf, strlen(gwif_buf) + 1) != 0) {
            __LOG(LOG_DEBUG, "%s is not use if, skip.\n", gwif_buf);
            return 0;
        }

        if (strncmp(ifname, "eth0.2", strlen("eth0.2") + 1) == 0) {
            /* if (apcli0 is up) { */
            /* if (apcli0 is real connection) { */
            ifi = &ifinfo_apcli0;
            /* } else { return -1 }} */
        } else if (strncmp(ifname, "apcli0", strlen("eth0.2") + 1) == 0) {
            /* if (eth0.2 is up) { */
            /* if (eth0.2 is real connection) { */
            ifi = &ifinfo_eth02;
            /* } else { return -1 }} */
        }

        snprintf(cmd_buf, sizeof(cmd_buf), "%s", "route del default");
        __LOG(LOG_DEBUG, "cmd_buf[%s]\n", cmd_buf);
        system(cmd_buf);
        snprintf(cmd_buf, sizeof(cmd_buf), "route add default gw %s netmask 0.0.0.0 dev apcli0", ifi->gw);
        __LOG(LOG_DEBUG, "cmd_buf[%s]\n", cmd_buf);
        system(cmd_buf);
        snprintf(cmd_buf, sizeof(cmd_buf), "route add -host %s  dev apcli0", ifi->gw);
        __LOG(LOG_DEBUG, "cmd_buf[%s]\n", cmd_buf);
        system(cmd_buf);
    }
    
    return 0;
}

/* parse this msg refer linux kernel(3.18.36)
 *      file: net/ipv4/route.c 
 *      func: inet_rtm_getroute()-->rt_fill_info(... RTM_NEWROUTE ...)
 * */
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
        __LOG(LOG_DEBUG, "Got a short RTM_NEWROUTE message\n");  
        return;  
    }  

    rt = (struct rtmsg*)NLMSG_DATA(nlh);  
    rta = (struct rtattr *)((char *) rt + NLMSG_ALIGN(sizeof(*rt)));  
    len = NLMSG_PAYLOAD(nlh, sizeof(*rt));  

    while(RTA_OK(rta, len)) {  
        __LOG(LOG_DEBUG, "rta_type:%d\n", rta->rta_type);
        switch (rta->rta_type) {
        case RTA_DST:
            dst = ntohl(*((unsigned int*)RTA_DATA(rta)));  
            printf_ip("ADD_ROUTE dst ip", dst);
            break;
        case RTA_OIF:
            dst_ifindex = *((unsigned int*)RTA_DATA(rta));
            if (NULL == if_indextoname(dst_ifindex, ifname)) {
                __LOG(LOG_DEBUG, "ifindex to name failed.\n");
                return;
            }
            __LOG(LOG_DEBUG, "ADD_ROUTE OUTIF ifindex:%d ifname:%s\n", dst_ifindex, ifname);
            break;
        case RTA_GATEWAY:
            gateway = ntohl(*((unsigned int*)RTA_DATA(rta)));
            printf_ip("ADD_ROUTE gateway", gateway);
            break;
        case RTA_PREFSRC:
            prefsrc = ntohl(*((unsigned int*)RTA_DATA(rta)));
            printf_ip("ADD_ROUTE prefsrc ", prefsrc);
            break;
        case RTA_TABLE:
            rt_table = *((unsigned int*)RTA_DATA(rta));
            __LOG(LOG_DEBUG, "ADD_ROUTE rt_table = %d\n", rt_table);
            break;
        }
        rta = RTA_NEXT(rta, len);  
    }  

    if (strncmp(ifname, "apcli0", strlen("apcli0") + 1) == 0) {
        get_if_addr_infos(&ifinfo_apcli0);
        //snprintf(ifinfo_apcli0.gw, sizeof(ifinfo_apcli0.gw), "%s", gateway);
        ip2str(gateway, ifinfo_apcli0.gw, sizeof(ifinfo_apcli0.gw));
    } else if (strncmp(ifname, "eth0.2", strlen("eth0.2") + 1) == 0) {
        get_if_addr_infos(&ifinfo_eth02);
        ip2str(gateway, ifinfo_eth02.gw, sizeof(ifinfo_eth02.gw));
    }

    return;
}  

/* parse this msg refer linux kernel(3.18.36)
 *      file: net/ipv4/ipmr.c
 *      func: mroute_netlink_event(... RTM_DELROUTE)-->ipmr_fill_mroute()
 * */
void parse_netlink_del_route(struct nlmsghdr *nlh)  
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
        __LOG(LOG_DEBUG, "Got a short RTM_NEWROUTE message\n");  
        return;  
    }  

    rt = (struct rtmsg*)NLMSG_DATA(nlh);  
    rta = (struct rtattr *)((char *) rt + NLMSG_ALIGN(sizeof(*rt)));  
    len = NLMSG_PAYLOAD(nlh, sizeof(*rt));  

    while(RTA_OK(rta, len)) {  
        __LOG(LOG_DEBUG, "rta_type:%d\n", rta->rta_type);
        switch (rta->rta_type) {
        case RTA_DST:
            dst = ntohl(*((unsigned int*)RTA_DATA(rta)));  
            printf_ip("DEL_ROUTE dst ip", dst);
            break;
        case RTA_OIF:
            dst_ifindex = *((unsigned int*)RTA_DATA(rta));
            if (NULL == if_indextoname(dst_ifindex, ifname)) {
                __LOG(LOG_DEBUG, "ifindex to name failed.\n");
                return;
            }
            __LOG(LOG_DEBUG, "DEL_ROUTE OUTIF ifindex:%d ifname:%s\n", dst_ifindex, ifname);
            break;
        case RTA_GATEWAY:
            gateway = ntohl(*((unsigned int*)RTA_DATA(rta)));
            printf_ip("DEL_ROUTE gateway", gateway);
            break;
        case RTA_PREFSRC:
            prefsrc = ntohl(*((unsigned int*)RTA_DATA(rta)));
            printf_ip("DEL_ROUTE prefsrc ", prefsrc);
            break;
        case RTA_TABLE:
            rt_table = *((unsigned int*)RTA_DATA(rta));
            __LOG(LOG_DEBUG, "DEL_ROUTE rt_table = %d\n", rt_table);
            break;
        }
        rta = RTA_NEXT(rta, len);  
    }  

    route_item_adjust(ifname, ROUTE_ACT_DOWN);

    return;
}  

int main(int argc, char* argv[])  
{  
    int nl_sk = 1, len = 0;  
    char buffer[4096];  
    struct nlmsghdr *nlh = NULL;;  
    struct sockaddr_nl addr;  

    snprintf(ifinfo_apcli0.ifname, sizeof(ifinfo_apcli0.ifname), "%s", "apcli0");
    snprintf(ifinfo_eth02.ifname, sizeof(ifinfo_eth02.ifname), "%s", "eth02");

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
            __LOG(LOG_DEBUG, "nsmsgg_type=%d\n", nlh->nlmsg_type);
            if (nlh->nlmsg_type == RTM_NEWADDR) {  
                parseNetlinkAddrMsg(nlh, 1);  
            } else if(nlh->nlmsg_type == RTM_DELADDR) {  
                parseNetlinkAddrMsg(nlh, 0);  
            } else if (nlh->nlmsg_type == RTM_NEWLINK) {  
                parseBinaryNetlinkMessage(nlh);  
            } else if (nlh->nlmsg_type == RTM_NEWROUTE) {
                __LOG(LOG_DEBUG, "========================\n");
                parse_netlink_add_route(nlh);
                __LOG(LOG_DEBUG, "========================\n");
            } else if (nlh->nlmsg_type == RTM_DELROUTE) {
                __LOG(LOG_DEBUG, "========================\n");
                parse_netlink_del_route(nlh);
                __LOG(LOG_DEBUG, "========================\n");
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
