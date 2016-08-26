/*
 *  Dns intercept.
 *
 *  Authors:
 *  Yetao <yetao@bhunetworks.com>
 *
 *  Copyright (C) 2015 bhunetworks.com
 *
 */
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
#include <net/addrconf.h>
#include <linux/timer.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
#include "../net/bridge/br_private.h"
#else
#include "./../../kernels/mips-linux-2.6.31/net/bridge/br_private.h"/* FixMe: is it good ? */
#endif

#include "dns_intercept.h"
#include "dns_intercept_sysfs.h"

MODULE_AUTHOR("Yetao");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bhu dns interception module");

#define MAXDNAME    256        /* maximum presentation domain name */

#define OP_QUERY           0                /* opcode, standard query */
#define OP_IQUERY          1                /* opcode, inverse query */
#define OP_STATUS          2                /* opcode, server status request */


#define C_IN            1               /* the arpa internet */
#define C_CHAOS         3               /* for chaos net (MIT) */
#define C_ANY           255             /* wildcard match */

#define T_A		1
#define T_NS            2               
#define T_CNAME		5
#define T_SOA		6
#define T_PTR		12
#define T_MX		15
#define T_TXT		16
#define T_SIG		24
#define T_AAAA		28
#define T_SRV		33
#define T_NAPTR		35
#define T_OPT		41
#define	T_TKEY		249		
#define	T_TSIG		250
#define T_MAILB		253	
#define T_ANY		255

spinlock_t bhudns_lock;
struct list_head name_list;
int bhudns_enable;
int bhudns_debug;

static int
bhudns_get_lan_if_ip(const struct net_device *indev, u32 *ip, u32 userip);

static void
bhudns_dump_packet(u8 *ptr, int len)
{
    int i;
    /*
    if(bhudns_debug <= 1)
        return;
        */
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printk("\n %04x", i);
        printk(" %02x", ptr[i]);
    }
    printk("\n");
}

#define debug(f, s...) {\
        if(bhudns_debug) printk("Dns-L2 " f, ##s);\
	}

#define dbg(f, s...) {\
		printk("Dns-L2 " f, ##s);\
	}


struct dns_rsp_hook {
	DNS_RSP_HOOK hook;
	void	*args;
};

struct dns_rsp_hook_list {
    struct list_head	list;
	struct dns_rsp_hook	hook;
};
struct list_head hook_list;

int
dns_reg_hook(DNS_RSP_HOOK hook, void *args)
{
    struct dns_rsp_hook_list *hooklist;
    list_for_each_entry(hooklist, &hook_list, list) {
		if (hooklist->hook.hook == hook)
			return -EEXIST;
	}
    hooklist = kzalloc(sizeof(*hooklist), GFP_KERNEL);
    if (hooklist == NULL) 
        return -ENOMEM;

    hooklist->hook.hook = hook;
    hooklist->hook.args = args;

    list_add_tail(&hooklist->list, &hook_list);
    return 0;
}
EXPORT_SYMBOL(dns_reg_hook);

int
dns_unreg_hook(DNS_RSP_HOOK hook)
{
    struct dns_rsp_hook_list *hooklist = NULL;

    list_for_each_entry(hooklist, &hook_list, list) {
		if (hooklist->hook.hook == hook)
			break;
	}

	if (hooklist == NULL || hooklist->hook.hook != hook)
		return -EEXIST;

	list_del(&hooklist->list);
	kfree(hooklist);

    return 0;
}
EXPORT_SYMBOL(dns_unreg_hook);


static int
extract_domain_name(char *str, char *name, int maxlen)
{
    char *p = str;
    char *q = name;
    int len = 0;
    while(*p != 0 && *p < 32) p++;
    while(*p != 0 && len < maxlen){
        if(*p < 32)
            *q = '.';
        else
            *q = *p;
        len ++;
        p ++;
        q ++;
    }
    *q = 0;
    return 0;
}

