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
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/netfilter/nf_conntrack_common.h>

#include "tcp_filter.h"

/*
typedef unsigned int nf_hookfn(unsigned int hooknum,
			       struct sk_buff *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn)(struct sk_buff *));
                   */
#define TF_DEBUG_EN 1
#if 1
#define TF_DEBUG(f, s...)   \
    do {                    \
        printk("TF [%s][%d]: " f "\n", __func__, __LINE__, ##s);  \
    }while(0)
#else
#define TF_DEBUG(f, s...) {}                    
#endif


typedef enum tf_data_type {
    TF_DATA_TYPE_NONE = 0,

    TF_DATA_TYPE_MARK,

    TF_DATA_TYPE_MAX,
}tf_data_type_en;

struct tf_data
{
    tf_data_type_en type;
    u_int32_t mark;
};


int nf_ct_flow_mark_set(struct sk_buff *skb, u_int32_t mark)
{
    struct tf_data *ct_tf = NULL;
    struct nf_conn *ct = NULL;
    enum ip_conntrack_info ctinfo;

    ct = nf_ct_get(skb, &ctinfo);
    if (ct == NULL) {
        return -1;
    }
    if (ct->p_data == NULL) {
        ct->p_data = kmalloc(sizeof(struct tf_data) ,GFP_ATOMIC); /* need kfree at destroy this conn */
        if (ct->p_data == NULL) {
            return -1;
        }
        ct_tf->type = TF_DATA_TYPE_MARK;
    }

    ct_tf = (struct tf_data *)(ct->p_data);
    if (ct_tf->type != TF_DATA_TYPE_MARK) {
        return -1;
    }

    ct_tf->mark |= mark;

    return 0;
}

#define ADDR_LEN      (6)

int _mac_addr_cmp(u_int8_t *src, u_int8_t *dst)
{
    int i;

    for (i = 0; i < ADDR_LEN; i++) {
        if (src[i] != dst[i])
            return 1;
    }

    return 0;
}

static int
tf_modify_tcp_option_timestamps(char *msg, int msg_len)
{
	int i = 0, s_val;
	char *s = msg, *ss;
	
	if (!msg||msg_len <= 0) {
		return 0;
	}
	
	while(i< msg_len) {
		ss = s+i;
		if (*ss == 8 && *(ss+1) == 10) {
			memcpy(ss+6, ss+2, 4);
			s_val = htonl(jiffies);			
			memcpy(ss+2, &s_val, 4);
			i += 10;
		} else if (*ss == 1 ||*ss == 0) {
			i += 1;
		} else {
			i += *(ss+1);
		}

	}

	return 0;
}

static int
tf_skb_iphdr_init(struct sk_buff *skb, u16 protocol, u32 saddr, u32 daddr, int len)
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

static struct sk_buff *
tf_tcp_new_pack(u32 saddr, u32 daddr, u16 sport, u16 dport,
		u32 seq, u32 ack_seq, u8 *msg, int msg_len, int syn, int ack, int push, int fin)
{
	int tcp_len, ip_len, eth_len, total_len, header_len;
	__wsum tcp_hdr_csum;
	struct sk_buff *skb = NULL;
	struct tcphdr *th = NULL;

	/* calculate len by protocals */
	tcp_len = msg_len + sizeof(struct tcphdr);
	ip_len = tcp_len + sizeof(struct iphdr);
	eth_len = ip_len + ETH_HLEN;
	total_len = eth_len + NET_IP_ALIGN;
	total_len += LL_MAX_HEADER;
	header_len = total_len - msg_len;

	/* alloc skb */
	if (!(skb = alloc_skb(total_len, GFP_KERNEL))) {
		TF_DEBUG("%s:alloc_skb failed!\n", __func__);
		return NULL;
	}
	/* reserve for header */
	skb_reserve(skb, header_len);
	/* copy payload data to skb */
	if (msg_len > 0) {
		skb_copy_to_linear_data(skb, msg, msg_len);
		skb->len += msg_len;
	}
	/* init tcp header info */
	skb_push(skb, sizeof(struct tcphdr));
	skb_reset_transport_header(skb);
	th = tcp_hdr(skb);
	skb->transport_header = (sk_buff_data_t)th;
	memset(th, 0x0, sizeof(struct tcphdr));
	th->doff = syn?tcp_len/4:5;
	th->source = sport;
	th->dest = dport;
	th->seq = seq;
	th->ack_seq = ack_seq;
	th->ack = ack;	
	th->psh = push;	
	th->rst = 0;
	th->fin = fin;/* NB: fin=1, different from url-redirect of url-filter.ko */
	th->syn = syn;	
	th->urg = 0;
	th->urg_ptr = 0;
	th->window = htons(13857); //TODO?why this num?
	tf_modify_tcp_option_timestamps((char *)(skb->transport_header + 20), th->doff*4-20);
	
	th->check = 0;
	skb->csum = 0;
	tcp_hdr_csum = csum_partial(th, tcp_len, 0);
	th->check = csum_tcpudp_magic(saddr,
		daddr, tcp_len, IPPROTO_TCP,tcp_hdr_csum);
	skb->csum = tcp_hdr_csum;
	if (!th->check)
		th->check = CSUM_MANGLED_0;

	tf_skb_iphdr_init(skb, IPPROTO_TCP, saddr, daddr, ip_len);

	return skb;
}

static int tf_tcp_send_syn_ack(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *th, char *out_dev_name)
{
	int tcp_len, seq = 0xf7bc4e8b/* should be random, fix it(0xf7bc4e8b) for wireshare display syn+ack ok */, syn = 1, ack = 1, push = 0, fin = 0;
	u32 ack_seq = 1;
	struct sk_buff *pskb = NULL;
	struct vlan_hdr *vh = NULL;
	struct ethhdr *eh = NULL;

	/* recalculate acknowledge sequeue number */
	tcp_len = ntohs(iph->tot_len) - ((iph->ihl + th->doff) << 2); /* X*4 */
	if (tcp_len == 0) {/* XXXXX: syn's tcp_len = 0!!! */
		tcp_len = 1;	/* for ack_seq +1, when ack for syn */
	}
	ack_seq = ntohl(th->seq) + tcp_len;
	ack_seq = htonl(ack_seq);

	skb->transport_header = ((void*)iph +(iph->ihl << 2));
	if(!(pskb = tf_tcp_new_pack(iph->daddr, 
                                iph->saddr, 
                                th->dest, 
                                th->source, 
                                seq, ack_seq, 
                                (char *)(skb->transport_header + 20), 
                                (th->doff * 4 - 20)/*sizeof(tcphdr)*/, 
                                syn, ack, push, fin))) {
		TF_DEBUG("%s:wpt_tcp_new_pack failed!\n", __func__);
		return -1;
	}

	/* copy vlan info if needed */
	if (skb->protocol == __constant_ntohs(ETH_P_8021Q)) {
		vh = (struct vlan_hdr *)skb_push(pskb, VLAN_HLEN);
		vh->h_vlan_TCI = vlan_eth_hdr(skb)->h_vlan_TCI;
		vh->h_vlan_encapsulated_proto = __constant_htons(ETH_P_8021Q);
	}

	/* copy eth heaer info */
	eh = (struct ethhdr *)skb_push(pskb, ETH_HLEN);
	skb_reset_mac_header(pskb);
	pskb->protocol = eth_hdr(skb)->h_proto;
	eh->h_proto = eth_hdr(skb)->h_proto;
	memcpy(eh->h_source, eth_hdr(skb)->h_dest, ETH_ALEN);
	memcpy(eh->h_dest, eth_hdr(skb)->h_source, ETH_ALEN);
	
	/* put skb to dev queue to xmit */
	if (out_dev_name) {
		pskb->dev = dev_get_by_name(&init_net, out_dev_name);
		if (pskb->dev) {
			dev_put(pskb->dev);
			if (dev_queue_xmit(pskb) < 0){
				//debug("%s dev_queue_xmit(%s) failed!\n", __func__, pskb->dev->name);
			} else {
				//debug("%s dev_queue_xmit(%s) ok!\n", __func__, pskb->dev->name);
			}
		} else {
            kfree_skb(pskb);
            //printk(KERN_ERR"%s failed! out_dev_name[%s]\n", __func__, out_dev_name);
            return -1;
        }
		return 0;
	} else {
		kfree_skb(pskb);
		TF_DEBUG("%s failed!\n", __func__);
		return -1;
	}
	
	return 0;
}

static char ignore_mac[6] = {0x00,0x50,0x56,0xC0,0x00,0x10};

static unsigned int tf_L3_dnat(unsigned int hooknum,
				      struct sk_buff *skb,
				      const struct net_device *in,
				      const struct net_device *out,
				      int (*okfn)(struct sk_buff *))
{
    int ret = NF_DROP;
    int head_len = 0;
    int data_len = 0;
    char *buf = NULL;
    struct ethhdr *eh = eth_hdr(skb); 
    struct iphdr  *ih = ip_hdr(skb);
    struct tcphdr *th = NULL;

    if (_mac_addr_cmp(eh->h_dest, ignore_mac) == 0 || _mac_addr_cmp(eh->h_source, ignore_mac) == 0) {
        //TF_DEBUG("ignore mac accept.");
        return NF_ACCEPT;
    }

    if (skb->protocol != htons(ETH_P_IP) && skb->protocol != htons(ETH_P_8021Q)) {
        TF_DEBUG("not ip drop.");
        return NF_DROP; //TODO:or DROP?
    }

    if (skb->protocol == htons(ETH_P_8021Q) && ih) {
        ih = (struct iphdr *)((u8 *)ih + 4);
    }

    if (ih->protocol != IPPROTO_TCP) { //TODO?Why not use htons
        TF_DEBUG("not tcp drop.");
        return NF_DROP; //TODO:or DROP?
    }

    th = (struct tcphdr *)((u8 *)ih + (ih->ihl * 4));           
    if (ntohs(th->dest) != 80 && ntohs(th->source) != 80) {
        TF_DEBUG("not http drop.");
        return NF_DROP;
    }

    head_len = (ih->ihl * 4)  + (th->doff * 4);
    data_len = ntohs(ih->tot_len) - head_len;
    buf = skb->data + head_len; /* http data */

    if (th->rst) {
        TF_DEBUG("TCP RST accept.");
        return NF_ACCEPT;
    }



    return NF_ACCEPT;
}

static struct nf_hook_ops tcp_filter_ops[] __read_mostly = {
    {
        .owner = THIS_MODULE,
        .hooknum = NF_INET_PRE_ROUTING, 
        .pf = PF_INET,/* for lay-3 */
        .priority = NF_IP_PRI_CONNTRACK+1,  /* make sure conntrack prepare ok.*/
        .hook = (nf_hookfn *)tf_L3_dnat, /* redirect to portal server */
    },
};

static int __init tcp_filter_init(void)
{
    //__skb_build_skb_v4();
    nf_register_hooks(tcp_filter_ops, ARRAY_SIZE(tcp_filter_ops));

    TF_MSG_PRINTF("init success.");

    return 0;
}

static void __exit tcp_filter_exit(void)
{

    /* without this, hooks will still do, buf will kernel panic */
	nf_unregister_hooks(tcp_filter_ops, ARRAY_SIZE(tcp_filter_ops)); 

    TF_MSG_PRINTF("exit success.");

    return;
}

module_init(tcp_filter_init);
module_exit(tcp_filter_exit);

MODULE_AUTHOR("G");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("skb build");

