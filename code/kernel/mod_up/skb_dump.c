#include <asm/io.h>
#include <asm/irq.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>

#include <linux/time.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/netfilter_bridge.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/ip.h>
#include <linux/inetdevice.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/genetlink.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <net/genetlink.h>

#include "up.h"

//80383024
//8033ef0c <cache_chain_mutex>:
//static DEFINE_MUTEX(cache_chain_mutex);
static struct mutex *cache_chain_mutex = NULL;
static struct list_head *cache_chain = NULL;

static void check_irq_off(void)
{
	BUG_ON(!irqs_disabled());
}

static void check_irq_on(void)
{
	BUG_ON(irqs_disabled());
}

/*
 * struct array_cache
 *
 * Purpose:
 * - LIFO ordering, to hand out cache-warm objects from _alloc
 * - reduce the number of linked list operations
 * - reduce spinlock operations
 *
 * The limit is stored in the per-cpu structure to reduce the data cache
 * footprint.
 *
 */
struct array_cache {
	unsigned int avail;
	unsigned int limit;
	unsigned int batchcount;
	unsigned int touched;
	spinlock_t lock;
	void *entry[];	/*
			 * Must have this definition in here for the proper
			 * alignment of array_cache. Also simplifies accessing
			 * the entries.
			 */
};
/*
 * The slab lists for all objects.
 */
struct kmem_list3 {
	struct list_head slabs_partial;	/* partial list first, better asm code */
	struct list_head slabs_full;
	struct list_head slabs_free;
	unsigned long free_objects;
	unsigned int free_limit;
	unsigned int colour_next;	/* Per-node cache coloring */
	spinlock_t list_lock;
	struct array_cache *shared;	/* shared per node */
	struct array_cache **alien;	/* on other nodes */
	unsigned long next_reap;	/* updated without locking */
	int free_touched;		/* updated without locking */
};

typedef unsigned int kmem_bufctl_t;

/*
 * struct slab
 *
 * Manages the objs in a slab. Placed either at the beginning of mem allocated
 * for a slab, or allocated from an general cache.
 * Slabs are chained into three list: fully used, partial, fully free slabs.
 */
struct slab {
	struct list_head list;
	unsigned long colouroff;
	void *s_mem;		/* including colour offset */
	unsigned int inuse;	/* num of objs active in slab */
	kmem_bufctl_t free;
	unsigned short nodeid;
};

static int __s_show(struct kmem_cache *cachep)
{
	//struct kmem_cache *cachep = list_entry(p, struct kmem_cache, next);
	struct slab *slabp;
	unsigned long active_objs;
	unsigned long num_objs;
	unsigned long active_slabs = 0;
	unsigned long num_slabs, free_objects = 0, shared_avail = 0;
	const char *name;
	char *error = NULL;
	int node;
	struct kmem_list3 *l3;

	active_objs = 0;
	num_slabs = 0;
	for_each_online_node(node) {
		l3 = cachep->nodelists[node];
		if (!l3)
			continue;

		check_irq_on();
		spin_lock_irq(&l3->list_lock);

		list_for_each_entry(slabp, &l3->slabs_full, list) {
			if (slabp->inuse != cachep->num && !error)
				error = "slabs_full accounting error";
			active_objs += cachep->num;
			active_slabs++;
		}
		list_for_each_entry(slabp, &l3->slabs_partial, list) {
			if (slabp->inuse == cachep->num && !error)
				error = "slabs_partial inuse accounting error";
			if (!slabp->inuse && !error)
				error = "slabs_partial/inuse accounting error";
			active_objs += slabp->inuse;
			active_slabs++;
		}
		list_for_each_entry(slabp, &l3->slabs_free, list) {
			if (slabp->inuse && !error)
				error = "slabs_free/inuse accounting error";
			num_slabs++;
		}
		free_objects += l3->free_objects;
		if (l3->shared)
			shared_avail += l3->shared->avail;

		spin_unlock_irq(&l3->list_lock);
	}
	num_slabs += active_slabs;
	num_objs = num_slabs * cachep->num;
	if (num_objs - active_objs != free_objects && !error)
		error = "free_objects accounting error";

	name = cachep->name;
	if (error)
		printk(KERN_ERR "slab: cache %s error: %s\n", name, error);

	printk( "%-17s %6lu %6lu %6u %4u %4d",
		   name, active_objs, num_objs, cachep->buffer_size,
		   cachep->num, (1 << cachep->gfporder));
	printk( " : tunables %4u %4u %4u",
		   cachep->limit, cachep->batchcount, cachep->shared);
	printk( " : slabdata %6lu %6lu %6lu",
		   active_slabs, num_slabs, shared_avail);
#define STATS 0
#if STATS
	{			/* list3 stats */
		unsigned long high = cachep->high_mark;
		unsigned long allocs = cachep->num_allocations;
		unsigned long grown = cachep->grown;
		unsigned long reaped = cachep->reaped;
		unsigned long errors = cachep->errors;
		unsigned long max_freeable = cachep->max_freeable;
		unsigned long node_allocs = cachep->node_allocs;
		unsigned long node_frees = cachep->node_frees;
		unsigned long overflows = cachep->node_overflow;

		printk( " : globalstat %7lu %6lu %5lu %4lu \
				%4lu %4lu %4lu %4lu %4lu", allocs, high, grown,
				reaped, errors, max_freeable, node_allocs,
				node_frees, overflows);
	}
	/* cpu stats */
	{
		unsigned long allochit = atomic_read(&cachep->allochit);
		unsigned long allocmiss = atomic_read(&cachep->allocmiss);
		unsigned long freehit = atomic_read(&cachep->freehit);
		unsigned long freemiss = atomic_read(&cachep->freemiss);

		printk( " : cpustat %6lu %6lu %6lu %6lu",
			   allochit, allocmiss, freehit, freemiss);
	}
#endif
	printk("\n\n\n");
	return 0;
}