#define GETSHORT(s, cp) { \
	unsigned char *t_cp = (unsigned char *)(cp); \
	(s) = ((u16)t_cp[0] << 8) \
	    | ((u16)t_cp[1]) \
	    ; \
	(cp) += 2; \
}
#define CHECK_LEN(header, pp, plen, len) \
    ((size_t)((pp) - (unsigned char *)(header) + (len)) <= (plen))

/* extract as .-concatenated string into name */
static int 
extract_name(struct bhu_dns_hdr *header, size_t plen, unsigned char **pp, 
        char *name, int isExtract, int extrabytes)
{
    unsigned char *cp = (unsigned char *)name, *p = *pp, *p1 = NULL;
    unsigned int j, l, hops = 0;
    int retvalue = 1;

    if (isExtract)
        *cp = 0;

    while (1)
    { 
        unsigned int label_type;

        if (!CHECK_LEN(header, p, plen, 1))
            return 0;

        if ((l = *p++) == 0) 
            /* end marker */
        {
            /* check that there are the correct no of bytes after the name */
            if (!CHECK_LEN(header, p, plen, extrabytes))
                return 0;

            if (isExtract)
            {
                if (cp != (unsigned char *)name)
                    cp--;
                *cp = 0; /* terminate: lose final period */
            }
            else if (*cp != 0)
                retvalue = 2;

            if (p1) /* we jumped via compression */
                *pp = p1;
            else
                *pp = p;

            return retvalue;
        }

        label_type = l & 0xc0;

        if (label_type == 0xc0) /* pointer */
        { 
            if (!CHECK_LEN(header, p, plen, 1))
                return 0;

            /* get offset */
            l = (l&0x3f) << 8;
            l |= *p++;

            if (!p1) /* first jump, save location to go back to */
                p1 = p;

            hops++; /* break malicious infinite loops */
            if (hops > 255)
                return 0;

            p = l + (unsigned char *)header;
        }
        else if (label_type == 0x80)
            return 0; /* reserved */
        else if (label_type == 0x40)
            return 0; /* reserved */
        else 
        { /* label_type = 0 -> label. */
            if (cp - (unsigned char *)name + l + 1 >= MAXDNAME)
                return 0;
            if (!CHECK_LEN(header, p, plen, l))
                return 0;

            for(j=0; j<l; j++, p++)
                if (isExtract)
                {
                    unsigned char c = *p;
                    if (isascii(c) && !iscntrl(c) && c != '.')
                        *cp++ = *p;
                    else
                        return 0;
                }
                else 
                {
                    unsigned char c1 = *cp, c2 = *p;

                    if (c1 == 0)
                        retvalue = 2;
                    else 
                    {
                        cp++;
                        if (c1 >= 'A' && c1 <= 'Z')
                            c1 += 'a' - 'A';
                        if (c2 >= 'A' && c2 <= 'Z')
                            c2 += 'a' - 'A';

                        if (c1 != c2)
                            retvalue =  2;
                    }
                }

            if (isExtract)
                *cp++ = '.';
            else if (*cp != 0 && *cp++ != '.')
                retvalue = 2;
        }
    }
}

extern char *
ip2str (u32 ipv4, char buf[18]);

