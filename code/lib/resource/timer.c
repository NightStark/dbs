#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#ifndef NS_TIMERFD	
#include <sys/timerfd.h>
#endif
#include <sys/epoll.h>


#include <ns_base.h>

#include <ns_lilist.h>
#include <ns_bitmap.h>
#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_thread.h>
#include <ns_timer.h>

#define CLOCKID CLOCK_REALTIME

typedef struct tag_timer_info
{
    DCL_NODE_S stNode;

    INT iTimerId;
    ULLONG ullPeriod; /* 定时器要定时的时长，毫秒  */
    ULLONG ullExpire;
    INT iReadFd;     /* 任何向捕获这个定时的，可以把这个fd加入epoll */
    INT iWriteFd;    /* pipe的写端, 写入数据可触发iReadFd监听到事件 */
}TIMER_INFO_ST;

STATIC TIMER_INFO_ST *TIMER_info_Create(IN ULLONG ullPeriod);
STATIC VOID TIMER_info_Destroy(IN INT iReadFd);

#ifndef NS_TIMERFD	
INT Timer_Create(IN INT iSec)
{
    INT iTimerFd;
    struct itimerspec stOutTime;
    struct timespec stNowTime;

    /* Create a CLOCK_REALTIME absolute timer with initial
       expiration and interval as specified in command line */

	mem_set0(&stNowTime, sizeof(stNowTime));
    if (clock_gettime(CLOCK_REALTIME, &stNowTime) == -1)
    {
        ERR_PRINTF("clock gettime Failed!");
		return -1;
    }
    
	mem_set0(&stOutTime, sizeof(stOutTime));
    stOutTime.it_value.tv_sec = stNowTime.tv_sec + iSec;
    stOutTime.it_value.tv_nsec = stNowTime.tv_nsec;
    
    stOutTime.it_interval.tv_sec = iSec;
    stOutTime.it_interval.tv_nsec = 0;

    iTimerFd = timerfd_create(CLOCK_REALTIME, 0);
    if (iTimerFd == -1)
    {
        ERR_PRINTF("timerfd create Failed!");
		return -1;
    }

    if (timerfd_settime(iTimerFd, TFD_TIMER_ABSTIME, &stOutTime, NULL) == -1)
    {
        ERR_PRINTF("timerfd create Failed!");
		return -1;
    }

    MSG_PRINTF("timer started !");

	return iTimerFd;
}
#else
INT Timer_Create(IN INT iSec)
{
    ULLONG ullPeriod;
    TIMER_INFO_ST * pstTimer = NULL;

    ullPeriod = iSec * 1000;
    pstTimer = TIMER_info_Create(ullPeriod);
    if (NULL == pstTimer) {
        ERR_PRINTF("timer info create failed.");
        return -1;
    }

    return pstTimer->iReadFd;
}
#endif

STATIC VOID timer_Delete(INT iTimerId)
{
    
    TIMER_info_Destroy(iTimerId);
	//close(iTimerId);
	/* 交由 THREAD_epoll_EventDel释放 */
	return;
}

ULONG timer_callback(IN UINT events, IN VOID *arg)
{
	INT iLen;
	ULONG ulRet;
	UINT64 ui64TimerData;
	TIMER_DATA_S *pstTimerData;
	THREAD_EPOLL_EVENTS_S *pstTrdEpEvent;
	
	timerout_callback_t pfTmOutCB;
	
	pstTrdEpEvent = (THREAD_EPOLL_EVENTS_S *)arg;
	pstTimerData = (TIMER_DATA_S *)pstTrdEpEvent->pPriData;
	
	if ((events & EPOLLIN) && (pstTrdEpEvent->uiEvents & EPOLLIN))
	{
		MSG_PRINTF("timer is out!");
		
		iLen = read(pstTrdEpEvent->iEventFd, &ui64TimerData, sizeof(UINT64));
        if (iLen!= sizeof(UINT64))
        {
			ERR_PRINTF("Read Timer Data Failed");
			return ERROR_FAILE;
        }
        
        pfTmOutCB = (timerout_callback_t)pstTimerData->pfTmOutCB;
		if (NULL != pfTmOutCB)
		{
			ulRet = pstTimerData->pfTmOutCB(pstTimerData->iTimerID, 
										    pstTimerData->auiPara);
		}
	}
	
	return ulRet;
}

