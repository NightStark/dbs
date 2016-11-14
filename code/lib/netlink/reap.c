#include <stdio.h>          
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "knetlink.h"
#include "../../kernel/mod_up/up.h"

static struct knl_handle g_reap_kh = { .fd = -1 }; 

/* 收割 庄稼：接收内核发来的数据。 */
static int 
reap_crops_cb(const struct sockaddr_nl *addr_nl, 
                 struct nlmsghdr *h,
                 void *args)
{
    int type = 0;
    struct nlattr *tb[__UP_ATTR_MAX + 1];
    int headlen = GENL_HDRLEN + NLMSG_HDRLEN;

    printf("hand msg.\n");
    knl_parse_nlattr(tb, 
                     __UP_ATTR_MAX, 
                     (char *)h + headlen,
                     h->nlmsg_len - headlen);
    if (tb[UP_ATTR_TYPE]) {
        type = knl_nla_getattr_u8(tb[UP_ATTR_TYPE]); 
    }

    printf("###type = %X###\n", (unsigned int)type);

    return 0;
}

int reap_init(void)
{
    int gnid = 0;
    struct {
        struct nlmsghdr     n;
        struct genlmsghdr   g;
        char                buf[1024];
    }req;
    
    gnid = knl_genl_get_id("up_module");
    if (gnid <= 0) {
        printf("get id failed.\n");
        return -1;
    }

    printf("gnid = %d.\n", gnid);

    //if (knl_open_byproto(&g_reap_kh, gnid, NETLINK_GENERIC, 3)) {
    if (knl_open_byproto(&g_reap_kh, gnid, NETLINK_GENERIC)) {
        printf("knl open by proto failed.\n");
        return -1;
    }


	/* send my pid */
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
    req.n.nlmsg_type = g_reap_kh.local.nl_groups;
    req.n.nlmsg_pid = getpid();
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.g.cmd = UP_OPS_REPORT_PID;
    req.g.version = 1;/* TODO */

    if (knl_talk(&g_reap_kh, &req.n, 0, 0, NULL) < 0) {
        printf("knl talk failed.\n");
        goto talk_err;
    }

    if (knl_listen(&g_reap_kh, reap_crops_cb, NULL)) {
        printf("knl listen failed.\n");
        goto talk_err;
	}

    return 0;

talk_err:
    knl_close(&g_reap_kh);
    return -1;
}

int main(void)
{
    reap_init();
    return 0;
}
