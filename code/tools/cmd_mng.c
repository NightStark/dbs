#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include <sys/syslog.h>
#include <sys/eventfd.h>
#include <fcntl.h>

#include "popen.h"
#include "list.h"

typedef struct cmd_info
{
    struct list_head node;
    char cmd[256];
    FILE *fp;
    int fd;
    char value[1024];
    int value_p;
    struct event *ev;
}CMD_MNG_INFO_ST;

static struct list_head g_cmd_list;

#define __MUTEX_STATIC(M,I) static pthread_mutex_t M = I
__MUTEX_STATIC(g_cmd_list_mutex, PTHREAD_MUTEX_INITIALIZER);
#define __MUTEX_LOCK(M) \
do { \
    pthread_mutex_lock(&M); \
} while(0) 
#define __MUTEX_UNLOCK(M) \
do { \
    pthread_mutex_unlock(&M); \
} while(0) 

#define CMD_MNG_LOCK __MUTEX_LOCK(g_cmd_list_mutex)
#define CMD_MNG_UNLOCK __MUTEX_UNLOCK(g_cmd_list_mutex)

#define _sys_popen popen
#define _sys_pclose pclose

#define _LOG(level, fmt, ...) \
    do { \
        printf("[%s][%d]"fmt"\n" , __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

static int g_evt_base_is_ready = 0;
static struct event_base *g_evt_base = NULL;
static struct event *g_timer_event = NULL;

CMD_MNG_INFO_ST *
cmd_mng_info_create(char *cmd)
{
    CMD_MNG_INFO_ST *cmd_info = NULL;

    cmd_info = malloc(sizeof(CMD_MNG_INFO_ST));
    if (cmd_info == NULL) {
        _LOG(LOG_DEBUG, "oom");
        return NULL;
    }
    memset(cmd_info, 0, sizeof(CMD_MNG_INFO_ST));

    snprintf(cmd_info->cmd, sizeof(cmd_info->cmd), "%s ;echo __TYGGCMDEND__RET_$?__", cmd);

    return cmd_info;
}

void cmd_mng_info_destroy(CMD_MNG_INFO_ST *cmd_info)
{
    if (cmd_info->fp) {
        _sys_pclose(cmd_info->fp);
        cmd_info->fp = NULL;
    }

    if (cmd_info->ev) {
        event_del(cmd_info->ev);
        event_free(cmd_info->ev);
    }
    free(cmd_info);
    cmd_info = NULL;

    return;
}

void cmd_mng_info_destroy_all(void)
{
    CMD_MNG_INFO_ST *cmd_info = NULL, *n = NULL;

    CMD_MNG_LOCK; 
    list_for_each_entry_safe(cmd_info, n, &g_cmd_list, node) {
        list_del(&cmd_info->node);
        cmd_mng_info_destroy(cmd_info);
    }
    CMD_MNG_UNLOCK; 

}

static void cmd_read_cb(int fd, short event, void *arg)
{
    int len = 0;
    char *pend = NULL;
    CMD_MNG_INFO_ST *cmd_info = NULL;

    cmd_info = (CMD_MNG_INFO_ST *)arg;
    if (NULL == cmd_info) {
        _LOG(LOG_ERR, "invalid arg");
        return;
    }

    _LOG(LOG_DEBUG, "cmd:[%s]", cmd_info->cmd);

    len = read(fd, cmd_info->value + cmd_info->value_p, 
            sizeof(cmd_info->value) - cmd_info->value_p - 1);
    if (len < 0) {
        _LOG(LOG_DEBUG, "fread failed of cmd[%s].", "cmd");
        return;
    }
    cmd_info->value_p += len;
    cmd_info->value[cmd_info->value_p] = '\0';
    _LOG(LOG_DEBUG, "cmd result:====[len:%d][%s]====", len, cmd_info->value);
    pend = strstr(cmd_info->value, "__TYGGCMDEND__");

    if (len == 0 || pend) {
        _LOG(LOG_DEBUG, "cmd run over ====[%d][%s]====", len, cmd_info->value);
        CMD_MNG_LOCK; 
        list_del(&cmd_info->node);
        cmd_mng_info_destroy(cmd_info);
        CMD_MNG_UNLOCK; 
        cmd_info = NULL;
    }

    return;
}

int get_cmd_result_async(char *cmd)
{
    FILE *fp = NULL;
    int fd = 0;
    CMD_MNG_INFO_ST *cmd_info = NULL;
    struct event *eventfd_event = NULL;

    if (cmd == NULL) {
        return -1;
    }

    CMD_MNG_LOCK;
    if (!g_evt_base_is_ready) {
        CMD_MNG_UNLOCK;
        _LOG(LOG_ERR, "cmd evt base is not ready.");
        return -1;
    }
    CMD_MNG_UNLOCK;

    _LOG(LOG_DEBUG, "get cmd [%s]", cmd);

    cmd_info = cmd_mng_info_create(cmd);
    if (NULL == cmd_info) {
        return -1;
    }

    fp = _sys_popen(cmd_info->cmd, "r");
    if (fp == NULL ) {
        _LOG(LOG_DEBUG, "popen failed of cmd[%s].", cmd);
        return -1;
    }

    fd = fileno(fp);
    cmd_info->fp = fp;
    cmd_info->fd = fd;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    eventfd_event = event_new(g_evt_base, fd, EV_READ | EV_PERSIST | EV_ET, cmd_read_cb, cmd_info); 
    if (eventfd_event == NULL) {
        _LOG(LOG_DEBUG, "event new failed");
        goto error;
    }
    cmd_info->ev = eventfd_event;
    CMD_MNG_LOCK; 
    list_add(&cmd_info->node, &g_cmd_list);
    CMD_MNG_UNLOCK; 
    event_add(eventfd_event, NULL);

    return 0;
error:
    if (eventfd_event) {
        event_free(eventfd_event);
    }

    if (fp) {
        fclose(fp);
    }

    if (cmd_info) {
        free(cmd_info);
    }
    
    return -1;
}

struct timeval g_timeout;

static void cmd_timer_cb(int fd, short kind, void *userp)
{

    //_LOG(LOG_DEBUG, "------------------");
    //TODO:can not kill sub-process of sh!!!
    //_sys_ptimeout();
    //_LOG(LOG_DEBUG, "------------------");

    evtimer_add(g_timer_event, &g_timeout);
    return;
}

static int init_cmd_timer(void)
{
    g_timer_event = evtimer_new(g_evt_base, cmd_timer_cb, NULL);
	if (NULL == g_timer_event) {
		_LOG(LOG_ERR, "event new failed.\n");
        return -1;
	}

    evtimer_add(g_timer_event, &g_timeout);

    return 0;
}
void *cmd_work_thrd(void *args)
{
    int ret = 0;

    _LOG(LOG_DEBUG, "cmd work start ...");
    CMD_MNG_LOCK;
    g_evt_base = event_base_new();
    CMD_MNG_UNLOCK;
	if (NULL == g_evt_base) {
		_LOG(LOG_ERR, "event base new failed.\n");
		return NULL;
	}

    g_timeout.tv_sec = 1;
    g_timeout.tv_usec = 0;
    init_cmd_timer();

    CMD_MNG_LOCK;
    g_evt_base_is_ready = 1;
    CMD_MNG_UNLOCK;
    /*NOTE: if g_evt_base not add a event, event_base_dispatch will exit */
    ret = event_base_dispatch(g_evt_base);
    if (ret != 0) {
        _LOG(LOG_DEBUG, "event_base_dispatch error:%d", ret);
        perror("event_base_dispatch");
    }

    _LOG(LOG_DEBUG, "cmd work exit");

    evtimer_del(g_timer_event);
    event_free(g_timer_event);
    event_base_free(g_evt_base);
    g_evt_base = NULL;

    return NULL;
}

int cmd_work_init(void)
{
    pthread_t cmd_work_tid;
    pthread_attr_t cmd_work_attr;

    INIT_LIST_HEAD(&g_cmd_list);
    pthread_attr_init(&cmd_work_attr);
    pthread_attr_setdetachstate(&cmd_work_attr, PTHREAD_CREATE_DETACHED);
    if (0 != pthread_create(&cmd_work_tid, &cmd_work_attr, cmd_work_thrd, NULL)) {
        _LOG(LOG_ERR, "postman signup therad create failed");
        return -1;
    }

    return 0;
}

int cmd_work_stop(void)
{
    CMD_MNG_LOCK;
    g_evt_base_is_ready = 0;
    CMD_MNG_UNLOCK;

    cmd_mng_info_destroy_all();
    event_base_loopbreak(g_evt_base);

    return 0;
}

int main()
{
    int cnt = 0;
    //char x[] = "hello xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    //char buf[1024];
    //dump_data_to_buffer(x, strlen(x));

    get_cmd_result_async("date; sleep 1; date; echo 4");
    get_cmd_result_async("date; sleep 1; date; echo 4");
    cmd_work_init();
    sleep(1);

    //get_cmd_result_async("ls -al");

    //_LOG(LOG_DEBUG, "cmd result:====[%s]====", buf);
    
    while(cnt < 4) {
    /*
        get_cmd_result_async("date; sleep 1; date; echo 1");
        get_cmd_result_async("date; sleep 1; date; echo 2");
        get_cmd_result_async("date; sleep 1; date; echo 3");
        */
        get_cmd_result_async("date; sleep 1; date; echo 4");
        sleep(2);
        cnt ++;
    }

    get_cmd_result_async("date; sleep 10; date; echo 4");
    cmd_work_stop();
    get_cmd_result_async("date; sleep 10; date; echo 4");

    sleep(10);

    return 0;
}
