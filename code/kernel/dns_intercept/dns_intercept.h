/*
 *  Dns intercept.
 *
 *  Authors:
 *  Yetao <yetao@bhunetworks.com>
 *
 *  Copyright (C) 2015 bhunetworks.com
 *
 */
#ifndef _DNSHACK_H_
#define _DNSHACK_H_

#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/wireless.h>


struct name_node {
	struct list_head node;
    char *name;
    u32 ip;
};

#if defined(__LITTLE_ENDIAN_BITFIELD)
struct flags{
    __u16   rcode:4,
            z:3,
            ra:1,
            rd:1,
            rc:1,
            aa:1,
            op:4,
            qr:1;
};
#elif defined (__BIG_ENDIAN_BITFIELD)
struct flags{
    __u16   qr:1, 
            op:4, 
            aa:1, 
            rc:1, 
            rd:1, 
            ra:1, 
            z:3, 
            rcode:4;
};
#else
#error  "Please fix <asm/byteorder.h>"
#endif
union flag_union{
    struct flags bits;
    __u16 unit;
};
struct bhu_dns_hdr {
    __be16  id;
    union flag_union flag;
    __be16  qdcount;
    __be16  ancount;
    __be16  nscount;
    __be16  arcount;
};

struct dns_response {
    __be16  name;
    __be16  type;
    __be16  class;
    __be32  ttl;
    __be16  len;
    __be32  ip;
} __attribute__((packed));

typedef int (*DNS_RSP_HOOK)(void *args, struct bhu_dns_hdr *hdr, const char *domain, u32 *ipvec, u32 ipnum);


void
bhudns_remove_name_node(char *name);
void 
bhudns_clear_name_node_list(void);
int
bhudns_add_name_node(char *name, u32 ip);
struct name_node *
bhudns_match_node(char *name);

int
dns_reg_hook(DNS_RSP_HOOK hook, void *args);

int
dns_unreg_hook(DNS_RSP_HOOK hook);

#endif