static inline void *index_to_obj(struct kmem_cache *cache, struct slab *slab,
				 unsigned int idx)
{
	return slab->s_mem + cache->buffer_size * idx;
}

struct prot_str {
    const char *str;
    unsigned  int p;
};

static struct prot_str g_port_strs_list[] = {
    {"ETH_P_LOOP",	0x0060},		/* Ethernet Loopback packet	*/
    {"ETH_P_PUP",     0x0200},		/* Xerox PUP packet		*/
    {"ETH_P_PUPAT", 0x0201},		/* Xerox PUP Addr Trans packet	*/
    {"ETH_P_IP",     0x0800},		/* Internet Protocol packet	*/
    {"ETH_P_X25",     0x0805},		/* CCITT X.25			*/
    {"ETH_P_ARP",     0x0806},		/* Address Resolution packet	*/
    {"ETH_P_BPQ",     0x08FF},		/* G8BPQ AX.25 Ethernet Packet	[ NOT AN OFFICIALLY REGISTERED ID ] */
    {"ETH_P_IEEEPUP",     0x0a00},		/* Xerox IEEE802.3 PUP packet */
    {"ETH_P_IEEEPUPAT", 0x0a01},		/* Xerox IEEE802.3 PUP Addr Trans packet */
    {"ETH_P_DEC",       0x6000},          /* DEC Assigned proto           */
    {"ETH_P_DNA_DL",    0x6001},          /* DEC DNA Dump/Load            */
    {"ETH_P_DNA_RC",    0x6002},          /* DEC DNA Remote Console       */
    {"ETH_P_DNA_RT",    0x6003},          /* DEC DNA Routing              */
    {"ETH_P_LAT",       0x6004},          /* DEC LAT                      */
    {"ETH_P_DIAG",      0x6005},          /* DEC Diagnostics              */
    {"ETH_P_CUST",      0x6006},          /* DEC Customer use             */
    {"ETH_P_SCA",       0x6007},          /* DEC Systems Comms Arch       */
    {"ETH_P_TEB",       0x6558},		/* Trans Ether Bridging		*/
    {"ETH_P_RARP",      0x8035},		/* Reverse Addr Res packet	*/
    {"ETH_P_ATALK", 0x809B	  },  /* Appletalk DDP		*/
    {"ETH_P_AARP", 0x80F3		},/* Appletalk AARP		*/
    {"ETH_P_8021Q", 0x8100      },    /* 802.1Q VLAN Extended Header  */
    {"ETH_P_IPX",     0x8137	},	/* IPX over DIX			*/
    {"ETH_P_IPV6", 0x86DD		},/* IPv6 over bluebook		*/
    {"ETH_P_PAUSE", 0x8808		},/* IEEE Pause frames. See 802.3 31B */
    {"ETH_P_SLOW", 0x8809		},/* Slow Protocol. See 802.3ad 43B */
    {"ETH_P_WCCP", 0x883E		},/* Web-cache coordination protocol
                                   * defined in draft-wilson-wrec-wccp-v2-00.txt */
    {"ETH_P_PPP_DISC", 0x8863},		/* PPPoE discovery messages     */
    {"ETH_P_PPP_SES", 0x8864 },   	/* PPPoE session messages	*/
    {"ETH_P_MPLS_UC", 0x8847 },   	/* MPLS Unicast traffic		*/
    {"ETH_P_MPLS_MC", 0x8848 },   	/* MPLS Multicast traffic	*/
    {"ETH_P_ATMMPOA", 0x884c },   	/* MultiProtocol Over ATM	*/
    {"ETH_P_ATMFATE", 0x8884 },   	/* Frame-based ATM Transport
                                     * over Ethernet
                                     */
    {"ETH_P_PAE",     0x888E},		/* Port Access Entity (IEEE 802.1X) */
    {"ETH_P_AOE",     0x88A2},		/* ATA over Ethernet		*/
    {"ETH_P_TIPC", 0x88CA	},	/* TIPC 			*/
    {"ETH_P_1588", 0x88F7	},	/* IEEE 1588 Timesync */
    {"ETH_P_FCOE", 0x8906	},	/* Fibre Channel over Ethernet  */
    {"ETH_P_FIP",     0x8914},		/* FCoE Initialization Protocol */
    {"ETH_P_EDSA", 0xDADA},		/* Ethertype DSA [ NOT AN OFFICIALLY REGISTERED ID ] */

