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

/* 这些全局变量可以放到一个结构体里，OOP下 */
ULONG      g_ulTaskIDPollFd = -1;
DCL_HEAD_S g_stTaskPendHead;            /* 未被处理的task列表 */
DCL_HEAD_S g_stTaskHead;                /* 在 处理中 的task列表 */
DCL_HEAD_S g_stTaskProccessedHead;      /* 被 处理过 的task列表 */
pthread_mutex_t     g_Task_mutex;/* = PTHREAD_MUTEX_INITIALIZER;*/
pthread_mutexattr_t g_Task_mutexattr;/* = PTHREAD_MUTEX_INITIALIZER;*/
pthread_mutex_t g_ThrdTaskWait_mutex = PTHREAD_MUTEX_INITIALIZER;  		    /* 线程[等待]锁 */
pthread_cond_t  g_ThrdTaskWait_cond;  			/* 线程[唤醒条件]锁 */

#define NS_TASK_LOCK \
    pthread_mutex_lock(&g_Task_mutex)
#define NS_TASK_UNLOCK \
    pthread_mutex_unlock(&g_Task_mutex)

STATIC NS_TASK_INFO * Server_Task_GetByTaskId(UINT uiTaskId) 
{
    NS_TASK_INFO *pstTask = NULL;
    
    NS_TASK_LOCK;
    DCL_FOREACH_ENTRY(&g_stTaskPendHead, pstTask, stNodeTask) {
        if (pstTask->uiTaskId == uiTaskId) {
            NS_TASK_UNLOCK;
            return pstTask;
        }
    }
    NS_TASK_UNLOCK;

    return NULL;
}

ULONG Server_Task_RunTask(ULONG uiTaskId)
{
    pfTaskFunc pfTask = NULL;
    NS_TASK_INFO *pstTask = NULL;
    THREAD_INFO_S *pstThrd = NULL;

    pstThrd = Thread_server_GetCurrent();
    DBGASSERT(NULL != pstThrd);

    MSG_PRINTF("task id:%d is handled (in work thread %d).", 
            uiTaskId, pstThrd->iThreadID);
    
    /* this is a recursive lock call */
    NS_TASK_LOCK;
    pstTask = Server_Task_GetByTaskId(uiTaskId);
    if (pstTask == NULL) {
        ERR_PRINTF("invalid task ID. can find task info");
        return ERROR_FAILE;
    }
    /* 从pend表移到  处理中  的表去 */
    DCL_Del(&(pstTask->stNodeTask));
    DCL_AddTail(&g_stTaskHead, &(pstTask->stNodeTask));
    pfTask = pstTask->pfTask;
    pstTask->uiThrdId = pstThrd->iThreadID;
    NS_TASK_UNLOCK;

    MSG_PRINTF("--------pfTask=0x%X--------------.", pfTask);
    if (pfTask != NULL) {
        /* RUN TASK */
        pfTask(pstTask);
    }
    NS_TASK_LOCK;
    DCL_Del(&(pstTask->stNodeTask));
    DCL_AddTail(&g_stTaskProccessedHead, &(pstTask->stNodeTask));
    NS_TASK_UNLOCK;

    return ERROR_SUCCESS;
}

/* work 线程调用，执行task */
ULONG Server_Task_Handle(VOID *pQueMsgData)
{
	THRD_QUEMSG_DATA_TASK_DISPATCH_S  *pstTaskDisMsg = NULL;


    pstTaskDisMsg = (THRD_QUEMSG_DATA_TASK_DISPATCH_S *)pQueMsgData;

    Server_Task_RunTask(pstTaskDisMsg->uiTaskId);

    free(pstTaskDisMsg);
    pstTaskDisMsg = NULL;
    
    return ERROR_SUCCESS;
}

/* 创建一个task */
ULONG Server_Task_Create(pfTaskFunc pfTask, VOID *pArgs)
{
    NS_TASK_INFO *pstTask = NULL; 

    if (g_stTaskPendHead.uiLiLiLength >= NS_TASK_COUNT_MAX) {
        ERR_PRINTF("ns task is too many.");
        return ERROR_FAILE;
    }

    MSG_PRINTF("------------------------");
    pstTask = malloc(sizeof(NS_TASK_INFO));
    if (pstTask == NULL) {
        ERR_PRINTF("oom.");
        return ERROR_FAILE;
    }

    MSG_PRINTF("------------------------");
    pstTask->pfTask    = pfTask;
    pstTask->ulArgs[0] = (ULONG)pArgs;
    /*
    pstTask->ulArgs[1] = (ULONG)(pArgs + 1);
    pstTask->ulArgs[2] = (ULONG)(pArgs + 2);
    pstTask->ulArgs[3] = (ULONG)(pArgs + 3);
    */
    MSG_PRINTF("------------------------");
    pstTask->uiTaskId  = AllocID(g_ulTaskIDPollFd, 1);
    DBGASSERT(pstTask->uiTaskId >= 0); 

    NS_TASK_LOCK;
    DCL_AddTail(&g_stTaskPendHead, &(pstTask->stNodeTask));
    NS_TASK_UNLOCK;

    MSG_PRINTF("------------------------");
    /* wake up dispatch thread, maybe he iw wait */
    pthread_mutex_lock(&g_ThrdTaskWait_mutex);	 //需要操作临界资源，先加锁，	
    pthread_cond_signal(&g_ThrdTaskWait_cond); 
    pthread_mutex_unlock(&g_ThrdTaskWait_mutex); //解锁		

    return ERROR_SUCCESS;
}