/* 
   释放制定线程下的所有定时器资源 
   线程释放之前释放， 
*/
ULONG TIMER_DeleteAllForThread(IN INT iThreadId)
{
	THREAD_INFO_S *pstThreadInfo = NULL;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt = NULL;
	
	pstThreadInfo = Thread_server_GetByThreadId(iThreadId);
	if (pstThreadInfo == NULL)
	{
		ERR_PRINTF("Thread_server_GetByThreadId Failed!");
		return ERROR_FAILE;
	}	

	for(;;)
	{
		pstThrdEpEvt = DCL_FIRST_ENTRY(&(pstThreadInfo->pstThrdEp->stThrdEpEvtHead), pstThrdEpEvt,stNode);
		if (NULL == pstThrdEpEvt)
		{
			break;
		}
		if (pstThrdEpEvt->iTimerId != -1)
		{
			free(pstThrdEpEvt->pPriData);
			THREAD_epoll_EventDel(pstThrdEpEvt->iTimerId, pstThreadInfo->pstThrdEp);
			pstThrdEpEvt = NULL;
		}
	}
	/*
	DCL_FOREACH_ENTRY(&(pstThreadInfo->pstThrdEp->stThrdEpEvtHead), pstThrdEpEvt, stNode)
	{
		ERR_PRINTF("________________________________");
		if (pstThrdEpEvt->iTimerId != -1)
		{
			free(pstThrdEpEvt->pPriData);
			THREAD_epoll_EventDel(pstThrdEpEvt->iTimerId, pstThreadInfo->pstThrdEp);	
			pstThrdEpEvt = NULL;
		}
	}
	*/
	
	return ERROR_SUCCESS;
}

/* 
   注意1，修改该函数时 和函数TIMER_DeleteAllForThread释放相同，以免遗漏 
   
   注意2，这里的定时器事件和普通事件是一样的Event结构体，他也包含一个iTimerId，不要混淆，奶奶的
*/
ULONG TIMER_DeleteForThread(IN INT iThreadId, IN INT iTimerId)
{
	THREAD_INFO_S *pstThreadInfo = NULL;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt = NULL;

	MSG_PRINTF("Release Timer [TimeriD : %d]", iTimerId);
	
	pstThreadInfo = Thread_server_GetByThreadId(iThreadId);
	if (pstThreadInfo == NULL)
	{
		ERR_PRINTF("Thread_server_GetByThreadId Failed!");
		return ERROR_FAILE;
	}

	pstThrdEpEvt = Thread_epoll_EpEvtGetByEtId(iTimerId, 
											   pstThreadInfo->pstThrdEp);
	if (pstThrdEpEvt == NULL)
	{
		ERR_PRINTF("thread epoll Event is delete already!");
		return ERROR_SUCCESS;
	}
	
	free(pstThrdEpEvt->pPriData);
	THREAD_epoll_EventDel(iTimerId, pstThreadInfo->pstThrdEp);

	timer_Delete(iTimerId);

	return ERROR_SUCCESS;
}

/*
 * iThreadId : 要处理改定时超时的线程的ID
 * iMsec     : 定时秒数
 * timerout_callback_t : 超时回调函数
 * pPara     : 回调函数的参数
 * */
INT TIMER_CreateForThread(IN INT iThreadId, /* 待整合 */
                          IN INT iMSec, 
                          IN timerout_callback_t pfTmOutCB,
                          IN VOID *pPara /* len = 4 个 字 */)
{
	ULONG ulRet  = 0;
	INT iTimerFD = 0;
	TIMER_DATA_S          *pstTimerData;
	
	iTimerFD = Timer_Create(iMSec);
	if (-1 == iTimerFD)
	{
		return -1;
	}
					   
	pstTimerData = mem_alloc(sizeof(TIMER_DATA_S));
	if (NULL == pstTimerData)
	{
		ERR_PRINTF("mem_alloc Failed!");
		return -1;
	}
	
	pstTimerData->iTimerID  = iTimerFD;
	pstTimerData->pfTmOutCB = pfTmOutCB;
	
	memcpy(pstTimerData->auiPara, pPara, sizeof(UINT) * 4);
	
	ulRet = THREAD_epoll_EventAdd(iTimerFD, 
								  EPOLLIN|EPOLLET,
								  iThreadId,
								  timer_callback,
								  pstTimerData);
	if (ERROR_SUCCESS != ulRet)
	{
		return -1;
	}

	return iTimerFD;
	
}

