#ifndef __NS_LOG_H_
#define __NS_LOG_H_

#include <syslog.h>

enum {
	NS_LOG_MOD_DEFAULT,
	NS_LOG_MOD_THREAD,
	NS_LOG_MOD_GET,
	NS_LOG_MOD_KERNEL,

	NS_LOG_MOD_MAX,
};

extern struct dlog_ctl g_dlog_ctl;


void print_buf(char * buf, int len);

int
ns_log_open_conf (int rw, int *fd, void **addr);

void
ns_log_close_conf (int *fd, void **addr);

int
ns_log_commit_conf (void *addr, int len);

int ns_log_mx(int id, int level, const char *fmt, ...);
int ns_log_init(const char *name);
int ns_log_exit();

#define NS_LOG_TO_STDOUT	0x1
#define NS_LOG_TO_CONSOLE	0x2
#define NS_LOG_TO_SYSLOG	0x4
#define NS_LOG_TO_FILE	    0xc
#define NS_LOG_TO_REMOTE	0x14
#define NS_LOG_TO_ALL	(NS_LOG_TO_CONSOLE|NS_LOG_TO_FILE|NS_LOG_TO_REMOTE)

#define NS_LOG_TO_CONVERT(id)	(((id)&0x18)<<12)

struct ns_log_ctl_lv {
	unsigned int stderr_en:1;
	unsigned int stderr_lv:3;
	unsigned int console_en:1;
	unsigned int console_lv:3;
	unsigned int syslog_en:1;
	unsigned int syslog_lv:3;
	unsigned int file_en:1;
	unsigned int file_lv:3;
	unsigned int remote_en:1;
	unsigned int remote_lv:3;
};

struct ns_log_ctlf {
	int start;
	int size;
	int (*func)(void *addr, int id, int level);
};

struct ns_log_ctl {
	int n;
	int size;
	struct ns_log_ctlf f[];
};

#define NS_LOG_CONF_FILE	"/tmp/ns_log_conf.sm"
#define NS_LOG_CONF_FILE_OFFSET	0x100


#define NS_LOG_TYPE_MASK 0xffff0000
#define NS_LOG_ID(type, id)	((type<<16)|id)
#define NS_LOG_MOD(id)		((id>>16)&0xFF)
#define NS_LOG_SUBMOD(id)		(id&0xFFFF)

#ifndef MOD_ID
#define MOD_ID	0
#endif

#define ns_log(l, f, ...) ns_log_mx(MOD_ID, l, f, ##__VA_ARGS__)

//#include "memwatch.h"

#define LOG_PACK_FILE_PATH "/tmp/log.tar.gz"
int ns_log_pack_flash_log();

#define ns_log_caller() ns_log(LOG_DEBUG, "%s: caller %p\n", __func__, __builtin_return_address(0))

#endif /* _LOG_H_ */

