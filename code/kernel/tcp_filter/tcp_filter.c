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

spinlock_t g_tf_lock; /* = SPIN_LOCK_UNLOCKED*/;

#define TF_LOCK \
    do { \
        spin_lock(&g_tf_lock); \
    }while(0)
#define TF_UNLOCK \
    do { \
        spin_unlock(&g_tf_lock); \
    }while(0)

int nf_ct_flow_mark_get(struct sk_buff *skb, u_int32_t *mark);

/* NOTE:can never be inlined because it uses variable argument lists */
static int
_tf_skb_makr_log(const char *f, int l, struct sk_buff *skb, const char *fmt, ...)
{
	int i = 0;
    u_int32_t mark;
    char buf[128];
	va_list args;

    if (nf_ct_flow_mark_get(skb, &mark) != 0) {
        TF_DEBUG("get nf ct flow mark failed.");
        return NF_DROP;
    }

	va_start(args, fmt);
	i = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    i += snprintf(buf + i, sizeof(buf) - i, "%s", "-----mark:");

    if (mark & TF_SKB_MARK_NEED_FAKE_FIN) {
        i += snprintf(buf + i, sizeof(buf) - i, "%s", "[NEED FAKE FIN]");
    }
    if (mark & TF_SKB_MARK_NORMAL_PKT) {
        i += snprintf(buf + i, sizeof(buf) - i, "%s", "[NORMAL PKT]");
    }
    if (mark & TF_SKB_MARK_STA_HAS_SENT_SYN) {
        i += snprintf(buf + i, sizeof(buf) - i, "%s", "[HAS SENT SYN]");
    }
    i += snprintf(buf + i, sizeof(buf) - i, "%s", "-----\n");

    printk("[%s][%d]:%s\n", f, l, buf);

    return 0;
}

#define TF_SKB_MARK_LOG(skb, f, s...) \
    _tf_skb_makr_log(__func__, __LINE__, skb, f, ##s);

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
        TF_DEBUG("skb without nfct.");
        return -1;
    }
    if (ct->p_data == NULL) {
        ct->p_data = kmalloc(sizeof(struct tf_data) ,GFP_ATOMIC); /* need kfree at destroy this conn */
        if (ct->p_data == NULL) {
            TF_DEBUG("oom.");
            return -1;
        }
        OS_MemZeroSet(ct->p_data, sizeof(struct tf_data));
        ct_tf = (struct tf_data*)(ct->p_data);
        ct_tf->type = TF_DATA_TYPE_MARK;
    }

    ct_tf = (struct tf_data *)(ct->p_data);
    if (ct_tf->type != TF_DATA_TYPE_MARK) {
        return -1;
    }

    ct_tf->mark |= mark;

    return 0;
}