    /*
     *	Non DIX types. Won't clash for 1500 types.
     */

    {"ETH_P_802_3", 0x0001		},/* Dummy type for 802.3 frames  */
    {"ETH_P_AX25", 0x0002		},/* Dummy protocol id for AX.25  */
    {"ETH_P_ALL",     0x0003	},	/* Every packet (be careful!!!) */
    {"ETH_P_802_2", 0x0004		},/* 802.2 frames 		*/
    {"ETH_P_SNAP", 0x0005		},/* Internal only		*/
    {"ETH_P_DDCMP",     0x0006  },        /* DEC DDCMP: Internal only     */
    {"ETH_P_WAN_PPP",   0x0007   },       /* Dummy type for WAN PPP frames*/
    {"ETH_P_PPP_MP",    0x0008   },       /* Dummy type for PPP MP frames */
    {"ETH_P_LOCALTALK", 0x0009	 },   /* Localtalk pseudo type 	*/
    {"ETH_P_CAN",       0x000C	 },   /* Controller Area Network      */
    {"ETH_P_PPPTALK",     0x0010 },   	/* Dummy type for Atalk over PPP*/
    {"ETH_P_TR_802_2", 0x0011	 },   /* 802.2 frames 		*/
    {"ETH_P_MOBITEX", 0x0015},		/* Mobitex (kaz@cafe.net)	*/
    {"ETH_P_CONTROL", 0x0016},		/* Card specific control frames */
    {"ETH_P_IRDA", 0x0017	},	/* Linux-IrDA			*/
    {"ETH_P_ECONET", 0x0018	},	/* Acorn Econet			*/
    {"ETH_P_HDLC", 0x0019	},	/* HDLC frames			*/
    {"ETH_P_ARCNET", 0x001A	},	/* 1A for ArcNet :-)            */
    {"ETH_P_DSA",     0x001B},		/* Distributed Switch Arch.	*/
    {"ETH_P_TRAILER", 0x001C},		/* Trailer switch tagging	*/
    {"ETH_P_PHONET", 0x00F5},		/* Nokia Phonet frames          */
    {"ETH_P_IEEE802154", 0x00F6},		/* IEEE802.15.4 frame		*/
};

#define IEEE80211_FC0_VERSION_MASK          0x03
#define IEEE80211_FC0_VERSION_SHIFT         0
#define IEEE80211_FC0_VERSION_0             0x00
#define IEEE80211_FC0_TYPE_MASK             0x0c
#define IEEE80211_FC0_TYPE_SHIFT            2
#define IEEE80211_FC0_TYPE_MGT              0x00
#define IEEE80211_FC0_TYPE_CTL              0x04
#define IEEE80211_FC0_TYPE_DATA             0x08

