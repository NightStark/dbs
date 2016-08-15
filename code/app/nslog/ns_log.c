#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ns_log.h>

#define USE_DML_LOG_NAME
#define USE_DML_BASE_LOG_NAME

#define NS_LOG_OPEN_CONF_RW 1

extern struct ns_log_ctl g_ns_log_ctl;

struct log_mod{
	const char *name;
	int (*func)(int argc, char *argv[]);
	void (*usage)();
	int (*init)();
    struct ns_log_ctl_lv def_lv;
};

/*
static int
log_ctl_get_console_lvl_from_addr(struct ns_log_ctl_lv *lv)
{
	int val = 0;
	if (lv->console_en)
		val += (lv->console_lv+1);
	return val;
}
*/

static int
log_ctl_get (struct ns_log_ctl_lv *lv)
{
	int val = 0;

	if (lv->stderr_en)
		val += (lv->stderr_lv+1);
	if (lv->console_en)
		val += (lv->console_lv+1)*10;
	if (lv->syslog_en)
		val += (lv->syslog_lv+1)*100;
	if (lv->file_en)
		val += (lv->file_lv+1)*1000;
	if (lv->remote_en)
		val += (lv->remote_lv+1)*10000;

	return val;
}

/*
static int
log_ctl_get_console_lvl(int val)
{
	return (val % 100) / 10;
}
*/

static int
log_ctl_set (struct ns_log_ctl_lv *lv, int val)
{
	int i;

	if (val < 0)
		return -1;

	if (val == 0) {
		memset(lv, 0, sizeof(struct ns_log_ctl_lv));
		return 0;
	}

	i = val % 10;
	lv->stderr_en = i?1:0;
	if (i)
		lv->stderr_lv = i - 1;

	i = (val % 100)/10;
	lv->console_en = i?1:0;
	if (i)
		lv->console_lv = i - 1;

	i = (val % 1000)/100;
	lv->syslog_en = i?1:0;
	if (i)
		lv->syslog_lv = i - 1;

	i = (val % 10000)/1000;
	lv->file_en = i?1:0;
	if (i)
		lv->file_lv = i - 1;

	i = (val % 100000)/10000;
	lv->remote_en = i?1:0;
	if (i)
		lv->remote_lv = i - 1;

	return 0;
}

/**************************************/
/***** cmd handle for default     *****/
/**************************************/

void
usage_def ()
{
	printf("Only def\n");
}

void
usage_therad ()
{
	printf("Only thread\n");
}

int
init_def ()
{
	int fd = -1;
	void *addr = NULL;
	struct ns_log_ctl_lv *lv;

	if (ns_log_open_conf(1, &fd, &addr) < 0) {
		printf("Can't open dlog config file!\n");
		return -1;
	}

	lv = addr + g_ns_log_ctl.f[NS_LOG_MOD_DEFAULT].start;
	log_ctl_set(lv, 500);
	
	ns_log_close_conf(&fd, &addr);

	return 0;
}

int
init_thread ()
{
	int fd = -1;
	void *addr = NULL;
	struct ns_log_ctl_lv *lv;

	if (ns_log_open_conf(1, &fd, &addr) < 0) {
		printf("Can't open dlog config file!\n");
		return -1;
	}

	lv = addr + g_ns_log_ctl.f[NS_LOG_MOD_THREAD].start;
	log_ctl_set(lv, 500);
	
	ns_log_close_conf(&fd, &addr);

	return 0;
}

int
log_def (int argc, char *argv[])
{
	int fd = -1;
	void *addr = NULL;
	struct ns_log_ctl_lv *lv;
	int val;

	if (ns_log_open_conf(1, &fd, &addr) < 0) {
		printf("Can't open dlog config file!\n");
		return -1;
	}

	lv = addr + g_ns_log_ctl.f[NS_LOG_MOD_DEFAULT].start;

	switch (argc) {
	case 0:
	case 1:
		val = log_ctl_get(lv);
		printf("\t%05d %5d def\n", val, 1);
		break;
	case 2:
		val = atoi(argv[1]);
		log_ctl_set(lv, val);
		break;
	}
	ns_log_close_conf(&fd, &addr);

	return 0;
}

int
log_thread (int argc, char *argv[])
{
	int fd = -1;
	void *addr = NULL;
	struct ns_log_ctl_lv *lv;
	int val;

	if (ns_log_open_conf(1, &fd, &addr) < 0) {
		printf("Can't open dlog config file!\n");
		return -1;
	}

	lv = addr + g_ns_log_ctl.f[NS_LOG_MOD_THREAD].start;

	switch (argc) {
	case 0:
	case 1:
		val = log_ctl_get(lv);
		printf("\t%05d %5d thread\n", val, 1);
		break;
	case 2:
		val = atoi(argv[1]);
		log_ctl_set(lv, val);
		break;
	}
	ns_log_close_conf(&fd, &addr);

	return 0;
}

/* WARNING: Reserved module name: "init" */
//TODO,we need a tree. for each mod and chile mode
struct log_mod log_mods[] = {
    {"def",     log_def,    usage_def,  init_def, },
    {"thread",  log_thread, usage_therad,  init_thread, },
    {"get",  log_thread, usage_therad,  init_thread, },
    {"", NULL, NULL, NULL},
};

void
usage_set ()
{
	printf("Every number means:\n"
		"  Remote, File, Syslog, console, stdout\n"
		"\t0: Close\n"
		"\t1: Emergency\n"
		"\t2: Alert\n"
		"\t3: Critical\n"
		"\t4: Error\n"
		"\t5: Warning\n"
		"\t6: Notice\n"
		"\t7: Info\n"
		"\t8: Debug\n");
		printf("**************************\n\t");
}

void
usage (int argc, char *argv[])
{
	int i;

	if (argc >= 3) {
		for (i = 0; i < sizeof(log_mods)/sizeof(log_mods[0]); i++) {
			if (log_mods[i].name[0] == 0){
				goto mhelp;
			}
			if (strcmp(argv[2], log_mods[i].name) == 0) {
				break;
			}
		}
	} else {
mhelp:
		printf("**************************\n\t");
		printf("Support module:\n\t");
		for (i = 0; i < sizeof(log_mods)/sizeof(log_mods[0]); i++) {
			if (log_mods[i].name[0] == 0){
				break;
			}
			printf(" %s", log_mods[i].name);
		}
		printf("\n\r**************************\n\r");
		usage_set();
		printf("\nFor detail, use: dlog help <module>\n");
		return;
	}
	printf("Help of module: %s\n",log_mods[i].name);
	log_mods[i].usage();
}

void 
log_init(void)
{
	int i=0;

	for (i = 0; i < sizeof(log_mods)/sizeof(log_mods[0]); i++) {
		if (log_mods[i].init != NULL) {
			log_mods[i].init();
		}
	}

    printf("ns log init success.\n");
}

int
main(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		usage(argc, argv);
		return 1;
	}

	if (strcmp(argv[1], "init") == 0) {
		log_init();
		return 0;
	}

	for (i = 0; i < sizeof(log_mods)/sizeof(log_mods[0]); i++) {
		if (log_mods[i].name[0] == 0){
			usage(argc, argv);
			return -1;
		}
		if (strcmp(argv[1], log_mods[i].name) == 0) {
			break;
		}
	}

	log_mods[i].func(argc-2, argv+2);

	return 0;
}
