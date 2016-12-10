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
        ct->p_data = kmalloc(sizeof(struct tf_data) ,GFP_ATOMIC);
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

static unsigned int tf_L3_dnat(unsigned int hooknum,
				      struct sk_buff *skb,
				      const struct net_device *in,
				      const struct net_device *out,
				      int (*okfn)(struct sk_buff *))
{

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

    TF_MSG_PRINTF("exit success.");

    return;
}

module_init(tcp_filter_init);
module_exit(tcp_filter_exit);

MODULE_AUTHOR("G");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("skb build");