#define IEEE80211_FC0_SUBTYPE_MASK          0xf0
#define IEEE80211_FC0_SUBTYPE_SHIFT         4
/* for TYPE_MGT */
static struct prot_str g_prot_str_list_w_mgt[] = {
    {"ASSOC_REQ     ",0x00},
    {"ASSOC_RESP    ",0x10},
    {"REASSOC_REQ   ",0x20},
    {"REASSOC_RESP  ",0x30},
    {"PROBE_REQ     ",0x40},
    {"PROBE_RESP    ",0x50},
    {"BEACON        ",0x80},
    {"ATIM          ",0x90},
    {"DISASSOC      ",0xa0},
    {"AUTH          ",0xb0},
    {"DEAUTH        ",0xc0},
    {"ACTION        ",0xd0},
    {"ACTION_NO_ACK ",0xe0},
};                     
/* for TYPE_CTL */   
static struct prot_str g_prot_str_list_w_ctl[] = {
    {"Control_Wrapper",   0x70},    // For TxBF RC
    {"BAR           ",0x80},
    {"PS_POLL       ",0xa0},
    {"RTS           ",0xb0},
    {"CTS           ",0xc0},
    {"ACK           ",0xd0},
    {"CF_END        ",0xe0},
    {"CF_END_ACK    ",0xf0},
};                   
/* for TYPE_DATA (bit combination) */
static struct prot_str g_prot_str_list_w_data[] = {
    {"DATA          ",0x00},
    {"CF_ACK        ",0x10},
    {"CF_POLL       ",0x20},
    {"CF_ACPL       ",0x30},
    {"NODATA        ",0x40},
    {"CFACK         ",0x50},
    {"CFPOLL        ",0x60},
    {"CF_ACK_CF_ACK ",0x70},
    {"QOS           ",0x80},
    {"QOS_NULL      ",0xc0},
};
/*
 * generic definitions for IEEE 802.11 frames
 */
struct ieee80211_frame {
    u_int8_t    i_fc[2];
    u_int8_t    i_dur[2];
    #define IEEE80211_ADDR_LEN      6
    u_int8_t    i_addr1[IEEE80211_ADDR_LEN];
    u_int8_t    i_addr2[IEEE80211_ADDR_LEN];
    u_int8_t    i_addr3[IEEE80211_ADDR_LEN];
    u_int8_t    i_seq[2];
    /* possibly followed by addr4[IEEE80211_ADDR_LEN]; */
    /* see below */
} __packed;

static struct file *g_log_fp = NULL;

struct log_file 
{
    char file_path[128];
    struct file *fp;
    loff_t pos;
};

static struct file* s_open_file(struct log_file *lf)
{
   struct file *fp = NULL; 

   fp = filp_open(lf->file_path, O_RDWR | O_CREAT, 0644);
   if (IS_ERR(fp)) {
       printk("[%s][%d]file[%s] open failed\n", __func__, __LINE__, lf->file_path);
       return NULL;
   }

   lf->fp = fp;

   return fp;
};

static void s_close_file(struct log_file *lf)
{
    if (lf->fp) {
        filp_close(lf->fp, NULL);
        lf->fp = NULL;
        lf->pos = 0;
    }

    return;
}

//static int s_write_2_file(struct file *fp, char *buf, int buf_len, loff_t pos)
static int s_write_2_file(struct log_file *lf, char *buf, int buf_len)
{
    mm_segment_t fs;
    
    fs = get_fs();
    set_fs(KERNEL_DS);

    vfs_write(lf->fp, buf, buf_len, &(lf->pos));
    //lf->fp->f_op->write(lf->fp, buf, buf_len, &(lf->fp->f_pos));

    set_fs(fs);

    //lf->pos += buf_len;

    return 0;
}