#if 0 /* 信号定时器 */
#define SIG SIGRTMIN

ULONG TIMER_CreateForThread(IN INT iMSec)
{
   timer_t 			 TimerId;
   struct sigevent   stSigEvent;
   struct itimerspec stITimerSpec;
   sigset_t 		 SigMask;
   struct sigaction  stSigAct;
   long long freq_nanosecs;
      		
   /* Block timer signal */
   printf("Blocking signal %d\n", SIG);
   sigemptyset(&SigMask);
   sigaddset(&SigMask, SIG);
   if (sigprocmask(SIG_SETMASK, &SigMask, NULL) == -1)
   {
	   ERR_PRINTF("Blocking signal failed!");
	   return ERROR_FAILE;
   }

   /* Create the timer */
   stSigEvent.sigev_notify = SIGEV_SIGNAL;
   stSigEvent.sigev_signo = SIG;
   stSigEvent.sigev_value.sival_ptr = &TimerId;
   if (timer_create(CLOCKID, &stSigEvent, &TimerId) == -1)
   {
	   ERR_PRINTF("timer_create failed!");
	   return ERROR_FAILE;
   }

   ERR_PRINTF("timer_create SUCCESS timer ID is 0x%lx\n", (long)TimerId);

   /* Start the timer */
   mem_set0(&stITimerSpec, sizeof(stITimerSpec));
   stITimerSpec.it_value.tv_sec = iMSec / 1000;
   //stITimerSpec.it_value.tv_nsec = freq_nanosecs % 1000000000;
   stITimerSpec.it_interval.tv_sec = iMSec / 1000;
   //stITimerSpec.it_interval.tv_nsec = stITimerSpec.it_value.tv_nsec;
   if (timer_settime(TimerId, 0, &stITimerSpec, NULL) == -1)
   {
	   ERR_PRINTF("timer_settime failed!");
	   return ERROR_FAILE;
   }

   

   return EXIT_SUCCESS;
}

#endif 

typedef struct tag_timer_list_head
{
    INT iCnt;
    DCL_HEAD_S stHead;
    pthread_mutex_t timerListMutex;
}TIMER_LIST_HEAD_ST;

typedef struct t_pipe_fds
{
    INT iReadFd;
    INT iWriteFd;
}TIMER_PIPE_FDS_ST;

TIMER_LIST_HEAD_ST g_stTimerListHead;
#define TIMER_LOCK   (pthread_mutex_lock(&(g_stTimerListHead.timerListMutex)))
#define TIMER_UNLOCK (pthread_mutex_unlock(&(g_stTimerListHead.timerListMutex)))

/* get time since system boot, by ms */
INLINE ULLONG TIMER_get_BootTime(VOID) 
{
    ULLONG ullTBoot = 0;
    struct timespec clc = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &clc) < 0) {
        ERR_PRINTF("get time failed");
        return 0;
    }

    ullTBoot = clc.tv_sec * 1000 + clc.tv_nsec / 1000000;

    return ullTBoot;
}


