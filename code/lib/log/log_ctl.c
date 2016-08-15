#include <stdio.h>
#include "ns_log.h"


/* XXX: Don't refernce any function in other module!!! */

int ns_log_mod_def (void *addr, int id, int level)
{
#if 1
	int mask = 0;
	struct ns_log_ctl_lv *lv = addr;

	if (lv->stderr_en && lv->stderr_lv >= level) {
		mask |= NS_LOG_TO_STDOUT;
	}
	if (lv->console_en && lv->console_lv >= level) {
		mask |= NS_LOG_TO_CONSOLE;
	}
	if (lv->syslog_en && lv->syslog_lv >= level) {
		mask |= NS_LOG_TO_SYSLOG;
		if (lv->file_en && lv->file_lv >= level) {
			mask |= NS_LOG_TO_FILE;
		}
		if (lv->remote_en && lv->remote_lv >= level) {
			mask |= NS_LOG_TO_REMOTE;
		}
	}
	return mask;
#else
	if (LOG_WARNING >= level) { /* print warning and above level log */
		return NS_LOG_TO_SYSLOG;/* TODO: do some check */
	} else {
		return 0;
	}
#endif
}

int inline ns_log_mod_thread (void *addr, int id, int level) 
{
    return ns_log_mod_def(addr, id, level);
}


#define NS_LOG_CTL_LV_SIZE          (sizeof(struct ns_log_ctl_lv))
#define NS_LOG_MOD_DEFAULT_START	0
#define NS_LOG_MOD_DEFAULT_SIZE	    NS_LOG_CTL_LV_SIZE
#define NS_LOG_MOD_THREAD_START     NS_LOG_MOD_DEFAULT_START + NS_LOG_MOD_DEFAULT_SIZE
#define NS_LOG_MOD_THREAD_SIZE      NS_LOG_CTL_LV_SIZE
#define NS_LOG_MOD_GET_START     NS_LOG_MOD_THREAD_START + NS_LOG_MOD_THREAD_SIZE
#define NS_LOG_MOD_GET_SIZE      NS_LOG_CTL_LV_SIZE



struct ns_log_ctl g_ns_log_ctl = {
    .n = NS_LOG_MOD_MAX,
    .f = {
    /* default mod */
        [NS_LOG_MOD_DEFAULT] = {
            .start = NS_LOG_MOD_DEFAULT_START,
            .size  = NS_LOG_MOD_DEFAULT_SIZE,
            .func  = ns_log_mod_def,
        },
    /* thread mod */
        [NS_LOG_MOD_THREAD] = {
            .start = NS_LOG_MOD_THREAD_START,
            .size  = NS_LOG_MOD_THREAD_SIZE,
            .func  = ns_log_mod_thread,
        },
        [NS_LOG_MOD_GET] = {
            .start = NS_LOG_MOD_GET_START,
            .size  = NS_LOG_MOD_GET_SIZE,
            .func  = ns_log_mod_def,
        },
    },
#define NS_LOG_MOD_SIZE	(NS_LOG_MOD_THREAD_SIZE + NS_LOG_MOD_DEFAULT_SIZE)
    .size = NS_LOG_MOD_SIZE,
};


