/*
 *  sysfs for Webportal Layer 2 module.
 *
 *  Authors:
 *  Yetao		 <yetao@bhunetworks.com>
 *
 *  Copyright (C) 2015 bhunetworks.com
 *
 */

#include <linux/in.h>
#include <linux/ctype.h>
#include <linux/stat.h>
#include <linux/kobject.h>
#include <linux/list.h>

#include "dns_intercept.h"
#include "dns_intercept_sysfs.h"

#define SYSFS_DNSHACK_ATTR "parameters"

#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

extern spinlock_t bhudns_lock;
extern struct list_head name_list;
extern int bhudns_enable;
extern int bhudns_debug;

static int inet_pton4(const char *src, void *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;
    int ch;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}

char *
ip2str (u32 ipv4, char buf[18])
{
    sprintf(buf, "%d.%d.%d.%d", (ipv4>>24)&0xff, (ipv4>>16)&0xff,
            (ipv4>>8)&0xff, ipv4&0xff);
    return buf;
}

u32
str2ip(char *str_ip)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    inet_pton4(str_ip, &addr.sin_addr);
    return ntohl(addr.sin_addr.s_addr);
}


#define DNSHACK_ATTR(_name, _mode, _show, _store) \
    struct kobj_attribute bhudns_attr_##_name = __ATTR(_name, _mode, _show, _store)

static ssize_t 
show_bhudns_enable(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    int rc = -1;
    spin_lock(&bhudns_lock);
    rc = sprintf(buf, "%d\n", bhudns_enable);
    spin_unlock(&bhudns_lock);
    return rc;
}
static ssize_t 
store_bhudns_enable(struct kobject *kobj, struct kobj_attribute *attr,
        const char *buf, size_t count)
{
    int rc = -1, rv = 0;;
    rc = sscanf(buf, "%d\n", &rv);
    spin_lock(&bhudns_lock);
    if (rc == 1) {
        bhudns_enable = rv;
    }
    spin_unlock(&bhudns_lock);
    return strlen(buf);
}

static DNSHACK_ATTR(enable, S_IRUGO | S_IWUSR,
        show_bhudns_enable, store_bhudns_enable);

static ssize_t 
show_bhudns_debug(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    int rc = -1;
    spin_lock(&bhudns_lock);
    rc = sprintf(buf, "%d\n", bhudns_debug);
    spin_unlock(&bhudns_lock);
    return rc;
}
static ssize_t 
store_bhudns_debug(struct kobject *kobj, struct kobj_attribute *attr,
        const char *buf, size_t count)
{
    int rc = -1, rv = 0;;
    rc = sscanf(buf, "%d\n", &rv);
    if (rc == 1) {
        bhudns_debug = rv;
    }
    return strlen(buf);
}

static DNSHACK_ATTR(debug, S_IRUGO | S_IWUSR,
        show_bhudns_debug, store_bhudns_debug);


static ssize_t 
show_bhudns_name_nodes(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    int k = 0;
    char tmp[256] = {0};
    char ip[32] = {0};
	struct name_node *p;

    spin_lock(&bhudns_lock);
    list_for_each_entry(p, &name_list, node) {
        ip2str(p->ip, ip);
        k += sprintf(tmp, "%s %s\n", p->name, ip);
        strcat(buf, tmp);
    }
    spin_unlock(&bhudns_lock);
    return k;
}

static ssize_t 
store_bhudns_name_nodes(struct kobject *kobj, struct kobj_attribute *attr,
        const char *buf, size_t count)
{
    int rc = -1;
    char op[16] = {0};
    char name[256] = {0};
    char ip[32] = {0};

    spin_lock(&bhudns_lock);
    rc = sscanf(buf, "%s %s %s", op, name, ip);
    //printk(KERN_ERR"%s, rc:%d, op:%s, if_name:%s\n", __func__, rc, op, pi.if_name);
    if (rc >= 1) {
        if(!strcasecmp(op, "clear"))
            bhudns_clear_name_node_list();
        else if (!strcmp(op, "A")) {
            if(rc == 2)
                bhudns_add_name_node(name, 0);
            else
                bhudns_add_name_node(name, str2ip(ip));
        } else if (!strcmp(op, "D")) {
            bhudns_remove_name_node(name);
        }
    } 
    spin_unlock(&bhudns_lock);

    return strlen(buf);
}

static DNSHACK_ATTR(name_nodes, S_IRUGO | S_IWUSR,
        show_bhudns_name_nodes, store_bhudns_name_nodes);


static struct attribute *bhudns_attrs[] = {
    &bhudns_attr_enable.attr,
    &bhudns_attr_debug.attr,
    &bhudns_attr_name_nodes.attr,
    NULL
};

static struct attribute_group bhudns_group = {
    .name = SYSFS_DNSHACK_ATTR,
    .attrs = bhudns_attrs,
};

int bhudns_add_sysfs(struct kobject *obj)
{
    int err;

    if (!obj) {
        pr_info("%s: obj is invalid.\n",__func__);
        return -1;
    }

    err = sysfs_create_group(obj, &bhudns_group);
    if (err) {
        pr_info("%s: can't create group %s\n", __func__, bhudns_group.name);
        return -1;
    }

    return 0;
}
int bhudns_remove_sysfs(struct kobject *obj)
{

    if (!obj) {
        pr_info("%s: obj is invalid.\n",__func__);
        return -1;
    }

    sysfs_remove_group(obj, &bhudns_group);

    return 0;
}
