#include <asm/io.h>
#include <asm/irq.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>

#include <linux/time.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/netfilter_bridge.h>
#include <linux/kernel.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <net/ip.h>
#include <linux/inetdevice.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <net/genetlink.h>

#include "up.h"

static unsigned int g_up_pid = 0;

static const char * g_http_header_field_list[] = 
{
    "Accept",
    "Accept-Charset",
    "Accept-Encoding",
    "Accept-Language",
    "Accept-Ranges",
    "Age",
    "Allow",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Content-Encoding",
    "Content-Language",
    "Content-Length",
    "Content-Location",
    "Content-MD5",
    "Content-Range",
    "Content-Type",
    "Date",
    "ETag",
    "Expect",
    "Expires",
    "From",
    "Host",
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Last-Modified",
    "Location",
    "Max-Forwards",
    "Pragma",
    "Proxy-Authenticate",
    "Proxy-Authorization",
    "Range",
    "Referer",
    "Retry-After",
    "Server",
    "TE",
    "Trailer",
    "",
};

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

    //NLA_PUT_U8(skb, UP_ATTR_TYPE, 0xA5);
    nla_put_u8(skb, UP_ATTR_TYPE, 0xA5);

    genlmsg_end(skb, hdr);

#ifdef OLD_VER
    err = genlmsg_unicast(skb, g_up_pid);
#else
    err = genlmsg_unicast(&init_net, skb, g_up_pid);
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
    int err = -1;
#ifdef OLD_VER
    int i = 0;

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
#else
	err = genl_register_family_with_ops(&g_up_nl_family, g_up_nl_ops);
    if (err != 0) {
        UP_MSG_PRINTF("genl register ops failed.");
        goto reg_ops_err;
    }
#endif
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

typedef struct tag_http_field_node
{
    struct list_head stNode;
    STR_A2B_INFO_ST stA2B;
}HTTP_FIELD_NODE_ST;

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

    posB = __strstr2(posA, pstStrA2BInfo->strB, strlen);
    if (posB == NULL) {
        return -1;
    }

    pstStrA2BInfo->posA = posA;
    pstStrA2BInfo->posB = posB;

    pstStrA2BInfo->A2BLen = posB - posA;
    UP_MSG_PRINTF("A:%x,B:%x, len:%d", (unsigned int)posA,
            (unsigned int)posB, (posB-posA));

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

/* find @str in pstA2B
 * */