int
bhudns_handle_dns_rsp(struct bhu_dns_hdr *dh, u32 pktlen)
{
    struct dns_rsp_hook_list *hooklist = NULL;
    int ret = 0;
    char domain[256] = {0};
#define MAX_IP_NUM 10
    u32 ip[MAX_IP_NUM];
    u32 ipnum = 0;
    unsigned char *p = (unsigned char *)(dh+1);
    int qtype, qclass;
    int i;
    char buf[18];
    union flag_union flag;

    flag.unit = ntohs(dh->flag.unit);

    if (ntohs(dh->qdcount) != 1) {//TODO:discard dns rsp when questions num > 1
        debug("qdcount(%d) != 1\n", ntohs(dh->qdcount));
        return 0;
    }
    if (ntohs(dh->ancount) < 1) {
        debug("ancount(%d) < 1\n", ntohs(dh->ancount));
        return 0;
    }
    if (flag.bits.op != OP_QUERY) {
        debug("OP(%d) != QUERY\n", flag.bits.op);
        return 0;
    }
    if (flag.bits.rcode != 0) {
        debug("rcode(%d) != 0\n", flag.bits.rcode);
        return 0;
    }
    if (!extract_name(dh, pktlen, &p, domain, 1, 4)) {
        debug("extract name fail!\n");
        return 0;
    }
    debug("domain:%s\n", domain);
    
    GETSHORT(qtype, p);
    GETSHORT(qclass, p);

    if (qtype != T_A) {
        debug("not ipv4\n");
        return 0;
    }

    if (qclass != C_IN) {
        debug("not inet\n");
        return 0;
    }
    
    for (i=0; i<htons(dh->ancount) && i<MAX_IP_NUM; i++) {
        struct dns_response *rsp = (struct dns_response *)p;
        if (ntohs(rsp->type) == T_A) {
            ip[ipnum] = ntohl(rsp->ip);        
            debug("ip:%s\n", ip2str(ip[ipnum], buf));
            ++ipnum;
        }
        p += 12 + ntohs(rsp->len);
    }

    if (ipnum == 0) {
        debug("ip num 0\n");
        return 0;
    }
    list_for_each_entry(hooklist, &hook_list, list) {
		if (hooklist->hook.hook) {
            ret = hooklist->hook.hook(hooklist->hook.args, dh, domain, ip, ipnum);
            if (ret != 0) 
                return -1;
	    }
	}
    return 0;
}

int __get_lan_if_ipv6(const struct net_device *indev, u32 *ip, const struct in6_addr *saddr)                            
{
    const struct net_device *dev;                                                                                        
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)                                                                          
    struct net_bridge_port *p = NULL;                                                                                    
#endif                                                                                                                   
    //char buf[128];                                                                                                       
    struct net *net = NULL;                                                                                              
    struct inet6_ifaddr *in6_ifaddr = NULL;                                                                              

    if (!indev||!ip) {                                                                                                   
        return -1;                                                                                                       
    }                                                                                                                    

    dev = indev;                                                                                                         
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)                                                                          
    p = br_port_get_rcu(dev);                                                                                            
    if (p && p->br && p->br->dev)                                                                                        
        dev = p->br->dev;                                                                                                
#else                                                                                                                    
    if (dev->br_port && dev->br_port->br && dev->br_port->br->dev)                                                       
        dev = dev->br_port->br->dev;                                                                                     
#endif                                                                                                                   

    net = dev_net(dev);                                                                                                  
    //in6_ifaddr = ipv6_get_ifaddr(net, 0, (struct net_device *)indev, 1);                                                                
    if (in6_ifaddr != NULL) {                                                                                            
        //snprintf(buf, sizeof(buf), "%x", in6_ifaddr->addr);                                                              
        bhudns_dump_packet((u8 *)&(in6_ifaddr->addr), 16);
    }                                                                                                                    

    //return bhudns_get_if_ipv6(dev, ip, saddr);                                                                         
    return 0;                                                                                                            
}                                                                                                                        

unsigned int __bhudns_skb_intercept_v6(struct sk_buff *skb, const struct net_device *in)
{
    u8 nexthdr = 0;
    int offset = 0;
    __be16 frag_off;
    const struct ipv6hdr *hdr = NULL;
	struct udphdr *uh = NULL;
    u32 ip = 0;


    printk("[%s][%d]----------\n", __func__, __LINE__);
    /* copy from "linux kernel fund:ip6_mc_input()"*/
	hdr = ipv6_hdr(skb);
    nexthdr = hdr->nexthdr;
    offset = ipv6_skip_exthdr(skb, sizeof(*hdr),
            &nexthdr, &frag_off);

    //if (ntohs(nexthdr) == IPPROTO_UDP) {
    if (nexthdr == IPPROTO_UDP) {
        //offset = 0;
        bhudns_dump_packet(skb->data, skb->len);
        printk("[%s][%d]-----offset=%d-----\n", __func__, __LINE__, offset);
        uh = (struct udphdr*)(skb_network_header(skb) + offset);
        //uh = (struct udphdr*)(((u8 *)hdr) + offset);
        //uh = (struct udphdr*)((u8*)ih + ip_hdrlen(skb));
        printk("[%s][%d]-----source:%d dest:%d-----\n", __func__, __LINE__, ntohs(uh->source), ntohs(uh->dest));
        __get_lan_if_ipv6(in, &ip,  &(hdr->saddr));
    }

    return 0;
}

