/* langyanjun:
 *      many code is copy from pimd (https://github.com/troglobit/pimd)
 * */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> 
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h> 
#include <string.h> 
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <time.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>

#define TRUE            1
#define FALSE           0

#define IP_IGMP_HEADER_LEN      24

//#define INADDR_ALLHOSTS_GROUP   ((u_int32_t) 0xe0000001U)     /* 224.0.0.1 */

//#define IGMP_HOST_MEMBERSHIP_QUERY   0x11   /* Host membership query    */

#ifndef IGMP_MEMBERSHIP_QUERY
#define IGMP_MEMBERSHIP_QUERY       IGMP_HOST_MEMBERSHIP_QUERY
#endif

struct igmpv3_query {
    uint8_t  type;
    uint8_t  code;
    uint16_t csum;
    uint32_t group;
#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
    uint8_t  qrv:3,
             suppress:1,
             resv:4;
#else
    uint8_t  resv:4,
             suppress:1,
             qrv:3;
#endif
    uint8_t  qqic;
    uint16_t nsrcs;
    uint32_t srcs[0];
};

char   *igmp_send_buf;
int     igmp_socket;
uint32_t allhosts_group;

uint32_t igmp_query_interval  = 12;

 static void
__dump_data(unsigned char *ptr, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n");
}


int inet_cksum(uint16_t *addr, u_int len)
{
    int sum = 0;
    int nleft = (int)len;
    uint16_t *w = addr;
    uint16_t answer = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(uint8_t *) (&answer) = *(uint8_t *)w ;
        sum += answer;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);         /* add carry */
    answer = ~sum;              /* truncate to 16 bits */

    return answer;
}

static inline uint8_t igmp_floating_point(unsigned int mantissa)
{
    unsigned int exponent;

    /* Wrap around numbers larger than 2^15, since those can not be
     * presented with 7-bit floating point. */
    mantissa &= 0x00007FFF;

    /* If top 8 bits are zero. */
    if (!(mantissa & 0x00007F80))
        return mantissa;

    /* Shift the mantissa and mark this code floating point. */
    mantissa >>= 3;
    /* At this point the actual exponent (bits 7-5) are still 0, but the
     *      * exponent might be incremented below. */
    exponent   = 0x00000080;

    /* If bits 7-4 are not zero. */
    if (mantissa & 0x00000F00) {
        mantissa >>= 4;
        /* The index of largest set bit is at least 4. */
        exponent  |= 0x00000040;
    }

    /* If bits 7-6 OR bits 3-2 are not zero. */
    if (mantissa & 0x000000C0) {
        mantissa >>= 2;
        /* The index of largest set bit is atleast 6 if we shifted the
         * mantissa earlier or atleast 2 if we did not shift it. */
        exponent  |= 0x00000020;
    }

    /* If bit 7 OR bit 3 OR bit 1 is not zero. */
    if (mantissa & 0x00000020) {
        mantissa >>= 1;
        /* The index of largest set bit is atleast 7 if we shifted the
         * mantissa two times earlier or atleast 3 if we shifted the
         * mantissa last time or atleast 1 if we did not shift it. */
        exponent  |= 0x00000010;
    }

    return exponent | (mantissa & 0x0000000F);
}

void k_set_if(int socket, uint32_t ifa)
{
    struct in_addr adr;

    adr.s_addr = ifa;
    if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&adr, sizeof(adr)) < 0) {
        if (errno == EADDRNOTAVAIL || errno == EINVAL)
            printf("setsockopt IP_MULTICAST_IF error.\n");
        perror("setsockopt IP_MULTICAST_IF error.");
        return;
    }

    return;
}

void k_set_loop(int socket, int flag)
{
    uint8_t loop;

    loop = flag;
    if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop)) < 0) {
        printf("setsockopt IP_MULTICAST_LOOP error.\n");
        perror("setsockopt IP_MULTICAST_LOOP error.");
    }

    return;
}