#if 0
static int __s_show_known_skb(struct sk_buff *skb, int *fp_pos, int *_cnt)
{
    int pi  = 0;
    int pos = *fp_pos;
    int buf_len = 0;
    char buf[4096];
    int cnt = *_cnt;
	struct ethhdr  *eh = NULL;

    for (pi = 0; pi < ARRAY_SIZE(g_port_strs_list); pi++) {
        if (skb->protocol == htons(g_port_strs_list[pi].p)) {
            //printk("KNOWN[%d] skb p:%s\n", cnt, g_port_strs_list[pi].str);
            if (skb->protocol == htons(ETH_P_IP)) {
                eh = eth_hdr(skb);
                buf_len = snprintf(buf, sizeof(buf),"%4d:%s:\tSRC:"MAC_FMT"\tDEST:"MAC_FMT"\n", 
                        cnt, g_port_strs_list[pi].str, 
                        MAC2STR(eh->h_source), 
                        MAC2STR(eh->h_dest));
                pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
                return 1;
            }

            buf_len = snprintf(buf, sizeof(buf),"KNOWN[%d] skb p:%s\n", cnt, g_port_strs_list[pi].str);
            pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
            return 1;
        }
    }

    *_cnt = cnt;
    *fp_pos = pos;

    return 0;
}
#endif

#if 0
/*
 * @pi: portal type index.
 * @know : know it is a IP skb. 
 *    == 1 : it is a IP skb
 *    == 0 : maybe it is a IP skb
 * */
static int __s_show_tcp(struct sk_buff *skb, int cnt, struct log_file *lf, int pi, int know_ip)
{
    char buf[1024];
    int buf_len = 0;
    struct ethhdr  *eh = NULL;
    struct iphdr   *ip_h = NULL;
    struct tcphdr  *tcp_h = NULL;
    int s_len = 0;

    s_len = sizeof(buf);

    //if (skb->protocol == htons(ETH_P_IP) || know_ip == 0) {
    if (know_ip == 1 || know_ip == 0) {
        if (know_ip == 0) {
            eh = (struct ethhdr *)(skb->data);
        } else {
            eh = eth_hdr(skb);
        }
        if (skb->len >= 54) {
            if (know_ip == 0) {
                ip_h = (struct iphdr *)(eh + 1);
            } else {
                ip_h = ip_hdr(skb);
            }
            if (ip_h->protocol == IPPROTO_TCP) {
                tcp_h = (struct tcphdr *)((u8 *)ip_h + (ip_h->ihl << 2));
                buf_len = snprintf(buf, sizeof(buf),
                        "\n[%d]TCP:[IP_KNOW:%d][len:%d][%s][%d]dport:%d.dmac"MAC_FMT"|", 
                        cnt, know_ip, skb->len,
                        skb->leak_func, skb->leak_line,  /* need in kernel to add this item at "struct skb_buf" */
                        ntohs(tcp_h->dest),
                        MAC2STR(eh->h_dest)
                        );
                if (tcp_h->ack) {
                    buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                            "%s,", "ACK");
                }
                if (tcp_h->fin) {
                    buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                            "%s,", "FIN");
                }
                if (tcp_h->psh) {
                    buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                            "%s,", "PSH");
                }
                if (tcp_h->syn) {
                    buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                            "%s,", "SYN");
                }
                if (tcp_h->rst) {
                    buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                            "%s,", "RST");
                }
                buf_len += snprintf((buf + buf_len), (s_len - buf_len),
                        "|%s", "\n");
                s_write_2_file(lf, buf, buf_len);
                s_write_2_file(lf, skb->data, skb->len);

                return 0;
            }
        }

        buf_len = snprintf(buf, sizeof(buf),"KNOWN0[%4d]:%s:\tSRC:"MAC_FMT"\tDEST:"MAC_FMT"\n", 
                cnt, g_port_strs_list[pi].str, 
                MAC2STR(eh->h_source), 
                MAC2STR(eh->h_dest));
        s_write_2_file(lf, buf, buf_len);
        s_write_2_file(lf, skb->data, skb->len);

        return 0;
    }

    return -1;
}
#else
static int __s_show_tcp(struct sk_buff *skb, int cnt, struct log_file *lf, int pi, int know_ip)
{
    return 0;
}
#endif