STATIC TIMER_INFO_ST *TIMER_info_Create(IN ULLONG ullPeriod)
{
    INT   iFds[2];
    INT   iRet = -1;
    ULLONG ullTBoot = 0;
    TIMER_PIPE_FDS_ST *pstTFds = NULL;
    TIMER_INFO_ST *pstTimerInfo = NULL;

    pstTimerInfo = malloc(sizeof(TIMER_INFO_ST));
    if (NULL == pstTimerInfo) {
        ERR_PRINTF("malloc failed");
        goto err_pipe;
    }
    memset(pstTimerInfo, 0, sizeof(TIMER_INFO_ST));

    pstTimerInfo->ullPeriod = ullPeriod;
    iRet = pipe(iFds); //假动作，没人用啊，？？
    if (iRet < 0) {
        ERR_PRINTF("pipe failed.");
        goto err_pipe;
    }
    pstTFds = (TIMER_PIPE_FDS_ST *)iFds;

    pstTimerInfo->iReadFd  = pstTFds->iReadFd;
    pstTimerInfo->iWriteFd = pstTFds->iWriteFd;

    ullTBoot = TIMER_get_BootTime();
    if (0 == ullTBoot) {
        goto err_time;
    }

    pstTimerInfo->ullExpire = ullTBoot + pstTimerInfo->ullPeriod;

    TIMER_LOCK;
    DCL_AddTail(&(g_stTimerListHead.stHead), &(pstTimerInfo->stNode));
    TIMER_UNLOCK;

    return pstTimerInfo;
err_time:
    close(pstTimerInfo->iReadFd);
    close(pstTimerInfo->iWriteFd);
err_pipe:
    if (NULL != pstTimerInfo) {
        free(pstTimerInfo);
        pstTimerInfo = NULL;
    }
    return NULL;
}

STATIC TIMER_INFO_ST * TIMER_info_FindByReadFd(IN INT iReadFd)
{
    TIMER_INFO_ST *pstTimerInfo = NULL;

    TIMER_LOCK;
    DCL_FOREACH_ENTRY(&(g_stTimerListHead.stHead), pstTimerInfo, stNode) {
        if (pstTimerInfo->iReadFd == iReadFd) {
            break;
        }
    }
    TIMER_UNLOCK;

    return pstTimerInfo;
}

UINT INLINE TIMER_info_ReadFdisTimerFd(IN INT iReadFd)
{
    if (NULL != TIMER_info_FindByReadFd(iReadFd)) {
        return 1;
    } else {
        return 0;
    }
}

STATIC VOID TIMER_info_Destroy(IN INT iReadFd)
{
    
    TIMER_INFO_ST *pstTimerInfo = NULL;

    pstTimerInfo = TIMER_info_FindByReadFd(iReadFd);
    if (NULL == pstTimerInfo) {
        ERR_PRINTF("invalid readfd");
        return;
    }

    MSG_PRINTF("timer readfd[%d] is delete.", pstTimerInfo->iReadFd);
    TIMER_LOCK;
    close(pstTimerInfo->iReadFd);
    pstTimerInfo->iReadFd = -1;
    close(pstTimerInfo->iWriteFd);
    pstTimerInfo->iWriteFd= -1;

    DCL_Del(&(pstTimerInfo->stNode));

    free(pstTimerInfo);
    pstTimerInfo = NULL;

    TIMER_UNLOCK;


    return;
}

ULONG TIMER_list_Init(VOID)
{
    memset(&g_stTimerListHead, 0, sizeof(g_stTimerListHead));
    DCL_Init(&(g_stTimerListHead.stHead));
    pthread_mutex_init(&(g_stTimerListHead.timerListMutex), NULL);

    return ERROR_SUCCESS;
}

VOID TIMER_list_Fini(VOID)
{
    TIMER_INFO_ST *pstTimerInfo     = NULL;
    TIMER_INFO_ST *pstTimerInfoNext = NULL;

	TIMER_LOCK;
	DCL_FOREACH_ENTRY_SAFE(&(g_stTimerListHead.stHead), pstTimerInfo, pstTimerInfoNext, stNode){
		TIMER_info_Destroy(pstTimerInfo->iReadFd);
	}

	if (DCL_IS_EMPTY(&(g_stTimerListHead.stHead))) {
		MSG_PRINTF("Clear list success.");
	} else {
		ERR_PRINTF("NOT EMPTY!!");
		return;
	}

    DCL_Fint(&(g_stTimerListHead.stHead));
	TIMER_UNLOCK;

    pthread_mutex_destroy(&(g_stTimerListHead.timerListMutex));

    return;
}