unsigned int bhudns_skb_intercept_handle(
	unsigned int hooknum,
	struct sk_buff * skb,
	const struct net_device *in,
	const struct net_device *out,
	int(*fn)(struct sk_buff *)
	)
{
	struct ethhdr *eh = NULL;
	struct iphdr *ih = NULL;
	struct udphdr *uh = NULL;
    struct bhu_dns_hdr *dh = NULL;
	u16	ethtype = 0;
    char buf[256] = {0};
    struct name_node *node = NULL;
    struct dns_response answer;
    char *p = NULL;
    __be32 addr = 0;
    union flag_union flag;
    int _is_v6 = 0;
    u8 nexthdr = 0;
    int offset = 0;
    __be16 frag_off;
    struct ipv6hdr *v6hdr = NULL;
    struct in6_addr v6addr;

    if(!bhudns_enable)
        return NF_ACCEPT;

	eh = eth_hdr(skb);

	if(!skb  || !eh)
		return NF_ACCEPT;

	ethtype = eh->h_proto;

	if (skb->protocol==htons(ETH_P_IP)) {
		ih = ip_hdr(skb);
	} else if (skb->protocol==htons(ETH_P_IPV6)) {
        //__bhudns_skb_intercept_v6(skb, in);
        _is_v6 = 1;
        printk("[%s][%d]----is ipv6------\n", __func__, __LINE__);
	}
	
    if (!_is_v6) {
        if(skb->protocol==htons(ETH_P_8021Q) && ih) {
            ih = (struct iphdr *)((u8*)ih+4);
            ethtype = *(u16 *)((u8*)ih+2);
        }
        if (!ih || ih->protocol != IPPROTO_UDP) 
            return NF_ACCEPT;

        uh = (struct udphdr*)((u8*)ih + ip_hdrlen(skb));
    } else {
        v6hdr = ipv6_hdr(skb);
        nexthdr = v6hdr->nexthdr;
        offset = ipv6_skip_exthdr(skb, sizeof(*v6hdr),
                &nexthdr, &frag_off);
        if (nexthdr != IPPROTO_UDP) {
            printk("[%s][%d]----not UDP pkt------\n", __func__, __LINE__);
            return NF_ACCEPT;
        }
        //bhudns_dump_packet(skb->data, skb->len);
        uh = (struct udphdr*)(skb_network_header(skb) + offset);
        printk("[%s][%d]-----source:%d dest:%d-----\n", __func__, __LINE__, uh->source, uh->dest);
    }


    if (!_is_v6) {
        if(uh->dest != htons(53) && uh->source != htons(53)) { //not dns packet
            return NF_ACCEPT;
        }
    }

    dh = (struct bhu_dns_hdr *)((u8*)uh + sizeof(*uh));
    flag.unit = ntohs(dh->flag.unit);
    if ((uh->dest == htons(53) && flag.bits.qr == 0) || _is_v6) {//dns query packet
        if (!_is_v6) {
            debug("got dns query pkt, skb:%p, dev:%s, skb_len:%d, proto:%d, eh:%p, ih:%p, uh:%p\n", skb, skb->dev->name, skb->len, skb->protocol, eh, ih, uh);
        } else {
            printk("[%s][%d]----is ipv6------\n", __func__, __LINE__);
            debug("got dns query pkt, skb:%p, dev:%s, skb_len:%d, proto:%d, eh:%p, v6hdr:%p, uh:%p\n", skb, skb->dev->name, skb->len, skb->protocol, eh, v6hdr, uh);
        }
        bhudns_dump_packet(skb->data, skb->len);
debug("dh->qdcount:%d\n", ntohs(dh->qdcount));
        if(ntohs(dh->qdcount) != 1) {//questions should be 1 
            return NF_ACCEPT;
        }

debug("dh->ancount:%d\n", ntohs(dh->ancount));
        if(ntohs(dh->ancount)) //contain answers , maybe it's not a dns packet
            return NF_ACCEPT;

        if(extract_domain_name((char*)dh + sizeof(*dh), buf, sizeof(buf) - 1))
            return NF_ACCEPT;

        debug("skb:%p, query for:%s\n", skb, buf);

        spin_lock(&bhudns_lock);

        if (!_is_v6) { //FOR TEST!!!!!!
            if(!(node = bhudns_match_node(buf)))
                goto unlock_accept;
        }

        //send fake response now
        memset(&answer, 0, sizeof(answer));
        answer.name = ntohs(0xc00c);
        answer.type = ntohs(0x01);
        answer.class = ntohs(0x01);
        answer.ttl = 0;
        answer.len = ntohs(4);
        if((!_is_v6/* TEST !!! */) && node->ip){
            answer.ip = htonl(node->ip);
        } else{
            if (!_is_v6) {
                if(bhudns_get_lan_if_ip(in, &answer.ip, ntohl(ih->saddr)))
                    goto unlock_accept;
                if(answer.ip == 0)
                    goto unlock_accept;
                answer.ip = htonl(answer.ip);
            } else {
                printk("[%s][%d]----is ipv6 intercept to 192.168.62.1------\n", __func__, __LINE__);
                answer.ip = htonl(0xC0A83E01);
            }
        }
        spin_unlock(&bhudns_lock);

        if(!(p = skb_put(skb, sizeof(answer))))
            return NF_ACCEPT;
        flag.bits.qr = 1;
        flag.bits.aa = 1;
        flag.bits.ra = 1;
        dh->flag.unit = htons(flag.unit);
        dh->ancount = htons(1);
        memcpy(p, &answer, sizeof(answer));

        uh->len = htons(ntohs(uh->len) + sizeof(answer));
        uh->check = uh->dest;
        uh->dest = uh->source;
        uh->source = uh->check;
        uh->check = 0;

        if (!_is_v6) {
            ih->tot_len = htons(ntohs(ih->tot_len) + sizeof(answer));
            addr = ih->saddr;
            ih->saddr = ih->daddr;
            ih->daddr = addr;
            ih->check = 0;
            ih->id = 0;
            ip_send_check(ih);
        } else {
            memcpy(&v6addr,         &(v6hdr->saddr), sizeof(struct in6_addr));
            memcpy(&(v6hdr->saddr), &(v6hdr->daddr), sizeof(struct in6_addr));
            memcpy(&(v6hdr->daddr), &(v6addr),       sizeof(struct in6_addr));
            v6hdr->payload_len = uh->len;
        }

        memcpy(buf, eh->h_dest, ETH_ALEN);
        memcpy(eh->h_dest, eh->h_source, ETH_ALEN);
        memcpy(eh->h_source, buf, ETH_ALEN);

        if (_is_v6) {
            /* copy from udp_v6_push_pending_frames() */
            __wsum csum = csum_partial(skb_transport_header(skb),
                    sizeof(struct udphdr), 0);
            uh->check = csum_ipv6_magic(&(v6hdr->saddr), &(v6hdr->daddr),
                    uh->len, IPPROTO_UDP, csum);
        }

        /* when caculate checksum, win can't get response. set it to 0, windows ok.
         * uh->check = csum_tcpudp_magic(ih->saddr, ih->daddr,
         ntohs(uh->len) - sizeof(*uh), IPPROTO_UDP,
         csum_partial(uh, ntohs(uh->len) - sizeof(*uh), 0));
         */
        skb_push(skb, 14);
        if (!_is_v6) {
            if (eh->h_proto == htons(ETH_P_8021Q))
                skb_push(skb, 4);
        }

        if (!_is_v6) {
            debug("skb:%p, send response, skb_len:%d, protocol:%d, eh:%p, ih:%p, uh:%p\n", skb, skb->len, skb->protocol, eh, ih, uh);
        } else {
            debug("skb:%p, send response, skb_len:%d, protocol:%d, eh:%p, v6hdr:%p, uh:%p\n", skb, skb->len, skb->protocol, eh, v6hdr, uh);
        }
        bhudns_dump_packet(skb->data, skb->len);
        dev_queue_xmit(skb);
        return NF_STOLEN;
unlock_accept:
        spin_unlock(&bhudns_lock);
    } else if (uh->source == htons(53) && flag.bits.qr) {//dns resp packet
        debug("got dns rsp pkt, skb:%p, dev:%s, skb_len:%d, proto:%d, eh:%p, ih:%p, uh:%p\n", skb, skb->dev->name, skb->len, skb->protocol, eh, ih, uh);
        bhudns_dump_packet(skb->data, skb->len);
        bhudns_handle_dns_rsp(dh, uh->len-sizeof(struct udphdr));
    }
    return NF_ACCEPT;
}