#if 0
static int __s_show_obj(struct kmem_cache *cachep)
{
    //struct kmem_cache *cachep = list_entry(p, struct kmem_cache, next);
    struct slab *slabp;
    //unsigned long active_objs;
    //unsigned long num_objs;
    //unsigned long active_slabs = 0;
    //unsigned long num_slabs, free_objects = 0, shared_avail = 0;
    //const char *name;
    char *error = NULL;
    int node;
    struct kmem_list3 *l3;
    int index = 0;
    void *obj = NULL;
    struct sk_buff *skb = NULL;
    int pi = 0;
    struct ieee80211_frame *wh;
    int cnt = 0;
    int skb_user = 0;
    char buf[1024];
    int buf_len = 0;
    loff_t pos = 0;
    struct ethhdr  *eh = NULL;
    struct iphdr   *ip_h = NULL;
    struct tcphdr  *tcp_h = NULL;
    struct udphdr  *udp_h = NULL;
    __be16 d_protocol = 0, *d_p = NULL;
    int type = -1, subtype = -1;
    struct log_file lf;
    struct nf_conn *ct = NULL;
    enum ip_conntrack_info ctinfo;
    int nf_is_here = 0;

   
    memset(&lf, 0, sizeof(lf));

    //active_objs = 0;
    //num_slabs = 0;

    snprintf(lf.file_path, sizeof(lf.file_path), "%s", "/tmp/tfcard/slab_log2");
    g_log_fp = s_open_file(&lf);
    if (NULL == g_log_fp) {
        printk("[%s][%d]file open failed\n", __func__, __LINE__);
        return -1;
    }

    for_each_online_node(node) {
        l3 = cachep->nodelists[node];
        if (!l3)
            continue;

        check_irq_on();
        spin_lock_irq(&l3->list_lock);

        list_for_each_entry(slabp, &l3->slabs_full, list) {
            if (slabp->inuse != cachep->num && !error)
                error = "slabs_full accounting error";
            //active_objs += cachep->num;
            //active_slabs++;
            for(index = 0; index < cachep->num; index ++) {
                cnt ++;
                obj = index_to_obj(cachep, slabp, index);
                skb = (struct sk_buff *)obj;
#if 0
                if (__s_show_known_skb(skb, &pos, &cnt) != 0) {
                    continue;
                }
#endif
                for (pi = 0; pi < ARRAY_SIZE(g_port_strs_list); pi++) {
                    if (skb->protocol == htons(g_port_strs_list[pi].p)) {
                        //printk("KNOWN[%d] skb p:%s\n", cnt, g_port_strs_list[pi].str);
                        if (skb->protocol == htons(ETH_P_IP)) {
                            if (__s_show_tcp(skb, cnt, &lf, pi, 1) == 0) {
                                goto next_s;
                            }
                        }

                        buf_len = snprintf(buf, sizeof(buf),"\nKNOWN[%d] skb p:%s\n", cnt, g_port_strs_list[pi].str);
                        s_write_2_file(&lf, buf, buf_len);
                        s_write_2_file(&lf, skb->data, skb->len);
                        goto next_s;
                    }
                }

                if (skb->len >= (12 + 2)) {
                    d_p = (__be16 *)((skb->data) + 12);
                    d_protocol = *d_p;
                    for (pi = 0; pi < ARRAY_SIZE(g_port_strs_list); pi++) {
                        if (d_protocol == htons(g_port_strs_list[pi].p)) {
                            if (d_protocol == ETH_P_IP) {
                                if (__s_show_tcp(skb, cnt, &lf, pi, 0) == 0) {
                                    goto next_s;
                                }
                            }
                            buf_len = snprintf(buf, sizeof(buf),"\nKNOWN3[%d] skb p:%s\n", cnt, g_port_strs_list[pi].str);
                            pos = s_write_2_file(&lf, buf, buf_len);
                            pos = s_write_2_file(&lf, skb->data, skb->len);
                            goto next_s;
                        }
                    }
                }

#if 0
                if (skb->len >= 24) {
                    wh = (struct ieee80211_frame *)(skb->data);
                    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
                    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

                    if (type == IEEE80211_FC0_TYPE_MGT) {
                        for (pi = 0; pi < ARRAY_SIZE(g_prot_str_list_w_mgt); pi++) {
                            if (subtype == htons(g_prot_str_list_w_mgt[pi].p)) {
#define W_PP 1
#if W_PP
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_MGT[%d][len%d] skb p:%s\n", cnt, skb->len,
                                        g_prot_str_list_w_mgt[pi].str);
#else
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_MGT[%d] skb p:%s MAC1:"MAC_FMT"\n", cnt, 
                                        g_prot_str_list_w_mgt[pi].str, MAC2STR(wh->i_addr1));
#endif
                                pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
                                pos = s_write_2_file(g_log_fp, skb->data, skb->len, pos);

                                goto next_s;
                            }
                        }
                    } else if (type == IEEE80211_FC0_TYPE_CTL) {
                        for (pi = 0; pi < ARRAY_SIZE(g_prot_str_list_w_ctl); pi++) {
                            if (subtype == htons(g_prot_str_list_w_ctl[pi].p)) {
#if W_PP
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_CTL[%d][len:%d] skb p:%s\n", cnt, skb->len,
                                        g_prot_str_list_w_ctl[pi].str);
#else
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_CTL[%d] skb p:%s MAC1:"MAC_FMT"\n", cnt, 
                                        g_prot_str_list_w_ctl[pi].str, MAC2STR(wh->i_addr1));
#endif
                                pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
                                pos = s_write_2_file(g_log_fp, skb->data, skb->len, pos);

                                /*
                                    buf_len = snprintf(buf, sizeof(buf),"[cnt:%d][len:%d]CTS:\n", cnt, skb->len);
                                    pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
                                    pos = s_write_2_file(g_log_fp, skb->data, skb->len, pos);
                                    */

                                goto next_s;
                            }
                        }
                    } else if (type == IEEE80211_FC0_TYPE_DATA) {
                        for (pi = 0; pi < ARRAY_SIZE(g_prot_str_list_w_data); pi++) {
                            if (subtype == htons(g_prot_str_list_w_data[pi].p)) {
#if W_PP
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_DAT[%d][%d] skb p:%s\n", cnt, skb->len,
                                        g_prot_str_list_w_data[pi].str);
#else
                                buf_len = snprintf(buf, sizeof(buf),"KNOWN_W_DAT[%d] skb p:%s MAC1:"MAC_FMT"\n", cnt, 
                                        g_prot_str_list_w_data[pi].str, MAC2STR(wh->i_addr1));
#endif
                                pos = s_write_2_file(g_log_fp, buf, buf_len, pos);
                                pos = s_write_2_file(g_log_fp, skb->data, skb->len, pos);

                                goto next_s;
                            }
                        }
                    }
                }
