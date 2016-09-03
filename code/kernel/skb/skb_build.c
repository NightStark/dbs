#include <asm/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_bridge.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/kobject.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/inet.h>

#include <asm/io.h>
#define UP_MSG_PRINTF(fmt, ...) \
    printk("[%s][%d]", __func__, __LINE__); \
    printk(fmt, ##__VA_ARGS__); \
    printk("\n")

//#include <stdarg.h>

static void
__dump_data(u8 *ptr, int len, const char *info_fmt, ...)
{
    va_list ap;
    int i;
    printk("\n************************\n");
    printk("dump [");
    va_start(ap, info_fmt);
    vprintk(info_fmt, ap);
    va_end(ap);
    printk("]\n");

    for (i = 0; i < len; i++) {
        if (!(i%16))
            printk("\n %04x", i);
        printk(" %02x", ptr[i]);
    }
    printk("\n************************\n");
}

__be32
_str2ip (const char *str)
{
	return in_aton(str); /* linux/inet.h */
}

struct sk_buff *__skb_new_udp_pack(int is_v6,
                                        __be32 daddr,
                                        __be32 saddr,
                                        __be16 dst,
                                        __be16 src,
                                        unsigned char * udp_msg,
                                        int udp_msg_len, 
                                        unsigned char * dns_rsp,
                                        int dns_rsp_len,
                                        struct ipv6hdr *v6hdr)
{
    int total_len, eth_len, ip_len, udp_len, header_len;
    struct sk_buff *skb  = NULL;
    struct udphdr  *udph = NULL;
    struct iphdr   *iph  = NULL;
    struct ipv6hdr *rsp_v6hdr = NULL;
	struct ethhdr *eh = NULL;
    //char *rsp = NULL;
    
    printk("[%s][%d]----daddr:0x%x saddr:0x%x dst:%d src:%d------\n", 
            __func__, __LINE__,
            daddr,
            saddr,
            ntohs(dst),
            ntohs(src)
            );
    printk("[%s][%d]----udp msg:0x%x udp msg len:%d rsp msg:0x%x rsp msg len:%d------\n", 
            __func__, __LINE__,
            (__be32)udp_msg,
            udp_msg_len,
            (__be32)dns_rsp,
            dns_rsp_len
            );

    /* calculate skb len by protocals-headers */
    udp_len   = udp_msg_len  + sizeof(struct udphdr) + dns_rsp_len;                   
    if (!is_v6) {
        ip_len    = udp_len      + sizeof(struct iphdr);
    } else {
        ip_len    = udp_len      + sizeof(struct ipv6hdr);
    }
    eth_len   = ip_len       + ETH_HLEN;
    total_len = eth_len      + NET_IP_ALIGN;
    total_len += LL_MAX_HEADER;      
    header_len = total_len - udp_msg_len;
    /* alloc skb */
    if (!(skb = alloc_skb(total_len, GFP_KERNEL))) {
        printk("%s:alloc_skb failed!\n", __func__);
        return NULL;
    }   
    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
    /* reserve for header */                          
    skb_reserve(skb, header_len);                     
    /* copy udp payload data to skb */
    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
    if (udp_msg_len > 0) {
        skb_copy_to_linear_data(skb, udp_msg, udp_msg_len);
        skb->len += udp_msg_len;
        printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
        /* add dns response data */
        if (dns_rsp != NULL && dns_rsp_len != 0) {
            skb_copy_to_linear_data_offset(skb, udp_msg_len, dns_rsp, dns_rsp_len);
            skb->len += dns_rsp_len;
            printk("[%s][%d]-add rsp data---skb len:%d-----\n",  __func__, __LINE__, skb->len);
        }
    }

    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);

    /* init & set udp header info */
    skb_push(skb, sizeof(struct udphdr));
    skb_reset_transport_header(skb);
    
    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);

    udph = udp_hdr(skb);
    udph->source = src;
    udph->dest = dst;
    udph->len = htons(udp_len); /* dns rep is inster !! */
    udph->check = 0;

    if (!is_v6) {
        udph->check = csum_tcpudp_magic(saddr, daddr,
                udp_len, IPPROTO_UDP,
                csum_partial(udph, udp_len, 0));
        if (udph->check == 0)
            udph->check = CSUM_MANGLED_0;
    } else {
        /* copy from linux kernel: udp_v6_push_pending_frames() */
        __wsum csum = csum_partial(skb_transport_header(skb),
                sizeof(struct udphdr), 0);
        udph->check = csum_ipv6_magic(&(v6hdr->daddr), &(v6hdr->saddr), /* v6hdr's src as rsp_v6hdr's dst*/
                udph->len, IPPROTO_UDP, csum);
    }
    
    if (!is_v6) {
        skb_push(skb, sizeof(struct iphdr));
        skb_reset_network_header(skb);
        printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
        iph = ip_hdr(skb);
        memset(iph, 0, sizeof(struct iphdr));
        iph->protocol = IPPROTO_UDP;
        iph->version = 4;
        iph->ihl = 5;
        //put_unaligned(0x45, (unsigned char *)iph);
        iph->saddr = saddr;
        iph->daddr = daddr;
        //put_unaligned(saddr, &(iph->saddr));  
        //put_unaligned(daddr, &(iph->daddr)); 
        iph->ttl = 64;
        iph->id = 0; 
        iph->tos = 0;
        iph->tot_len = htons(ip_len);
        //put_unaligned(htons(ip_len), &(iph->tot_len));
        iph->frag_off = htons(IP_DF);
        //iph->frag_off = htons(0x4000 & 0xE000); /* Do not fragment */
        iph->check = 0;
        iph->check =  ip_fast_csum((unsigned char *)iph, iph->ihl);       
    } else {
        skb_push(skb, sizeof(struct ipv6hdr));
        skb_reset_network_header(skb);
        printk("[%s][%d]-ipv6---skb len:%d-----\n",  __func__, __LINE__, skb->len);
        rsp_v6hdr = ipv6_hdr(skb);
        memcpy(rsp_v6hdr, v6hdr, sizeof(struct ipv6hdr));
        memcpy(&(rsp_v6hdr->daddr), &(v6hdr->saddr), sizeof(struct in6_addr));
        memcpy(&(rsp_v6hdr->saddr), &(v6hdr->daddr), sizeof(struct in6_addr));
        rsp_v6hdr->payload_len = (udph->len);
        printk("[%s][%d]-ipv6---set ns make on skb-----\n",  __func__, __LINE__);
        skb->ns_mark = 1;
    }

    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
    eh = (struct ethhdr *)skb_push(skb, ETH_HLEN);       
    skb_reset_mac_header(skb);                           
    printk("[%s][%d]----skb len:%d-----\n", 
            __func__, __LINE__, skb->len);
    skb->protocol = eth_hdr(skb)->h_proto;               
    eh->h_proto = eth_hdr(skb)->h_proto;                  
    memcpy(eh->h_source, eth_hdr(skb)->h_dest, ETH_ALEN); 
    memcpy(eh->h_dest, eth_hdr(skb)->h_source, ETH_ALEN); 
    printk("[%s][%d]----pack set over. ssp skb len:%d-----\n", 
            __func__, __LINE__, skb->len);
    __dump_data(skb->data, skb->len, "all pkt");
    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);

    return skb;
}

static int __skb_build_skb(void)
{
    __be32 saddr = 0; 
    __be32 daddr = 0; 
    struct net_device *dev = NULL;
    char msg_buf[128];
    int msg_len = 0;

    dev = dev_get_by_name(&init_net, "eth0");
    if (NULL == dev) {
        return -1;
    }

    msg_len = snprintf(msg_buf, sizeof(msg_buf), "%s", "this is msg.");

    saddr = _str2ip("192.168.62.1");
    daddr = _str2ip("127.0.0.1");
    __skb_new_udp_pack(0, daddr, saddr, 8819,9918, msg_buf, msg_len + 1, NULL, 0, NULL);

    return 0;
}

static int __init skb_build_init(void)
{
    __skb_build_skb();
    UP_MSG_PRINTF("init success.");

    return 0;
}

static void __exit skb_build_exit(void)
{

    UP_MSG_PRINTF("exit success.");

    return;
}

module_init(skb_build_init);
module_exit(skb_build_exit);

MODULE_AUTHOR("G");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("skb build");

