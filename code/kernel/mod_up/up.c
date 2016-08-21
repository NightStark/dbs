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

#include "string.h"
#include "up.h"

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
    //p1 = __A2B_strstr(&stA2B, "application/x-javascript");
    if (p != NULL) {
        /* inject */
        return up_ct_http_response_inject(data, data_len);
    }
    /* check content type end */

    UP_MSG_PRINTF("http is not text/html.");
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
        return up_ct_http_response(data, data_len);
    }

    return 1;
}

//TODO:why do this
unsigned short tcp_checksum(struct iphdr *ip, struct tcphdr *tcp)                     
{
    int hlen = 0;
    int len = 0, count = 0;
    unsigned int sum = 0;
#if 1
    unsigned char odd[2] =
    {
        0, 0
    };
#endif
    unsigned short * ptr = NULL;
    hlen = (ip->ihl << 2);
    len = ntohs(ip->tot_len) - hlen;
    count = len;
    sum = 0;
    ptr = (unsigned short *)tcp;
    while (count > 1){
        sum += *ptr++;
        count -= 2;
    }
    if (count){
        odd[0] = *( unsigned char *) ptr;
        ptr = ( unsigned short *) odd;
        sum += *ptr;
    }

    /* Pseudo-header */
    ptr = (unsigned short *) &(ip->daddr);
    sum += *ptr++;
    sum += *ptr;
    ptr = (unsigned short *) &(ip->saddr);
    sum += *ptr++;
    sum += *ptr;
    sum += htons((unsigned short)len);
    sum += htons((unsigned short)ip->protocol);
#if 1
    /* Roll over carry bits */
    sum = ( sum >> 16) + ( sum & 0xffff);
    sum += ( sum >> 16);
    // Take one's complement
    sum = ~sum & 0xffff;
    return sum;
#else
    while(sum >> 16)
        sum = (sum >> 16) + (sum & 0xffff);
    return (unsigned short)(~sum);
#endif
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
        /*
            UP_MSG_PRINTF("dport:%d sport:%d", dport, sport);
            _ip2str(dst_ip, buf, sizeof(buf));
            UP_MSG_PRINTF("dip:%s", buf);
            _ip2str(src_ip, buf, sizeof(buf));
            UP_MSG_PRINTF("sip:%s", buf);
            UP_MSG_PRINTF("data len:%d", data_len);
            */

            //snprintf(buf, sizeof(buf), "%s", pdata);
            //UP_MSG_PRINTF("http data:%s", buf);

            if (0 == up_ct_http_get_url(pdata, data_len)) {
                th->check = 0;                            
                th->check = tcp_checksum(ih, th);       
            }
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

