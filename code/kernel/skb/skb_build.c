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

#include <linux/vmalloc.h>

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
                                        unsigned char *smac,
                                        unsigned char *dmac,
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
        __wsum csum = csum_partial(udph, udp_len, 0);
        udph->check = csum_ipv6_magic(&(v6hdr->saddr), &(v6hdr->daddr), /* v6hdr's src as rsp_v6hdr's dst*/
                udp_len, IPPROTO_UDP, csum); /* fuck , 这里的udp_len, 之前为什么要填udph->len, udph->len是网络字节序啊啊啊 */
    }
    if (udph->check == 0)
        udph->check = CSUM_MANGLED_0;
    
    if (!is_v6) {
        skb_push(skb, sizeof(struct iphdr));
        skb_reset_network_header(skb);
        printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
        iph = ip_hdr(skb);
        memset(iph, 0, sizeof(struct iphdr));
        iph->protocol = IPPROTO_UDP; /* UDP */
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
        //memcpy(&(rsp_v6hdr->daddr), &(v6hdr->saddr), sizeof(struct in6_addr));
        //memcpy(&(rsp_v6hdr->saddr), &(v6hdr->daddr), sizeof(struct in6_addr));
        rsp_v6hdr->version     = 6;
        rsp_v6hdr->hop_limit   = 64;
        rsp_v6hdr->nexthdr     = IPPROTO_UDP;
        rsp_v6hdr->payload_len = (udph->len);
        printk("[%s][%d]-ipv6---set ns make on skb-----\n",  __func__, __LINE__);
        //skb->ns_mark = 1;
    }

    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);
    eh = (struct ethhdr *)skb_push(skb, ETH_HLEN);       
    skb_reset_mac_header(skb);                           
    printk("[%s][%d]----skb len:%d-----\n", 
            __func__, __LINE__, skb->len);
    if (!is_v6) {
        skb->protocol = htons(ETH_P_IP);               
        eh->h_proto = htons(ETH_P_IP);                  
    } else {
        skb->protocol = htons(ETH_P_IPV6);               
        eh->h_proto = htons(ETH_P_IPV6);                  
    }
    memcpy(eh->h_source, smac, ETH_ALEN); 
    memcpy(eh->h_dest,   dmac, ETH_ALEN); 
    printk("[%s][%d]----pack set over. ssp skb len:%d-----\n", 
            __func__, __LINE__, skb->len);
    __dump_data(skb->data, skb->len, "all pkt");
    printk("[%s][%d]----skb len:%d-----\n",  __func__, __LINE__, skb->len);

    return skb;
}

static int __skb_build_skb_v4(void)
{
    __be32 saddr = 0; 
    __be32 daddr = 0; 
    struct net_device *dev = NULL;
    char msg_buf[128];
    int msg_len = 0;
    struct sk_buff *skb = NULL;
    unsigned char src_mac[6] = {0x00, 0x0C, 0x29, 0xFD, 0x87, 0xB3};
    //unsigned char dst_mac[6] = {0x00, 0x0C, 0x29, 0xFD, 0x87, 0xA9};
    unsigned char dst_mac[6] = {0x00, 0x0C, 0x29, 0xCE, 0x12, 0xE6};
    //00:0C:29:CE:12:E6

    dev = dev_get_by_name(&init_net, "eth0");
    if (NULL == dev) {
        return -1;
    }

    msg_len = snprintf(msg_buf, sizeof(msg_buf), "%s", "this is a msg.");

    saddr = _str2ip("192.168.1.1");
    daddr = _str2ip("192.168.1.120");
    skb = __skb_new_udp_pack(0, src_mac, dst_mac, daddr, saddr, htons(8819), htons(9918), msg_buf, msg_len + 1, NULL, 0, NULL);
    if (skb == NULL) {
        UP_MSG_PRINTF("build new udp pack failed.");
        return -1;
    }
    skb->dev = dev;
    dev_queue_xmit(skb);

    return 0;
}

static int __skb_build_skb_v6(void)
{
    struct net_device *dev = NULL;
    char msg_buf[128];
    int msg_len = 0;
    struct sk_buff *skb = NULL;
    //unsigned char src_mac[6] = {0x00, 0x0C, 0x29, 0xFD, 0x87, 0xB3};
    unsigned char src_mac[6] = {0x00, 0x0C, 0x29, 0xFD, 0x87, 0xA9};
    //unsigned char dst_mac[6] = {0x00, 0x0C, 0x29, 0xFD, 0x87, 0xA9};
    unsigned char dst_mac[6] = {0x00, 0x0C, 0x29, 0xCE, 0x12, 0xE6};
    //00:0C:29:CE:12:E6
    struct ipv6hdr v6hdr;
    //char v6_addr[64];
    char v6_saddr_buf[] = "2001:89:66:55::1";
    char v6_daddr_buf[] = "2001:89:66:55:20c:29ff:fece:12e6";

    dev = dev_get_by_name(&init_net, "eth0");
    if (NULL == dev) {
        return -1;
    }

    msg_len = snprintf(msg_buf, sizeof(msg_buf), "%s", "this is a msg.");

    memset(&v6hdr, 0, sizeof(struct ipv6hdr));
    in6_pton(v6_saddr_buf, strlen(v6_saddr_buf), v6hdr.saddr.s6_addr, '\0', NULL);
    __dump_data(v6hdr.saddr.s6_addr, 16, "v6 src addr");
    in6_pton(v6_daddr_buf, strlen(v6_daddr_buf), v6hdr.daddr.s6_addr, '\0', NULL);
    __dump_data(v6hdr.daddr.s6_addr, 16, "v6 dst addr");
    skb = __skb_new_udp_pack(1, src_mac, dst_mac, 0, 0, htons(8819), htons(41225), msg_buf, msg_len + 1, NULL, 0, &v6hdr);

    if (skb == NULL) {
        UP_MSG_PRINTF("build new udp pack failed.");
        return -1;
    }
    skb->dev = dev;
    dev_queue_xmit(skb);

    return 0;
}

static int __init skb_build_init(void)
{
    char *vm = NULL;
    unsigned int vm_phys;
    char *km = NULL;
    unsigned int km_phys;
    unsigned int hm_phys;

    //__skb_build_skb_v4();
    //__skb_build_skb_v6();
    UP_MSG_PRINTF("init success.");

    hm_phys = virt_to_phys(high_memory);
    printk("high_memory:0x%08X phys:%08X\n", (unsigned int)high_memory, hm_phys);
    vm = vmalloc(256 * 1024);
    if (vm) {
        printk("vmalloc success!\n");
        vm_phys = virt_to_phys(vm);
        printk("vm:0x%08X vm_phys:0x%08X\n", (unsigned int)vm, vm_phys);

        vfree(vm);
        vm = NULL;
    }

    km = kmalloc(1024, GFP_KERNEL);
    if (km) {
        printk("kmalloc success!\n");
        km_phys = virt_to_phys(km);
        printk("km:0x%08X km_phys:0x%08X\n", (unsigned int)km, km_phys);

        kfree(km);
        km = NULL;
    }
    


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