#endif

                /*
                   snprintf(buf, sizeof(buf), "wh[%d] sub fc:%02x%02x\n", cnt, 
                   (unsigned int)wh->i_fc[0],
                   (unsigned int)wh->i_fc[1]);
                   */

                nf_is_here = 0;
                ct = nf_ct_get(skb, &ctinfo);
                if (ct != NULL) {
                    nf_is_here = 1;
                }

                skb_user = atomic_read(&skb->users);
                buf_len = snprintf(buf, sizeof(buf), "\n[%d]UNKNOW[%s:%d][nf:%s]skb len:%d user:%d\n", 
                        cnt, skb->leak_func, skb->leak_line,
                        ((nf_is_here == 1) ? "FIND":"NOTHING"),
                        skb->len, skb_user);
                s_write_2_file(&lf, buf, buf_len);
                s_write_2_file(&lf, skb->data, skb->len);
next_s:
                continue;
            }
        }
        /*
           list_for_each_entry(slabp, &l3->slabs_partial, list) {
           if (slabp->inuse == cachep->num && !error)
           error = "slabs_partial inuse accounting error";
           if (!slabp->inuse && !error)
           error = "slabs_partial/inuse accounting error";
           active_objs += slabp->inuse;
           active_slabs++;
           }
           */

        spin_unlock_irq(&l3->list_lock);
    }

    if (g_log_fp) {
        s_close_file(&lf);
        g_log_fp = NULL;
    }
    return 0;
}
#else
static int __s_show_obj(struct kmem_cache *cachep)
{
    return 0;
}
#endif

struct skb_slab_info
{
    struct list_head next;
    char name[64];
    int  v;
};

static DEFINE_MUTEX(g_s_s_mutex);
struct list_head  g_s_s_head;

static void print_s_s_header(struct seq_file *m)
{
	/*
	 * Output format version, so at least we can change it
	 * without _too_ many complaints.
	 */
    /*
	seq_puts(m, "s_s - version: 2.1\n");
	seq_puts(m, "# name  "
		 " valude");
	seq_putc(m, '\n');
    */
}
static void *s_s_start(struct seq_file *m, loff_t *pos)
{
	loff_t n = *pos;

	mutex_lock(&g_s_s_mutex);
	if (!n)
		print_s_s_header(m);

	return seq_list_start(&g_s_s_head, *pos);
}