STATIC ULONG TIMER_thread_pipe_EpCallBack(IN UINT events, IN VOID *arg)
{
    ssize_t ret = -1; 
    size_t  len = 0;
    UINT64 ui64Data = 0xAA;
    TIMER_INFO_ST *pstTimerInfo = NULL;
    ULLONG ullTBoot = 0;

    //MSG_PRINTF("");
    //TODO:should read the pipe, the pipe FULL ?
    len = sizeof(ui64Data);
    
    ullTBoot = TIMER_get_BootTime();

    TIMER_LOCK;
    DCL_FOREACH_ENTRY(&(g_stTimerListHead.stHead), pstTimerInfo, stNode) {
        if (ullTBoot > pstTimerInfo->ullExpire) {
            if (pstTimerInfo->iWriteFd> 0) {
                ret = write(pstTimerInfo->iWriteFd, &ui64Data, len);
                if (ret != len) {
                    ERR_PRINTF("ret:len %d!=%d", ret, len);
                    ERR_PRINTF("write fd failed.");
                }
            } else {
                ERR_PRINTF("INVALID pipe fd.");
            }
            pstTimerInfo->ullExpire = ullTBoot + pstTimerInfo->ullPeriod;
        }
    }
    TIMER_UNLOCK;

    return ERROR_SUCCESS;
}

STATIC ULONG TIMER_thread_pipe_Init(IN THREAD_INFO_S *pstThrdInfo)
{
    INT   iFds[2];
    INT   iRet  = 0;
    ULONG ulRet = 0;
    TIMER_PIPE_FDS_ST *pstTFds = NULL;

    DBGASSERT(NULL != pstThrdInfo);

    iRet = pipe(iFds); //假动作，没人用啊，？？
    if (iRet < 0) {
        ERR_PRINTF("pipe failed.");
        return ERROR_FAILE;
    }

    pstTFds = (TIMER_PIPE_FDS_ST *)iFds;

    MSG_PRINTF("thread id = %d", pstThrdInfo->iThreadID);
    ulRet = Thread_server_EpollAdd(pstThrdInfo->iThreadID, 
                                   pstTFds->iReadFd,
                                   0,
                                   TIMER_thread_pipe_EpCallBack);

    return ulRet;
}

ULONG TIMER_THRD_INIT_CB(VOID * pArgs/* THREAD_INFO_S * */)
{
    THREAD_INFO_S *pstThrdInfo = NULL;

    DBGASSERT(NULL != pArgs);

    pstThrdInfo = (THREAD_INFO_S *)pArgs;
	MSG_PRINTF("---- Timer Thread Init Start ----");

    TIMER_thread_pipe_Init(pstThrdInfo);

	MSG_PRINTF("---- Timer Thread Init End----");

    return ERROR_SUCCESS;
}

STATIC ULONG TIMER_list_test(VOID)
{
	TIMER_INFO_ST *pstInfo = NULL;

	pstInfo = TIMER_info_Create(100);
	if (NULL == pstInfo) {
		ERR_PRINTF("crate timer info failed");
		return ERROR_FAILE;
	}
	MSG_PRINTF("crate timer info success");
	TIMER_info_Destroy(pstInfo->iReadFd);
	if (DCL_IS_EMPTY(&(g_stTimerListHead.stHead))) {
		MSG_PRINTF("EMPTY");
	} else {
		ERR_PRINTF("NOT EMPTY");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

ULONG TIMER_thread_init (VOID)
{
    INT iThrdId = -1;

    TIMER_list_Init();

	TIMER_list_test();

    iThrdId = Thread_server_CreateWithEpTimeOut(TIMER_THRD_INIT_CB, 
                                                NULL, 
                                                THREAD_TYPE_TIMER,
                                                100);
    if (iThrdId < 0) {
        ERR_PRINTF("Create Timer thread failed.");
        return ERROR_FAILE;
    }

	DBG_THRD_NAME_REG(iThrdId, "TIMER");

    return ERROR_SUCCESS;
}
VOID TIMER_thread_Fini (VOID)
{
	TIMER_list_Fini();
	return;
}
