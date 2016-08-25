/*
 *  sysfs for bhudns module.
 *
 *  Authors:
 *  Yetao		 <yetao@bhunetworks.com>
 *
 *  Copyright (C) 2015 bhunetworks.com
 *
 */
#ifndef _DNSHACK_SYSFS_H_
#define _DNSHACK_SYSFS_H_
int bhudns_remove_sysfs(struct kobject *obj);
int bhudns_add_sysfs(struct kobject *obj);
#endif
