#ifndef __LIBNETLINK_H__
#define __LIBNETLINK_H__ 1

#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>
#include <linux/if_link.h>
#include <linux/if_addr.h>
#include <linux/neighbour.h>

struct knl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	struct sockaddr_nl	peer;
	__u32			seq;
	__u32			dump;
};

extern int knl_rcvbuf;

extern int knl_open(struct knl_handle *rth, unsigned subscriptions);
extern int knl_open_byproto(struct knl_handle *rth, unsigned subscriptions, int protocol);
extern void knl_close(struct knl_handle *rth);
extern int knl_wilddump_request(struct knl_handle *rth, int fam, int type);
extern int knl_dump_request(struct knl_handle *rth, int type, void *req, int len);

typedef int (*knl_filter_t)(const struct sockaddr_nl *,
			     struct nlmsghdr *n, void *);

struct knl_dump_filter_arg
{
	knl_filter_t filter;
	void *arg1;
};

extern int knl_dump_filter_l(struct knl_handle *rth,
			      const struct knl_dump_filter_arg *arg);
extern int knl_dump_filter(struct knl_handle *rth, knl_filter_t filter,
			    void *arg);
extern int knl_talk(struct knl_handle *rtnl, struct nlmsghdr *n, pid_t peer,
		     unsigned groups, struct nlmsghdr *answer);
extern int knl_send(struct knl_handle *rth, const void *buf, int);
extern int knl_send_check(struct knl_handle *rth, const void *buf, int);

extern int knl_addattr(struct nlmsghdr *n, int maxlen, int type);
extern int knl_addattr8(struct nlmsghdr *n, int maxlen, int type, __u8 data);
extern int knl_addattr16(struct nlmsghdr *n, int maxlen, int type, __u16 data);
extern int knl_addattr32(struct nlmsghdr *n, int maxlen, int type, __u32 data);
extern int knl_addattr64(struct nlmsghdr *n, int maxlen, int type, __u64 data);
extern int knl_addattrstrz(struct nlmsghdr *n, int maxlen, int type, const char *data);

extern int knl_addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen);
extern int knl_addraw_l(struct nlmsghdr *n, int maxlen, const void *data, int len);
extern struct rtattr *knl_addattr_nest(struct nlmsghdr *n, int maxlen, int type);
extern int knl_addattr_nest_end(struct nlmsghdr *n, struct rtattr *nest);
extern struct rtattr *knl_addattr_nest_compat(struct nlmsghdr *n, int maxlen, int type, const void *data, int len);
extern int knl_addattr_nest_compat_end(struct nlmsghdr *n, struct rtattr *nest);
extern int knl_rta_addattr32(struct rtattr *rta, int maxlen, int type, __u32 data);
extern int knl_rta_addattr_l(struct rtattr *rta, int maxlen, int type, const void *data, int alen);

extern int knl_parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
#define knl_parse_nlattr(tb, max, nla, len) knl_parse_rtattr((struct rtattr **)tb, max, (struct rtattr *)(nla), len)
extern int knl_parse_rtattr_byindex(struct rtattr *tb[], int max, struct rtattr *rta, int len);
#define knl_parse_nlattr_byindex(tb, max, nla, len) knl_parse_rtattr_byindex((struct rtattr **)tb, max, (struct rtattr *)(nla), len)
extern int __knl_parse_rtattr_nested_compat(struct rtattr *tb[], int max, struct rtattr *rta, int len);

#define knl_parse_rtattr_nested(tb, max, rta) \
	(knl_parse_rtattr((tb), (max), RTA_DATA(rta), RTA_PAYLOAD(rta)))

#define knl_parse_rtattr_nested_compat(tb, max, rta, data, len) \
	({ data = RTA_PAYLOAD(rta) >= len ? RTA_DATA(rta) : NULL;	\
		__knl_parse_rtattr_nested_compat(tb, max, rta, len); })

static inline __u8 knl_rta_getattr_u8(const struct rtattr *rta)
{
	return *(__u8 *)RTA_DATA(rta);
}
#define knl_nla_getattr_u8(nla) knl_rta_getattr_u8((struct rtattr *)(nla))
static inline __u16 knl_rta_getattr_u16(const struct rtattr *rta)
{
	return *(__u16 *)RTA_DATA(rta);
}
#define knl_nla_getattr_u16(nla) knl_rta_getattr_u16((struct rtattr *)(nla))
static inline __u32 knl_rta_getattr_u32(const struct rtattr *rta)
{
	return *(__u32 *)RTA_DATA(rta);
}
#define knl_nla_getattr_u32(nla) knl_rta_getattr_u32((struct rtattr *)(nla))
static inline __u64 knl_rta_getattr_u64(const struct rtattr *rta)
{
	__u64 tmp;
	memcpy(&tmp, RTA_DATA(rta), sizeof(__u64));
	return tmp;
}
#define knl_nla_getattr_u64(nla) knl_rta_getattr_u64((struct rtattr *)(nla))
static inline const char *knl_rta_getattr_str(const struct rtattr *rta)
{
	return (const char *)RTA_DATA(rta);
}
#define knl_nla_getattr_str(nla) knl_rta_getattr_str((struct rtattr *)(nla))

extern int knl_listen(struct knl_handle *, knl_filter_t handler,
		       void *jarg);
extern int knl_from_file(FILE *, knl_filter_t handler,
		       void *jarg);

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((char *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#ifndef IFA_RTA
#define IFA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif
#ifndef IFA_PAYLOAD
#define IFA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ifaddrmsg))
#endif

#ifndef IFLA_RTA
#define IFLA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifinfomsg))))
#endif
#ifndef IFLA_PAYLOAD
#define IFLA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ifinfomsg))
#endif

#ifndef NDA_RTA
#define NDA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef NDA_PAYLOAD
#define NDA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ndmsg))
#endif

#ifndef NDTA_RTA
#define NDTA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndtmsg))))
#endif
#ifndef NDTA_PAYLOAD
#define NDTA_PAYLOAD(n) NLMSG_PAYLOAD(n,sizeof(struct ndtmsg))
#endif

int
knl_genl_get_id (const char *name);
int
knl_genl_get_mcgrp_id (const char *name, const char *mcgrp_name);

#endif /* __LIBNETLINK_H__ */