static inline char * __A2B_strstr(STR_A2B_INFO_ST *pstA2B, const char *str)
{
    char *p = NULL;

    p  = __strstr2(pstA2B->posA, str, pstA2B->A2BLen);

    return p;
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
char *
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

static int __http_response_inject_check_room(char *http_hdr, int http_hdr_len, 
        int need_len, struct list_head *http_field_list_head)
{
    int i = 0;
    int dig_len = 0;
    STR_A2B_INFO_ST stA2B;
    HTTP_FIELD_NODE_ST *pstHttpField = NULL, *n = NULL;

    memset(&stA2B, 0, sizeof(stA2B));

    //TODO:can for a loop

    for (i = 0; i < ARRAY_SIZE(g_http_header_field_list); i++)
    if (0 == _get_http_header_filed(http_hdr, http_hdr_len, g_http_header_field_list[i], &stA2B)) {
        dig_len += stA2B.A2BLen;
        pstHttpField = vmalloc(sizeof(HTTP_FIELD_NODE_ST));
        if (pstHttpField == NULL) {
            UP_MSG_PRINTF("oom ...!");
            goto err_oom;
        }
        memcpy(&(pstHttpField->stA2B), &stA2B, sizeof(pstHttpField->stA2B));
        list_add_tail(&(pstHttpField->stNode), http_field_list_head);
    }

    if (dig_len >= need_len) {
        return 1;
    }


    return 0;
err_oom:
    list_for_each_entry_safe(pstHttpField, n, http_field_list_head, stNode) {
        list_del(&(pstHttpField->stNode));
        kfree(pstHttpField);
    }
    pstHttpField = NULL;
    n = NULL;
    return -1;
}

/*
 * |**************|****[fieldA][fieldB][fieldC]***********|**************************************|
 * |<-TCP-Header->|<-----------HTTP-Header--------------->|<html><head>......</head>......</html>|
 *                ^                                       ^            ^
 *                |                    ^                  |            |
 *                http_hdr             |                  http_data    inject_start(inject_end)
 *                                     |                               |
 * after inject                        |                               |
 *                                     |-------------------------------| 
 *                                                  /                  /\
 *                                                 /                  /  \
 *                                                /move              /    \
 *                                               |        __________/      \____
 *                                               V       /              dig     |
 *                            |-----------------------| /                       |
 *                            |                       |/                        | 
 *                            V                       V                         V
 * |**************|***[fieldA][fieldC]***|************|*************************|*************************|
 * |<-TCP-Header->|<--------HTTP-Header--|<html><head><javascript> </javascript>......</head>......</html>|
 *                ^                      ^            ^                         ^
 *                |                      |            |<------inject data------>|
 *                http_hdr                http_data    inject_start             inject_end
 *
 * note:
 *      sizeof([fieldB]) >= (inject data length)
 *
 * */

static int
up_ct_http_response_inject(char * data, int data_len)
{
    char *p = NULL;
    char *http_data     = NULL;
    int   http_data_len = 0;
    char *http_hdr      = NULL;
    int   http_hdr_len  = 0;
    char js_str[128];
    char *inject_start  = NULL;
    char *inject_end    = NULL;
    STR_A2B_INFO_ST *pstA2B = NULL;
    struct list_head http_field_list;

    int js_str_len = 0;
    int ret = 0;

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
    http_hdr_len = http_data - http_hdr;

    http_data_len = data_len - http_hdr_len;
    if (http_data_len <= 0) {
        UP_MSG_PRINTF("no http data.");
        return -1;
    }

    INIT_LIST_HEAD(&http_field_list);
    js_str_len = 512;
    ret = __http_response_inject_check_room(http_hdr, http_hdr_len, js_str_len, &http_field_list);
     HTTP_FIELD_NODE_ST *pstS = NULL;
    list_for_each_entry(pstS, &http_field_list, stNode) {
        char buf[256];
        snprintf(buf, (pstS->stA2B.A2BLen + 1 >= sizeof(buf) ? sizeof(buf) : pstS->stA2B.A2BLen + 1), 
                "%s", pstS->stA2B.posA);
        UP_MSG_PRINTF(":%s", buf);
    }
    if (ret) {
        UP_MSG_PRINTF("no enougth room for inject.");
        return -1;
    }

    UP_MSG_PRINTF("room is enougth for inject. and start inject");

    /* get <head> in HTML */
    p = __strstr2(http_data, "<head>", http_data_len);
    if (p == NULL) {
        UP_MSG_PRINTF("find <head> failed.");
        return -1;
    }
    inject_start = inject_end = p + 6; /* skip "<head>" */

    /* start do inject */

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

    snprintf(buf, (stA2B.A2BLen + 1 >= sizeof(buf) ? sizeof(buf) : stA2B.A2BLen + 1), 
            "%s", stA2B.posA);

    UP_MSG_PRINTF(":%s", buf);
    /* check if this is a noraml html file */
    p  = __A2B_strstr(&stA2B, "text/html");
    p1 = __A2B_strstr(&stA2B, "application/x-javascript");
    if (p != NULL || p1 != NULL) {
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

    return 0;
}

//static nf_hookfn *up_ct_http_hook_cb;

static unsigned int up_ct_http_hook_cb(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    char buf[512];
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

    // if (ctinfo == IP_CT_ESTABLISHED) 
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

        //up_report_data();
        //UP_MSG_PRINTF("dport:%d sport:%d", dport, sport);
        if (sport == 80 /* || dport == 80 */) {
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
        .hook     = (nf_hookfn *)up_ct_http_hook_cb,
        //.hook     = up_ct_http_hook_cb,
    },
};

static int __init up_init(void)
{
    int err = 0;
    up_link_init();
    UP_MSG_PRINTF("up link init success.");

    if (up_sysfs_init() < 0) {
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
    
    UP_MSG_PRINTF("init success.");

    return 0;
}

static void __exit up_exit(void)
{

    nf_unregister_hooks(up_hooks, ARRAY_SIZE(up_hooks));
    up_sysfs_fini();
    up_link_fini();

    UP_MSG_PRINTF("exit success.");
    return;
}

module_init(up_init);
module_exit(up_exit);

MODULE_AUTHOR("G");
MODULE_VERSION("V0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("up-module");