void k_hdr_include(int socket, int val)
{
#ifdef IP_HDRINCL
    if (setsockopt(socket, IPPROTO_IP, IP_HDRINCL, (char *)&val, sizeof(val)) < 0) {
        printf("setsockopt IP_HDRINCL error.\n");
        perror("setsockopt IP_HDRINCL error.");
    }
#endif
}

static void send_ip_frame(uint32_t src, uint32_t dst, int type, int code, char *buf, size_t len)
{
    int setloop = 0;
    struct ip *ip;
    struct sockaddr_in sin;
    char source[20], dest[20];

    /* Prepare the IP header */
    len          += IP_IGMP_HEADER_LEN;
    ip            = (struct ip *)buf;
    ip->ip_id         = 0; /* let kernel fill in */
    ip->ip_off        = 0;
    ip->ip_src.s_addr = src;
    ip->ip_dst.s_addr = dst;
    ip->ip_len        = htons(len);

    k_set_if(igmp_socket, src);
    if (type != IGMP_DVMRP || dst == allhosts_group) {
        setloop = 1;
        k_set_loop(igmp_socket, TRUE);
    }

    //ip->ip_ttl = MAXTTL;
    ip->ip_ttl = 1;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = dst;

    __dump_data(buf, len);

    if (sendto(igmp_socket, buf, len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        printf("sendto error.\n");
        perror("sendto error.");
    }

    if (setloop)
        k_set_loop(igmp_socket, FALSE);

    return;
}

void send_igmp(char *buf, uint32_t src, uint32_t dst, int type, int code, uint32_t group, int datalen)
{
    size_t len = IGMP_MINLEN + datalen;
    struct igmpv3_query *igmp;

    igmp              = (struct igmpv3_query *)(buf + IP_IGMP_HEADER_LEN);
    igmp->type        = type;
    if (datalen >= 4)
        igmp->code    = igmp_floating_point(code);
    else
        igmp->code    = code;
    igmp->group       = group;
    igmp->csum        = 0;

    if (datalen >= 4) {
        igmp->qrv = 2;
        igmp->qqic = igmp_floating_point(igmp_query_interval);
    }

    igmp->csum        = inet_cksum((uint16_t *)igmp, len);

    printf("len = %d\n", len);

    send_ip_frame(src, dst, type, code, buf, len);

    return;
}



int get_if_ip(void)
{
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in sin;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }
    strcpy(ifr.ifr_name,"eth1");
    if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)//直接获取IP地址
    {
        perror("ioctl error");
        return -1;
    }
    memcpy(&sin, &ifr.ifr_dstaddr, sizeof(sin));

    printf("ip is %s \n",inet_ntoa(sin.sin_addr));

    return sin.sin_addr.s_addr;
}

int igmp_init(void)
{
    struct ip *ip;

    igmp_send_buf = calloc(1, 128 * 1024);
    allhosts_group   = htonl(INADDR_ALLHOSTS_GROUP);

    igmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (igmp_socket < 0) {
        perror("socket error");
        return -1;
    }

    k_hdr_include(igmp_socket, TRUE); /* include IP header when sending */

    ip         = (struct ip *)igmp_send_buf;
    memset(ip, 0, IP_IGMP_HEADER_LEN);
    ip->ip_v   = IPVERSION;
    ip->ip_hl  = IP_IGMP_HEADER_LEN >> 2;
    ip->ip_tos = 0xc0;          /* Internet Control   */
    ip->ip_id  = 0;         /* let kernel fill in */
    ip->ip_off = 0;
    ip->ip_ttl = MAXTTL;        /* applies to unicasts only */
    ip->ip_p   = IPPROTO_IGMP;
    ip->ip_sum = 0;         /* let kernel fill in */

    return 0;
}

int main(void)
{
    int code = 10 * 10;
    int datalen = 4;
    uint32_t uv_lcl_addr;

    igmp_init();

    uv_lcl_addr = get_if_ip();

    printf("type = %d\n", IGMP_MEMBERSHIP_QUERY);

    send_igmp(igmp_send_buf, uv_lcl_addr, allhosts_group,
            IGMP_MEMBERSHIP_QUERY,
            code, 0, datalen);

    return 0;
}
