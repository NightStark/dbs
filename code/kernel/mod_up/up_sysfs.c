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

#include "up.h"

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

int up_sysfs_init(void)
{
    int ret = -1;

    memset(&g_up_config, 0, sizeof(up_config_t));
	spin_lock_init(&(g_up_config.up_lock));
    ret = up_create_sysfs();
    
    return ret;
}

void up_sysfs_fini(void)
{
	struct kobject *up_obj = &(THIS_MODULE->mkobj.kobj);
    up_destroy_sysfs(up_obj);
    return;
}

