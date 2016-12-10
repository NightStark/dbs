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

#include "tcp_filter.h"

static int __init tcp_filter_init(void)
{
    //__skb_build_skb_v4();
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