STATIC NS_TASK_INFO * Server_Task_GetNext(NS_TASK_INFO *pstTaskPre) 
{
    /* the check is do at DCL_FIRST_ENTRY and DCL_NEXT_ENTRY functions
    if (DCL_IS_EMPTY(&g_stTaskPendHead) && DCL_IS_END(&g_stTaskPendHead, &(pstTaskPre->stNodeTask))) {
        return NULL;
    }
    */

    if (pstTaskPre == NULL) {
        return DCL_FIRST_ENTRY(&g_stTaskPendHead, pstTaskPre, stNodeTask);
    }

    return DCL_NEXT_ENTRY(&g_stTaskPendHead, pstTaskPre, stNodeTask);
}

/* 把task id 发送给指定的work线程，让work线程获取task信息并处理task */
STATIC ULONG server_task_dispatchToWorkThrd(INT iDestThrdId, NS_TASK_INFO *pstTask)
{
    THREAD_QUEMSG_DATA_S stThrdQueMsg;
	THRD_QUEMSG_DATA_TASK_DISPATCH_S  *pstTaskDisMsg = NULL;

    mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));

    pstTaskDisMsg = malloc(sizeof(THRD_QUEMSG_DATA_TASK_DISPATCH_S));
    if (pstTaskDisMsg == NULL) {
        ERR_PRINTF("oom.");
        return ERROR_FAILE;
    }

    mem_set0(pstTaskDisMsg, sizeof(THRD_QUEMSG_DATA_TASK_DISPATCH_S));

    pstTaskDisMsg->uiTaskId = pstTask->uiTaskId;

    stThrdQueMsg.uiQueMsgDataLen  = sizeof(THRD_QUEMSG_DATA_TASK_DISPATCH_S);
    stThrdQueMsg.uiQueMsgType     = THRD_QUEMSG_TYPE_TASK_DISPATCH;
    stThrdQueMsg.pQueMsgData      = (VOID *)pstTaskDisMsg;

    return THREAD_server_QueMsg_Send(iDestThrdId, &(stThrdQueMsg));
}

/* 把task分发到不同的work线程 */
STATIC ULONG _Server_do_TaskDispatch(VOID)
{
	THREAD_INFO_S *pstWorkThrd = NULL;
    NS_TASK_INFO *pstTask = NULL;

    NS_TASK_LOCK;
    while (1) {
        pstTask = Server_Task_GetNext(pstTask);
        if (pstTask == NULL) {
            break;
        }
        //TEST
        //pstTask->pfTask(pstTask);

        //TODO: 需要一个调度算法。 , client need a WORK thread too. and not start a work, without this thread
        pstWorkThrd = Thread_server_GetByThreadType(THREAD_TYPE_WORK_SERVER);
        DBGASSERT(NULL != pstWorkThrd);

        server_task_dispatchToWorkThrd(pstWorkThrd->iThreadID, pstTask);
    }
    NS_TASK_UNLOCK;

    return ERROR_SUCCESS;
}

/* 主循环，等待事件的申请，并分发 */
STATIC ULONG Server_TaskDispatch_loop(VOID)
{
    ULONG  ulRet = -1;
	struct timeval  now;
	struct timespec outtime;
	THREAD_INFO_S *pstThrd = NULL;

    pstThrd = Thread_server_GetCurrent();
    DBGASSERT(NULL != pstThrd);

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
			SHOW_PRINTF("Task dispatch Thread(thrd id:%d) is Wake Up by thread ...",pstThrd->iThreadID);
            _Server_do_TaskDispatch();
		}
    }

    return ERROR_SUCCESS;
}

ULONG Server_TaskDispatch_Init(VOID *arg)
{
    int ret = -1;

    /* 同一线程内g_Task_mutex可以嵌套 */
    ret = pthread_mutexattr_init(&g_Task_mutexattr);
    if (ret != 0) {
        ERR_PRINTF("mutex attr init failed.");
        return ERROR_FAILE;
    }
    pthread_mutexattr_settype(&g_Task_mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&g_Task_mutex, &g_Task_mutexattr);

    DCL_Init(&g_stTaskPendHead);
    DCL_Init(&g_stTaskHead);
    DCL_Init(&g_stTaskProccessedHead);

    g_ulTaskIDPollFd = CreateIDPool(NS_TASK_COUNT_MAX);
    if (g_ulTaskIDPollFd < 0) {
        ERR_PRINTF("create id pool failed.");
        return ERROR_FAILE;
    }

    MSG_PRINTF("Task dispatch start... ...");

    Server_TaskDispatch_loop();


    return ERROR_SUCCESS;
}