int nf_ct_flow_mark_get(struct sk_buff *skb, u_int32_t *mark)
{
    struct tf_data *ct_tf = NULL;
    struct nf_conn *ct = NULL;
    enum ip_conntrack_info ctinfo;
    ct = nf_ct_get(skb, &ctinfo);
    if (ct == NULL) {
        TF_DEBUG("get nf ct failed");
        return -1;
    }
    if (ct->p_data == NULL) {
        TF_DEBUG("nf ct data is NULL");
        if (nf_ct_flow_mark_set(skb, TF_SKB_MARK_INIT) < 0) {
            TF_DEBUG("nf ct data init failed.");
            return -1;
        }
    }

    ct_tf = (struct tf_data *)(ct->p_data);
    if (ct_tf->type != TF_DATA_TYPE_MARK) {
        TF_DEBUG("nf ct data is not mark data");
        return -1;
    }

    *mark = ct_tf->mark;

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
tf_destroy_skb_conntrack(struct sk_buff *skb)
{
    struct nf_conn *ct = NULL;
    enum ip_conntrack_info ctinfo;

    ct = nf_ct_get(skb, &ctinfo);
    if (ct) {
        nf_ct_kill_acct(ct, ctinfo, skb);
    } else {
        TF_DEBUG("ct is NULL.\n");
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
tf_tcp_new_pack(struct sk_buff *iskb, struct ethhdr *eh, u32 saddr, u32 daddr, u16 sport, u16 dport,
		u32 seq, u32 ack_seq, u8 *msg, int msg_len, int syn, int ack, int push, int fin)
{
	int tcp_len, ip_len, eth_len, total_len, header_len;
	__wsum tcp_hdr_csum;
	struct sk_buff *new_skb = NULL;
	struct tcphdr   *new_th = NULL;
    struct vlan_hdr *new_vh = NULL;

	/* calculate len by protocals */
	tcp_len = msg_len + sizeof(struct tcphdr);
	ip_len = tcp_len + sizeof(struct iphdr);
	eth_len = ip_len + ETH_HLEN;
	total_len = eth_len + NET_IP_ALIGN;
	total_len += LL_MAX_HEADER;
	header_len = total_len - msg_len;

	/* alloc new_skb */
	if (!(new_skb = alloc_skb(total_len, GFP_KERNEL))) {
		TF_DEBUG("%s:alloc_skb failed!\n", __func__);
		return NULL;
	}
	/* reserve for header */
	skb_reserve(new_skb, header_len);
	/* copy payload data to new_skb */
	if (msg_len > 0) {
		skb_copy_to_linear_data(new_skb, msg, msg_len);
		new_skb->len += msg_len;
	}
	/* init tcp header info */
	skb_push(new_skb, sizeof(struct tcphdr));
	skb_reset_transport_header(new_skb);
	new_th = tcp_hdr(new_skb);
	new_skb->transport_header = (__u16)(unsigned long)new_th;
	memset(new_th, 0x0, sizeof(struct tcphdr));
	new_th->doff = syn?tcp_len/4:5;
	new_th->source = sport;
	new_th->dest = dport;
	new_th->seq = seq;
	new_th->ack_seq = ack_seq;
	new_th->ack = ack;	
	new_th->psh = push;	
	new_th->rst = 0;
	new_th->fin = fin;/* NB: fin=1, different from url-redirect of url-filter.ko */
	new_th->syn = syn;	
	new_th->urg = 0;
	new_th->urg_ptr = 0;
	new_th->window = htons(13857); //TODO?why this num?
	tf_modify_tcp_option_timestamps((char *)(new_skb->transport_header + 20), new_th->doff*4-20);
	
	new_th->check = 0;
	new_skb->csum = 0;
	tcp_hdr_csum = csum_partial(new_th, tcp_len, 0);
	new_th->check = csum_tcpudp_magic(saddr,
		daddr, tcp_len, IPPROTO_TCP,tcp_hdr_csum);
	new_skb->csum = tcp_hdr_csum;
	if (!new_th->check)
		new_th->check = CSUM_MANGLED_0;

	tf_skb_iphdr_init(new_skb, IPPROTO_TCP, saddr, daddr, ip_len);
    
    /* copy vlan info if needed */
    if (iskb->protocol == __constant_ntohs(ETH_P_8021Q)) {
        new_vh = (struct vlan_hdr *)skb_push(new_skb, VLAN_HLEN);
        new_vh->h_vlan_TCI = vlan_eth_hdr(iskb)->h_vlan_TCI;
        new_vh->h_vlan_encapsulated_proto = __constant_htons(ETH_P_8021Q);
    }

    /* copy eth heaer info */
    eh = (struct ethhdr *)skb_push(new_skb, ETH_HLEN);
    skb_reset_mac_header(new_skb);
    new_skb->protocol = eth_hdr(iskb)->h_proto;
    eh->h_proto = eth_hdr(iskb)->h_proto;
    memcpy(eh->h_source, eth_hdr(iskb)->h_dest, ETH_ALEN);
    memcpy(eh->h_dest, eth_hdr(iskb)->h_source, ETH_ALEN);

	return new_skb;
}

int
tf_tcp_send_ack4fin(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *th, char *out_dev_name, int is_fin)
{
    int syn = 0, ack = 1, push = 0, fin = is_fin;
    u32 ack_seq = 1;
    struct sk_buff *new_skb = NULL;
    struct ethhdr *eh = NULL;

    ack_seq = ntohl(th->seq) + 1;
    ack_seq = htonl(ack_seq);

    TF_DEBUG("tf_tcp_new_pack is_fin:%d ...\n", is_fin);
    if(!(new_skb = tf_tcp_new_pack(skb, eh,
                    iph->daddr, 
                    iph->saddr, 
                    th->dest, 
                    th->source, 
                    th->ack_seq, 
                    ack_seq, 
                    (u8 *)(skb->transport_header + 20 + 20)/*FixMe:why is 40?*/, 
                    0/*sizeof(tcphdr)*/, 
                    syn, ack, push, fin))) {
        TF_DEBUG("%s:tf_tcp_new_pack failed!\n", __func__);
        return -1;
    }
    TF_DEBUG("tf_tcp_new_pack over.\n");

    TF_DEBUG("skb=0x%x, out_dev_name=%s.\n", (unsigned int)skb, out_dev_name);
    /* put skb to dev queue to xmit */
    if (out_dev_name) {
        new_skb->dev = dev_get_by_name(&init_net, out_dev_name);
        if (new_skb->dev) {
            dev_put(new_skb->dev);
            if (dev_queue_xmit(new_skb) < 0){
                TF_DEBUG("dev_queue_xmit(%s) failed!\n", new_skb->dev->name);
            } else {
                TF_DEBUG("dev_queue_xmit(%s) ok!\n", new_skb->dev->name);
            }
        } else {
            kfree_skb(new_skb);
            TF_DEBUG("failed! out_dev_name[%s]\n", out_dev_name);
            return -1;
        }
        return 0;
    } else {
        kfree_skb(new_skb);
        TF_DEBUG("%s failed!\n", __func__);
        return -1;
    }

    return 0;
}


static int 
tf_tcp_send_syn_ack(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *th, char *out_dev_name)
{
	int tcp_len, seq = 0xf7bc4e8b/* should be random, fix it(0xf7bc4e8b) for wireshare display syn+ack ok */, syn = 1, ack = 1, push = 0, fin = 0;
	u32 ack_seq = 1;
	struct sk_buff *new_skb = NULL;
	struct ethhdr *eh = NULL;

	/* recalculate acknowledge sequeue number */
	tcp_len = ntohs(iph->tot_len) - ((iph->ihl + th->doff) << 2); /* X*4 */
	if (tcp_len == 0) {/* XXXXX: syn's tcp_len = 0!!! */
		tcp_len = 1;	/* for ack_seq +1, when ack for syn */
	}
	ack_seq = ntohl(th->seq) + tcp_len;
	ack_seq = htonl(ack_seq);

	skb->transport_header = (__u16)(unsigned long)((void*)iph +(iph->ihl << 2));

    TF_DEBUG("tf_tcp_new_pack ...");
	if(!(new_skb = tf_tcp_new_pack(skb, eh,
                    iph->daddr, 
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
    TF_DEBUG("tf_tcp_new_pack over");
	
    TF_DEBUG("skb=0x%x, out_dev_name=%s.", (unsigned int)skb, out_dev_name);
	/* put skb to dev queue to xmit */
	if (out_dev_name) {
		new_skb->dev = dev_get_by_name(&init_net, out_dev_name);
		if (new_skb->dev) {
			dev_put(new_skb->dev);
			if (dev_queue_xmit(new_skb) < 0){
				TF_DEBUG("dev_queue_xmit(%s) failed !", new_skb->dev->name);
			} else {
				TF_DEBUG("dev_queue_xmit(%s) ok !", new_skb->dev->name);
			}
		} else {
            kfree_skb(new_skb);
            TF_DEBUG("failed! out_dev_name[%s]!", out_dev_name);
            return -1;
        }
		return 0;
	} else {
		kfree_skb(new_skb);
		TF_DEBUG("%s failed!\n", __func__);
		return -1;
	}
	
	return 0;
}

static char ignore_mac[6] = {0x00,0x50,0x56,0xC0,0x00,0x10};

#define BEGIN_WITH_HTTP_DATA(d, l) \
    ((((l) >= 3) && (0 == strncmp((d), "GET",3))) ||(((l)>= 4) && (0 == strncmp((d), "POST",4))))

static unsigned int tf_L3_dnat(unsigned int hooknum,
				      struct sk_buff *skb,
				      const struct net_device *in,
				      const struct net_device *out,
				      int (*okfn)(struct sk_buff *))
{
    int ret      = NF_DROP;
    int head_len = 0;
    int data_len = 0;
    char *buf    = NULL;
    u_int32_t flow_mark = 0;
    struct ethhdr *eh = eth_hdr(skb); 
    struct iphdr  *ih = ip_hdr(skb);
    struct tcphdr *th = NULL;

    if (_mac_addr_cmp(eh->h_dest, ignore_mac) == 0 || _mac_addr_cmp(eh->h_source, ignore_mac) == 0) {
        //TF_DEBUG("ignore mac accept.");
        return NF_ACCEPT;
    }

    if (skb->protocol != htons(ETH_P_IP) && skb->protocol != htons(ETH_P_8021Q)) {
        TF_DEBUG("not ip drop.");
        //return NF_DROP; //TODO:or DROP?
        return NF_ACCEPT; //TODO:or DROP?
    }

    if (skb->protocol == htons(ETH_P_8021Q) && ih) {
        ih = (struct iphdr *)((u8 *)ih + 4);
    }

    if (ih->protocol != IPPROTO_TCP) { //TODO?Why not use htons
        TF_DEBUG("not tcp drop.");
        //return NF_DROP; //TODO:or DROP?
        return NF_ACCEPT; //TODO:or DROP?
    }

    th = (struct tcphdr *)((u8 *)ih + (ih->ihl * 4));           
    if (ntohs(th->dest) != 80 && ntohs(th->source) != 80) {
        TF_DEBUG("not http drop.");
        //return NF_DROP;
        return NF_ACCEPT; //TODO:or DROP?
    }

    TF_DEBUG("get http pkt...");
    head_len = (ih->ihl * 4)  + (th->doff * 4);
    data_len = ntohs(ih->tot_len) - head_len;
    buf = skb->data + head_len; /* http data */

    if (nf_ct_flow_mark_get(skb, &flow_mark) != 0) {
        TF_DEBUG("get nf ct flow mark failed.");
        //return NF_DROP;
        return NF_ACCEPT; //TODO:or DROP?
    }

    flow_mark &= TF_SKB_MARK_MASK;

    if (th->rst) {
        TF_DEBUG("TCP RST accept.");
        return NF_ACCEPT;
    }

    if (th->fin) {
        TF_SKB_MARK_LOG(skb, "fin");
        /* 4-way handshake to web server */
        if ((ntohs(th->source) == 80) && (flow_mark & TF_SKB_MARK_NEED_FAKE_FIN)) {
            TF_SKB_MARK_LOG(skb, "fake ack4fin to server");
            tf_tcp_send_ack4fin(skb, ih, th, skb->input_if, 0);
            tf_destroy_skb_conntrack(skb);
            //tf_tcp_send_syn_ack(skb, ih, th, skb->input_if);
            return NF_DROP;
        }
        if ((ntohs(th->dest) == 80) && (flow_mark & TF_SKB_MARK_NORMAL_PKT)) {
            TF_SKB_MARK_LOG(skb, "fake ack4fin to sta");
            tf_tcp_send_ack4fin(skb, ih, th, skb->input_if, 0);
            TF_SKB_MARK_LOG(skb, "fake fin to sta");
            tf_tcp_send_ack4fin(skb, ih, th, skb->input_if, 1);
            return NF_DROP;
        }
    }

    if (th->syn || (th->ack && !th->psh)) {
        if (th->syn && (!th->psh)) {
            /* let first syn go reach the real server 
             * not first fake the syn
             * */
            TF_SKB_MARK_LOG(skb, "syn pkt");
            if (flow_mark & TF_SKB_MARK_NORMAL_PKT) {
                TF_SKB_MARK_LOG(skb, "fake syn ack ...");
                tf_tcp_send_syn_ack(skb, ih, th, skb->input_if);
                TF_SKB_MARK_LOG(skb, "fake syn ack over");
                return NF_DROP;
            }
            nf_ct_flow_mark_set(skb, TF_SKB_MARK_STA_HAS_SENT_SYN);
        }

        if (th->ack) {
            TF_SKB_MARK_LOG(skb, "ack pkt");
            if (th->ack && (!th->psh)) {
                /* http get in tcp segment */
            }
            
            TF_SKB_MARK_LOG(skb, "TODO:??");
            //TODO:??
            if (ntohs(th->source) == 80 && (flow_mark & TF_SKB_MARK_NORMAL_PKT)) {
                //debug("%s,%d, web serv ack to usr, but it's invalid for usr , drop it.\n", __func__, __LINE__);
                return NF_DROP;
            }
            //TODO:??
            if (ntohs(th->dest) == 80 && (flow_mark & TF_SKB_MARK_NORMAL_PKT) == 0) {
                //debug("%s,%d, usr's ack to serv, but it's invalid for web server, drop it.\n", __func__, __LINE__);
                return NF_DROP;
            }
            nf_ct_flow_mark_set(skb, TF_SKB_MARK_NORMAL_PKT);
            return NF_ACCEPT;
        }

        /* sta push data */
        if (ntohs(th->dest) == 80 && th->psh && th->ack) {
            TF_SKB_MARK_LOG(skb, "psh data");
            if (!BEGIN_WITH_HTTP_DATA(buf, data_len)) {
                TF_DEBUG("unknow data in 80 port, drop.");
                return NF_DROP;
            }
        }
    }


    return NF_ACCEPT;
}

unsigned int tf_L3_get_input_interface(
        unsigned int hooknum,
        struct sk_buff * skb,
        const struct net_device *in,
        const struct net_device *out,
        int(*fn)(struct sk_buff *)
        )
{

    TF_LOCK;
    if (!skb->input_if[0]) {
        snprintf(skb->input_if, sizeof(skb->input_if), "%s", in->name);
        TF_DEBUG("skb:0x%x, get input if name:[%s]", (unsigned int)skb, skb->input_if);
    }
    TF_UNLOCK;

    return NF_ACCEPT;
}


static struct nf_hook_ops tcp_filter_ops[] __read_mostly = {
    {
        .owner = THIS_MODULE,
        .hooknum = NF_INET_PRE_ROUTING,     
        .pf = PF_INET,/* for lay-3 */
        .priority = NF_IP_PRI_CONNTRACK + 1,
        .hook = (nf_hookfn *)tf_L3_get_input_interface,    /* set mark at skb which matched rule*/
    },
    {
        .owner = THIS_MODULE,
        .hooknum = NF_INET_PRE_ROUTING, 
        .pf = PF_INET,/* for lay-3 */
        .priority = NF_IP_PRI_CONNTRACK + 1,  /* make sure conntrack prepare ok.*/
        .hook = (nf_hookfn *)tf_L3_dnat, /* redirect to portal server */
    },
};

static int __init tcp_filter_init(void)
{
    //__skb_build_skb_v4();
	spin_lock_init(&g_tf_lock);
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