/*
int
wpt_skb_iphdr_init(struct sk_buff *skb, u16 protocol, u32 saddr, u32 daddr, int len)
{
	struct iphdr *iph = NULL;
	skb_push(skb, sizeof(struct iphdr));
	skb_reset_network_header(skb);
	iph = ip_hdr(skb);
	memset(iph, 0, sizeof(struct iphdr));
	iph->protocol = protocol;
	iph->version = 4;
	iph->ihl = 5;
	iph->saddr = saddr;
	iph->daddr = daddr;
	iph->ttl = 53;
	iph->id = 0; 
	iph->tos = 0;
	iph->tot_len = htons(len);
	iph->frag_off = htons(IP_DF);
	iph->check = 0;
	iph->check =  ip_fast_csum((unsigned char *)iph, iph->ihl);       
	return 0;
}
*/

static int
bhudns_get_if_ip(const struct net_device *dev, u32 *ip, u32 userip) 
{
	struct in_device *in_dev;
    struct in_ifaddr *pip = NULL;
	
	if (!dev) {
		return -1;
	}
	
	in_dev = in_dev_get(dev);
	if (in_dev) {
		if (in_dev->ifa_list) {
			*ip = ntohl(in_dev->ifa_list->ifa_address);
            pip = in_dev->ifa_list;
            while (pip) {
                char uip[16];
                char addr[16];
                char mask[16];
                debug("userip:%s, if_address:%s, mask:%s\n", 
                        ip2str(userip, uip), 
                        ip2str(ntohl(pip->ifa_address), addr),
                        ip2str(ntohl(pip->ifa_mask), mask));
                if ((userip & ntohl(pip->ifa_mask)) == (ntohl(pip->ifa_address) & ntohl(pip->ifa_mask))) {
                    debug("hit %s\n", addr);
                    *ip = ntohl(pip->ifa_address);
                    break;
                }
                pip = pip->ifa_next;
            }
		}	
		in_dev_put(in_dev);
	}
	return 0;
}