static void *s_s_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &g_s_s_head, pos);
}

static void s_s_stop(struct seq_file *m, void *p)
{
	struct kmem_cache *cachep = NULL;

	mutex_unlock(&g_s_s_mutex);

    mutex_lock(cache_chain_mutex);
    list_for_each_entry(cachep, cache_chain, next) {
        if (strncmp("skbuff_head_cache", cachep->name, strlen("skbuff_head_cache")) == 0) {
            printk("%s:%d name:%s\n", __func__, __LINE__, cachep->name);
            //__s_show(cachep);
            __s_show_obj(cachep);
        }
    }
    mutex_unlock(cache_chain_mutex);
}

static int s_s_show(struct seq_file *m, void *p)
{
    /*
	struct skb_slab_info *ssi  = list_entry(p, struct skb_slab_info, next);
    seq_printf(m, 
               "%8s %4d",
               ssi->name, 
               ssi->v);
    seq_putc(m, '\n');
    */

	return 0;
}
static const struct seq_operations s_slabinfo_op = {
	.start = s_s_start,
	.next = s_s_next,
	.stop = s_s_stop,
	.show = s_s_show,
};

static int s_slabinfo_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &s_slabinfo_op);
}

ssize_t s_slabinfo_write(struct file *file, const char __user * buffer,
		       size_t count, loff_t *ppos)
{
    return 0;
}
static const struct file_operations s_slabinfo_operations = {
	.open		= s_slabinfo_open,
	.read		= seq_read,
	.write		= s_slabinfo_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int ssi_add(const char * name, int v)
{
    struct skb_slab_info *ssi = NULL;

    ssi = vmalloc(sizeof(struct skb_slab_info));
    memset(ssi, 0, sizeof(struct skb_slab_info));

    snprintf(ssi->name, sizeof(ssi->name), "%s", name);
    ssi->v = v;

	mutex_lock(&g_s_s_mutex);
    list_add_tail(&(ssi->next), &g_s_s_head);
	mutex_unlock(&g_s_s_mutex);

    return 0;
}

static int slab_show(void)
{

    ssi_add("lang", 123);
    ssi_add("langl", 124);
    ssi_add("langlg", 1249);

    /*
     * cache_chain_mutex & cache_chain 是静态的变量，！
     * 解决办法。
     *
     * 编译完内核，对内核根目录下的vmlinux文件进行反汇编。
     * [lgg@mips-linux-2.6.31]$mips-linux-objdump vmlinux -D > 1
     * [lgg@mips-linux-2.6.31]$cat 1 | grep cache_chain
     * 8033ef0c <cache_chain_mutex>:
     * 80383024 <cache_chain>:
     *
     * 然后直接使用这两个地址试试，还真的可以。
     * */
    cache_chain_mutex = (struct mutex*)(void*)(unsigned int *)(unsigned int)(0x8033ef0c);//0x8033ef0c
    cache_chain = (struct list_head *)(void *)(unsigned int *)(unsigned int)(0x80383024); //0x80383024
	printk("cache_chain_mutex_addr = 0x%x\n", (unsigned int)cache_chain_mutex);
	printk("cache_chain_addr       = 0x%x\n", (unsigned int)cache_chain);

    /*
	struct kmem_cache *cachep = NULL;
    mutex_lock(cache_chain_mutex);
    list_for_each_entry(cachep, cache_chain, next) {
        printk("%s:%d name:%s\n", __func__, __LINE__, cachep->name);
    }
    mutex_unlock(cache_chain_mutex);
    */

	proc_create("s_slabinfo",S_IWUSR|S_IRUGO,NULL,&s_slabinfo_operations);

    return 0;
}

int skb_dump_init (void) 
{
    INIT_LIST_HEAD(&g_s_s_head);
    slab_show();
    return 0;
}

void skb_dump_fini(void)
{
    struct skb_slab_info *p, *n;
    remove_proc_entry("s_slabinfo", NULL);

	mutex_lock(&g_s_s_mutex);
    list_for_each_entry_safe(p, n, &g_s_s_head, next) {
        list_del(&(p->next));
        vfree(p);
    }
	mutex_unlock(&g_s_s_mutex);

    return;
}
