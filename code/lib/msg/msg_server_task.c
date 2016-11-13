#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <asm/errno.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_epoll.h>
#include <ns_thread.h>
#include <ns_sm.h>
#include <ns_msg.h>
#include <ns_server.h>
#include <ns_id.h>
#include <ns_task.h>

#define NS_TASK_COUNT_MAX (1024)

ULONG      g_ulTaskIDPollFd = -1;
DCL_HEAD_S g_stTaskHead;
pthread_mutex_t g_Task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_ThrdTaskWait_mutex = PTHREAD_MUTEX_INITIALIZER;  		    /* 线程[等待]锁 */
pthread_cond_t  g_ThrdTaskWait_cond;  			/* 线程[唤醒条件]锁 */

#define NS_TASK_LOCK \
    pthread_mutex_lock(&g_Task_mutex)
#define NS_TASK_UNLOCK \
    pthread_mutex_unlock(&g_Task_mutex)

ULONG Server_Task_Create(pfTaskFunc pfTask, VOID *pArgs)
{
    NS_TASK_INFO *pstTask = NULL; 

    if (g_stTaskHead.uiLiLiLength >= NS_TASK_COUNT_MAX) {
        ERR_PRINTF("ns task is too many.");
        return ERROR_FAILE;
    }

    pstTask = malloc(sizeof(NS_TASK_INFO));
    if (pstTask == NULL) {
        ERR_PRINTF("oom.");
        return ERROR_FAILE;
    }

    pstTask->pfTask    = pfTask;
    pstTask->ulArgs[0] = (ULONG)pArgs;
    pstTask->uiTaskId  = AllocID(g_ulTaskIDPollFd, 1);
    DBGASSERT(pstTask->uiTaskId >= 0); 

    NS_TASK_LOCK;
    DCL_AddTail(&g_stTaskHead, &(pstTask->stNodeTask));
    NS_TASK_UNLOCK;

    /* wake up dispatch thread, maybe he iw wait */
    pthread_mutex_lock(&g_ThrdTaskWait_mutex);	 //需要操作临界资源，先加锁，	
    pthread_cond_signal(&g_ThrdTaskWait_cond); 
    pthread_mutex_unlock(&g_ThrdTaskWait_mutex); //解锁		

    return ERROR_SUCCESS;
}

NS_TASK_INFO * Server_Task_GetNext(NS_TASK_INFO *pstTaskPre) 
{
    NS_TASK_INFO *pstTask = NULL;

    if (DCL_IS_EMPTY(&g_stTaskHead)) {
        return NULL;
    }

    if (pstTaskPre == NULL) {
        pstTask = DCL_FIRST_ENTRY(&g_stTaskHead, pstTask, stNodeTask);
    } else {
        pstTask = DCL_NEXT_ENTRY(&g_stTaskHead, pstTask, stNodeTask);
    }

    return pstTask;
}

STATIC ULONG _Server_do_TaskDispatch(VOID)
{
    NS_TASK_INFO *pstTask = NULL;

    while (1) {
        pstTask = Server_Task_GetNext(pstTask);
        if (pstTask == NULL) {
            break;
        }
    }

    return ERROR_SUCCESS;
}

STATIC ULONG Server_TaskDispatch_loop(VOID)
{
    ULONG  ulRet = -1;
	struct timeval  now;
	struct timespec outtime;

    while (1) {
        mem_set0(&outtime, sizeof(struct timespec));

        gettimeofday(&now, NULL);
        outtime.tv_sec  = now.tv_sec + 2;
        outtime.tv_nsec = now.tv_usec * 1000;

    	/*************** 非临界区 ****************/
    	ulRet = pthread_cond_timedwait(&g_ThrdTaskWait_cond, &g_ThrdTaskWait_mutex, &outtime);
		/**************** 临界区 ****************/
		/* 有其他线程唤醒了当前线程 */
		if (0 != ulRet && ETIMEDOUT != errno && 0 != errno)
		{
			perror("pthread_cond_timedwait");
			ERR_PRINTF("pthread cond wait Failed! errno = [%d]", errno);
			ulRet = ERROR_FAILE;
			break;
		}

		if (ETIMEDOUT == ulRet /* || 0 == errno */ )
		{
			SHOW_PRINTF("Task dispatch Thread is Wake Up by thread cond time out!");
		}
		else
		{
			SHOW_PRINTF("Task dispatch Thread is Wake Up by thread ...");
            _Server_do_TaskDispatch();
		}
    }

    return ERROR_SUCCESS;
}

ULONG Server_TaskDispatch_Init(VOID *arg)
{

    DCL_Init(&g_stTaskHead);
    g_ulTaskIDPollFd = CreateIDPool(NS_TASK_COUNT_MAX);
    if (g_ulTaskIDPollFd < 0) {
        ERR_PRINTF("create id pool failed.");
        return ERROR_FAILE;
    }

    MSG_PRINTF("Task dispatch start... ...");

    Server_TaskDispatch_loop();


    return ERROR_SUCCESS;
}