static int
bhudns_get_lan_if_ip(const struct net_device *indev, u32 *ip, u32 userip)
{
	const struct net_device *dev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
    struct net_bridge_port *p = NULL;
#endif
	
	if (!indev||!ip) {
		return -1;
	}

    dev = indev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
    p = br_port_get_rcu(dev);
    if (p && p->br && p->br->dev)
        dev = p->br->dev;
#else
    if (dev->br_port && dev->br_port->br && dev->br_port->br->dev)
        dev = dev->br_port->br->dev;
#endif

    return bhudns_get_if_ip(dev, ip, userip);
}

char * 
strdup(const char *s)
{
    size_t  len = strlen(s) +1;   
    void *new = kmalloc(len, GFP_ATOMIC);   
    if (new == NULL)      
        return NULL;   
    return (char *)memcpy(new,s,len);
} 

struct name_node *
bhudns_match_node(char *name)
{
	struct name_node *p;
    if(!name)
        return NULL;
	list_for_each_entry(p, &name_list, node) {
		if (p->name && strcasecmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

int
bhudns_add_name_node(char *name, u32 ip)
{
	struct name_node *p, *n;
    char buf[18];
    if(!name)
        return -1;
	list_for_each_entry_safe(p, n, &name_list, node) {
		if (p->name && strcasecmp(p->name, name) == 0){
            p->ip = ip;
            return 0;
        }
	}
    if(!(p = kmalloc(sizeof(*p), GFP_ATOMIC)))
        return -1;
    memset(p, 0, sizeof(*p));
    p->name = strdup(name);
    p->ip = ip;
    list_add_tail(&p->node, &name_list);
    debug("add dns %s %s\n", name, ip2str(ip, buf));
    return 0;
}

void
bhudns_remove_name_node(char *name)
{
	struct name_node *p, *n;
    if(!name)
        return;
	list_for_each_entry_safe(p, n, &name_list, node) {
		if (p->name && strcasecmp(p->name, name) == 0){
			kfree(p->name);
            list_del_init(&p->node);
            kfree(p);
            break;
        }
	}
}

void 
bhudns_clear_name_node_list(void)
{
	struct name_node *p, *n;
	list_for_each_entry_safe(p, n, &name_list, node) {
		if (p->name)
			kfree(p->name);
		list_del_init(&p->node);
		kfree(p);
	}
}

void 
bhudns_clear_hook_list(void)
{
	struct dns_rsp_hook_list *p, *n;
	list_for_each_entry_safe(p, n, &hook_list, list) {
		list_del_init(&p->list);
		kfree(p);
	}
}

static struct nf_hook_ops bhudns_hooks[] = {
	{
		.owner = THIS_MODULE,
		.hooknum = NF_BR_PRE_ROUTING, 	
		.pf = PF_BRIDGE,
		.priority = NF_BR_PRI_FIRST,
		.hook = (nf_hookfn *)bhudns_skb_intercept_handle,
	},
	{
		.owner = THIS_MODULE,
		.hooknum = NF_BR_PRE_ROUTING,	
		.pf = PF_INET,
		.priority = NF_IP_PRI_FIRST,
		.hook = (nf_hookfn *)bhudns_skb_intercept_handle,
	},
};

static int __init bhudns_init(void)
{
	struct kobject *obj = &(THIS_MODULE->mkobj.kobj);

	INIT_LIST_HEAD(&name_list);
    INIT_LIST_HEAD(&hook_list);
    spin_lock_init(&bhudns_lock);
    bhudns_enable = 1;
    bhudns_debug = 1;
    spin_lock(&bhudns_lock);
    bhudns_add_name_node("bhuwifi.com", 0);
    spin_unlock(&bhudns_lock);

    if(bhudns_add_sysfs(obj)){
		pr_info("%s: can't create bhudns sysfs\n",__func__);
		return -1;
	}

	nf_register_hooks(bhudns_hooks, ARRAY_SIZE(bhudns_hooks));

	return 0;
}

static void __exit bhudns_exit(void)
{
	struct kobject *obj = &(THIS_MODULE->mkobj.kobj);

	nf_unregister_hooks(bhudns_hooks, ARRAY_SIZE(bhudns_hooks));

	bhudns_remove_sysfs(obj);

    bhudns_clear_name_node_list();

    bhudns_clear_hook_list();
}

module_init(bhudns_init);
module_exit(bhudns_exit);

