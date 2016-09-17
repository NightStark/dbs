/*
	该服务线程并不是服务端的线程，而是可以服务消息的线程
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#ifndef NS_EVENTFD
#include <sys/eventfd.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include <asm/errno.h>
#include <signal.h>

#include <ns_base.h>

#include <ns_id.h>
#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_fsm.h>
#include <ns_lilist.h>
#include <ns_timer.h>
#include <ns_event.h>
#include <ns_thread.h>
#include <ns_thread_debug.h>
#include <ns_msg_signal.h>


#ifdef DBG_MOD_ID
#undef  DBG_MOD_ID
#endif
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_THREAD, 1)

INT g_iThreadSrvInfoID_Fd = 0;
DCL_HEAD_S g_stThreadSrvInfoHead;

/*****************************************************************************
 Prototype    : thread_server_MaskAllSignal
 Description  : 阻塞掉当前线程的所有的信号捕获
 Input        : VOID  
 Output       : None
 Return Value : STATIC
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG thread_server_MaskAllSignal(VOID)
{
	INT		 iRet;
    sigset_t SigMask;
    struct sigaction stSigAct;

	/* 在信号集中打开所有的信号 */
	iRet = sigfillset(&SigMask);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal Fill Set Failed!");
		return ERROR_FAILE;
	}

	/* 屏蔽掉当前线程的所有的信号捕获 */
	iRet = pthread_sigmask(SIG_SETMASK, &SigMask, NULL);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal mask Failed!");
		return ERROR_FAILE;
	}

	/*
		不知道为啥使用上面的操作后，还是能扑捉到SIG_MSG(SIGUSR1),只能这么搞了
	*/
	memset(&stSigAct, 0 ,sizeof(struct sigaction));
	/* 忽略 SIG_MSG(SIGUSR1) */
	iRet = sigfillset(&(stSigAct.sa_mask));
	if (iRet < 0)
	{
		ERR_PRINTF("Sig Empty Set Failed!");
		return ERROR_FAILE;
	}
	stSigAct.sa_handler = SIG_IGN;
	iRet = sigaction(SIG_MSG, &stSigAct, NULL);
	if (iRet < 0)
	{
		ERR_PRINTF("SIG_MSG Signal action Failed!");
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}

/*
	@return:ERROR_SUCCESS
			ERROR_FAILE
			ERROR_QUEMSG_EMPTY
			ERROR_THREAD_EXIT
*/
STATIC ULONG thread_server_QueMsgReadList(IN const THREAD_EPOLL_EVENTS_S *pstThrdEpEvents)
{
    ULONG ulRet = ERROR_SUCCESS;
    INT iSrcThrdId = 0;
    THREAD_QUEMSG_DATA_S stThrdQueMsg;
    THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb;
    

	/* 从消息链表循环读取消息 */
	while(ERROR_SUCCESS == ulRet)
	{
		mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));

		/* 从消息链表读取消息 */
		ulRet = THREAD_server_QueMsg_Recv(pstThrdEpEvents->iThreadId, 										  
										  &iSrcThrdId,
										  &stThrdQueMsg);
		
		if (ERROR_QUEMSG_EMPTY == ulRet)
		{
			SHOW_PRINTF("THREAD server QueMsg Recv EMPTY"); 
			return ERROR_QUEMSG_EMPTY;
		}
		else if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("THREAD server QueMsg Recv Failed!"); 
			return ERROR_FAILE;
		}

		if(THRD_QUEMSG_TYPE_KILL == stThrdQueMsg.uiQueMsgType)
		{
			return ERROR_THREAD_EXIT;
		}

		pfThrdQueMsgRecvCb = (THREAD_SERVER_QUEMSG_RECVCB)pstThrdEpEvents->pPriData;
		if  (NULL == pfThrdQueMsgRecvCb)
		{
			ERR_PRINTF("Thrd QueMsg Recv Callback is invalid!"); 
			return ERROR_FAILE;
		}
		
		pfThrdQueMsgRecvCb(iSrcThrdId ,&stThrdQueMsg);

	}
	
    return ERROR_SUCCESS;
}


/*
	@return:ERROR_SUCCESS
			ERROR_FAILE
			ERROR_QUEMSG_EMPTY
			ERROR_THREAD_EXIT
*/
STATIC ULONG thread_server_QueMsgRecv(IN const THREAD_EPOLL_EVENTS_S *pstThrdEpEvents)
{
	ULONG  ulRet = ERROR_SUCCESS;
	INT    iEvent;
	INT    iReadEventfdLen = 0;
	UINT64 uiEvtData   = 0;

	DBGASSERT(NULL != pstThrdEpEvents);

	iEvent = pstThrdEpEvents->iEventFd;
    MSG_PRINTF("iEventfd = %d", iEvent);
    /*
#ifndef NS_EVENTFD
#else
	iEvent = pstThrdEpEvents->iEventFd;
	iEvent = EVENT_WriteFD_2_ReadFd(pstThrdEpEvents->iEventFd);
#endif
*/

	iReadEventfdLen = read(iEvent, &uiEvtData, sizeof(uiEvtData));
	if (8 != iReadEventfdLen)
	{
        ERR_PRINTF("iReadEventfdLen = %d != 8", iReadEventfdLen);
	    ERR_PRINTF("eventfd read failed!");
		return ERROR_SUCCESS;
	}

    SHOW_PRINTF("****QueMsgRecv - Eventfd read (len = %d)(value = %llu) ****", 
                 iReadEventfdLen, uiEvtData);
	
    if (iReadEventfdLen > 0)
    {
        ulRet = thread_server_QueMsgReadList(pstThrdEpEvents);
	}
	else if (0 == iReadEventfdLen)
	{
	    SHOW_PRINTF("QueMsg iRead Eventfd Len = 0 !");
        ulRet = ERROR_FAILE;
    }
    else
    {
        ERR_PRINTF("-- QueMsg Recv ERROR! --");
        ulRet = ERROR_FAILE;
    }

	return ulRet;

}

/* 线程间消息回调(eventfd事件) */
ULONG Thread_server_QueMsgEpCallBack(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;

	DBGASSERT(NULL !=  arg);
		
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
	    ulErrCode = thread_server_QueMsgRecv(pstThrdEpEvents);
	}

	if ((events & EPOLLHUP) && (pstThrdEpEvents->uiEvents & EPOLLHUP))
	{
		ERR_PRINTF("Thread_server_QueMsg is EPOLLHUP!");
		ulErrCode = ERROR_FAILE;
	}
	
    return ulErrCode;
}

ULONG Thread_server_Init(VOID)
{
	THREAD_epoll_EpEvt_Init();
	DCL_Init(&g_stThreadSrvInfoHead);
	g_iThreadSrvInfoID_Fd = CreateIDPool(THREAD_SERVER_ID_POOL_MAX);
	
	return ERROR_SUCCESS;
}

VOID Thread_server_Fint(VOID)
{
	DestroyIDPool(g_iThreadSrvInfoID_Fd);
	DCL_Fint(&g_stThreadSrvInfoHead);
	THREAD_epoll_EpEvt_Fini();

	return;
}

THREAD_INFO_S *Thread_server_GetByThreadId(IN INT iThreadID)
{
	THREAD_INFO_S *pstThreadInfo = NULL;
	
	DCL_FOREACH_ENTRY(&g_stThreadSrvInfoHead, pstThreadInfo,stNode)
	{
		if (iThreadID == pstThreadInfo->iThreadID)
			break;
	}
		
	return pstThreadInfo;
}

THREAD_INFO_S *Thread_server_GetCurrent(VOID)
{
	pthread_t threadid_self;
	THREAD_INFO_S *pstThreadInfo = NULL;
	
	threadid_self = pthread_self();
	
	DCL_FOREACH_ENTRY(&g_stThreadSrvInfoHead, pstThreadInfo,stNode)
	{
		if (threadid_self == pstThreadInfo->pThreadID)
			break;
	}
		
	return pstThreadInfo;
}

INT Thread_server_GetCurrentThreadId(VOID)
{
	THREAD_INFO_S *pstThreadInfo = NULL;

	pstThreadInfo = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstThreadInfo);
	
	return pstThreadInfo->iThreadID;
}


THREAD_INFO_S *Thread_server_GetNext(IN THREAD_INFO_S *pstThrdPre)
{
	if (NULL == pstThrdPre)
	{
        return DCL_FIRST_ENTRY(&g_stThreadSrvInfoHead,pstThrdPre,stNode);
	}

	return DCL_NEXT_ENTRY(&g_stThreadSrvInfoHead,pstThrdPre,stNode);
}

THREAD_INFO_S *Thread_server_GetByThreadType(IN THREAD_TYPE_E enThreadType)
{
	THREAD_INFO_S *pstThrdInfo = NULL;

	DCL_FOREACH_ENTRY(&g_stThreadSrvInfoHead, pstThrdInfo,stNode)
	{
		if (pstThrdInfo->eThreadType == enThreadType)
			break;
	}
		
	return pstThrdInfo;

}

THREAD_INFO_S * thread_server_Alloc(VOID)
{
	UINT uiRet;
	THREAD_INFO_S *pstThreadInfo = NULL;

	pstThreadInfo = malloc(sizeof(THREAD_INFO_S));
	if (NULL == pstThreadInfo)
		return NULL;
	memset(pstThreadInfo, 0, sizeof(THREAD_INFO_S));

	pstThreadInfo->iThreadID = AllocID(g_iThreadSrvInfoID_Fd ,1);
	if (-1 == pstThreadInfo->iThreadID)
	{
		free(pstThreadInfo);
		return NULL;
	}

	pstThreadInfo->pThreadID       = -1;
	pstThreadInfo->pstThrdEp       = NULL;
	pstThreadInfo->pfThrdSrvInit   = NULL;
	pstThreadInfo->pThrdSrvInitArg = NULL;
	pstThreadInfo->eThreadStatus   = THREAD_STATUS_IDEL;
	pstThreadInfo->eThreadType     = THREAD_TYPE_NONE;
    pstThreadInfo->iEPWaitTimeOut  = -1;
	memset(&(pstThreadInfo->stThrdQueMsgInfo), 0 ,sizeof(THREAD_QUEMSG_INFO_S));
	pstThreadInfo->stThrdQueMsgInfo.iThrdQueMsgEventFd = -1;
	DCL_Init(&(pstThreadInfo->stThrdQueMsgInfo.stThreadQuemsgHead));
	uiRet = pthread_mutex_init(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsg_mutex, NULL);
	if (0 != uiRet)
	{
		ERR_PRINTF("pthread mutex init !");
		goto error_mutex_init;
	}

 	uiRet = pthread_mutex_init(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsgWait_mutex, NULL);
	if (0 != uiRet)
	{
		ERR_PRINTF("pthread wait mutex init !");
		goto error_mutex_init;
	}

	
	uiRet = pthread_cond_init(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsWaticond, NULL);
	if (0 != uiRet)
	{
		ERR_PRINTF("pthread cond init failed!");
		goto error_mutex_init;
	}
	
	pstThreadInfo->stThrdQueMsgInfo.uiThreadQuemsgLen = 0;

	DCL_AddTail(&g_stThreadSrvInfoHead, &(pstThreadInfo->stNode));
	
	return pstThreadInfo;

error_mutex_init:
	(VOID)DeleteID(g_iThreadSrvInfoID_Fd, pstThreadInfo->iThreadID);
	DCL_Del(&(pstThreadInfo->stNode));
	free(pstThreadInfo);

	return NULL;
	
}

STATIC VOID thread_server_Free(UINT uiThreadID)
{
	THREAD_INFO_S *pstThreadInfo = NULL;
	
	pstThreadInfo = Thread_server_GetByThreadId(uiThreadID);
	if (pstThreadInfo != NULL)
	{
		pthread_mutex_destroy(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsg_mutex);
		pthread_mutex_destroy(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsgWait_mutex);
		pthread_cond_destroy(&pstThreadInfo->stThrdQueMsgInfo.ThreadQuemsWaticond);
		
	    (VOID)DeleteID(g_iThreadSrvInfoID_Fd, uiThreadID);
		DCL_Del(&(pstThreadInfo->stNode));
		free(pstThreadInfo);
		pstThreadInfo = NULL;
	}
	
	return;
}

STATIC ULONG thread_server_for_each_Epevt(IN THREAD_INFO_S *pstThrd)
{
    THREAD_EPOLL_EVENTS_S *pstThrdEpEvts  = NULL;
    THREAD_EPOLL_CALL_BACK pfThrdEpCallBack = NULL;

    DBGASSERT(NULL != pstThrd);

    DCL_FOREACH_ENTRY(&(pstThrd->pstThrdEp->stThrdEpEvtHead), pstThrdEpEvts, stNode) {
        pfThrdEpCallBack = (THREAD_EPOLL_CALL_BACK)pstThrdEpEvts->pfCallBack;
        if (pfThrdEpCallBack) {
            pfThrdEpCallBack(0, pstThrdEpEvts);
        }
    }

    return ERROR_SUCCESS;
}

/* 线程主函数 */
VOID * thread_server_main(IN VOID * arg)
{
	ULONG ulRet 	= ERROR_SUCCESS;
    INT   iEWIndex  =  0;
    INT   iEpWaitGetCnt = -1;
    INT   iEpWaitTimeOut = -1;
	THREAD_INFO_S  		  *pstThrd    = NULL;
	THREAD_EPOLL_EVENTS_S *pstThrdEp    	= NULL;
	THREAD_EPOLL_CALL_BACK pfThrdEpCallBack = NULL;
	struct epoll_event    *pstEpEvtGet      = NULL;
	struct epoll_event     stEpEvtGet[THREAD_SERVER_EPOLL_EVENT_NUM_MAX];

	DBGASSERT(NULL != arg);

	pstThrd = (THREAD_INFO_S *)arg;
	pstEpEvtGet   = stEpEvtGet;

	pstThrd->pThreadID = pthread_self(); /* 放到这里可以使得在本线程中使用没有问题 */

	SHOW_PRINTF("Thread[%s] is started !\n"
	            "           Tid:%-3d ptid:0%0X Eid:%-3d TType:%-3d", 
				DBG_THRD_NAME_GET(pstThrd->iThreadID),
				pstThrd->iThreadID, 
				(UINT)pstThrd->pThreadID, 
				pstThrd->pstThrdEp->iEpFd,
				pstThrd->eThreadType);

	Dbg_Thread_SetDbgThreadID(pstThrd->iThreadID);

	ulRet = thread_server_MaskAllSignal();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Mask All Signal Failed!");
		goto thread_error_exit;
	}

	/* Init thread */
	if (NULL != pstThrd->pfThrdSrvInit)
	{
        //FIXME:thread exit event is can not handle. Gog !!!
        ulRet = pstThrd->pfThrdSrvInit(pstThrd);
        if (ERROR_SUCCESS != ulRet)
        {
            goto thread_error_exit;
        }
	}
	
	pstThrd->eThreadStatus = THREAD_STATUS_RUN;
    if (pstThrd->iEPWaitTimeOut > 0) {
        iEpWaitTimeOut = pstThrd->iEPWaitTimeOut;
    }

    /* wait  */
	while(1)
	{
		mem_set0(pstEpEvtGet, 
			     sizeof(struct epoll_event)  * THREAD_SERVER_EPOLL_EVENT_NUM_MAX
			     );
		iEpWaitGetCnt = epoll_wait(pstThrd->pstThrdEp->iEpFd, 
							       pstEpEvtGet, 
							       THREAD_SERVER_EPOLL_EVENT_NUM_MAX, 
							       iEpWaitTimeOut);
		if(iEpWaitGetCnt < 0)
		{
			/* 被信号中断掉，忽略，SIGUSR1很会中断掉 epoll wati */
			if (EINTR == errno)
			{
				MSG_PRINTF("epoll_wait is interrupt by Signal!, ignore.");
				
				continue;
			}
			ERR_PRINTF("epoll_wait Error [errno:%d]!", errno);
			
			break;
		}

        /* 使用time out则不再处理事件，说有注册的函数将被执行  */
        if (iEpWaitTimeOut > 0) {
            //MSG_PRINTF("epoll wait time out.");
            thread_server_for_each_Epevt(pstThrd);
            continue;
        }
		
		/* epoll 事件都会在这 被CallBack  */
		for (iEWIndex = 0; iEWIndex < iEpWaitGetCnt; iEWIndex++)
		{
			pstThrdEp = (THREAD_EPOLL_EVENTS_S *)pstEpEvtGet[iEWIndex].data.ptr;
			pfThrdEpCallBack = (THREAD_EPOLL_CALL_BACK)pstThrdEp->pfCallBack;
			MSG_PRINTF("epoll_wait events   = 0x%0x", pstEpEvtGet[iEWIndex].events);
			MSG_PRINTF("THREAD EPOLL events = 0x%0x", pstThrdEp->uiEvents);
			if (NULL != pfThrdEpCallBack)
			{
				ulRet = pfThrdEpCallBack(pstEpEvtGet[iEWIndex].events, 
										 pstThrdEp);
			}
			else
			{
				ERR_PRINTF("Can't Find Epoll Call Back Function!");
			}

			/* 线程退出 */
			if (ERROR_THREAD_EXIT == ulRet)
			{
				goto thread_error_exit;
			}
		}
	}

thread_error_exit:

	/* Fini the thread */
	if (NULL != pstThrd->pfThrdSrvFini)
	{
        ulRet = pstThrd->pfThrdSrvFini(pstThrd);
        if (ERROR_SUCCESS != ulRet)
        {
			ERR_PRINTF("Finished Thread[%s] Error!", DBG_THRD_NAME_GET(pstThrd->iThreadID));
        }
	}

	
	SHOW_PRINTF("Thread[%s] is EXIT!\n"
	            "           Tid:%-3d ptid:0%0X Eid:%-3d TType:%-3d", 
				DBG_THRD_NAME_GET(pstThrd->iThreadID),
				pstThrd->iThreadID, 
				(UINT)pstThrd->pThreadID, 
				pstThrd->pstThrdEp->iEpFd,
				pstThrd->eThreadType);
				
    THREAD_epoll_Destroy(pstThrd->pstThrdEp);
    thread_server_Free(pstThrd->iThreadID);  
    pstThrd = NULL;
    
	//pthread_exit((VOID *)0);

	return (VOID *)0;

}

/* 为线程添加消息接收的事件 Epoll Event */
STATIC ULONG thread_server_InitQueMsgEpollEvent(IN THREAD_INFO_S *pstThrdSrv,
												IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb)
{
	INT   iEventFd = -1;
	ULONG ulRet    = ERROR_SUCCESS;

	DBGASSERT(NULL != pstThrdSrv);
	DBGASSERT(NULL != pfThrdQueMsgRecvCb);
	
#ifndef NS_EVENTFD
    /* 线程间消息用的eventfd */
    /*
        eventfd 维护了一个 uint64 的 counter
        eventfd 的第一参数为 counter 的初值 ，
                如果设置了这个值，这会使得没有write也会有EPOLLIN
        write(fd, &(uint64), sizeof(uint64)); 
                第二个参数 会 加入counter计数
        read(fd, &(uint64), sizeof(uint64));  
                第二个参数的值和 eventfd的第二个参数有关(EFD_SEMAPHORE)

		EFD_SEMAPHORE:
			设置之后，read一次counter-1

		多次的write只会epoll_wait()检查到一次，
    */
                
	/* iEventFd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE); */
	iEventFd = eventfd(0, EFD_SEMAPHORE);
	if (0 > iEventFd)
	{
        ERR_PRINTF("eventfd create failed!");
        
        return ERROR_FAILE;
	}
    MSG_PRINTF("eventfd = %d", iEventFd);
#else
        //ERR_PRINTF("eventfd create no support!");
        //return ERROR_SUCCESS;
    iEventFd = EVENT_Create();
	if (0 > iEventFd)
	{
        ERR_PRINTF("eventfd create failed!");
        
        return ERROR_FAILE;
	}
    MSG_PRINTF(" read eventfd = %d", iEventFd);
    MSG_PRINTF(" Wiret eventfd = %d", EVENT_ReadFD_2_WriteFd(iEventFd));
#endif
	
	
#ifndef NS_EVENTFD
	pstThrdSrv->stThrdQueMsgInfo.iThrdQueMsgEventFd = iEventFd;
#else
	pstThrdSrv->stThrdQueMsgInfo.iThrdQueMsgEventFd = EVENT_ReadFD_2_WriteFd(iEventFd);
#endif
	pstThrdSrv->stThrdQueMsgInfo.pfThrdQueMsgRecvCb = pfThrdQueMsgRecvCb;

    /* 线程间消息 添加监听 */
#ifndef NS_EVENTFD
	ulRet = THREAD_epoll_EventAdd(iEventFd,
         	                      EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLET,
         	                      pstThrdSrv->iThreadID,
         	                      Thread_server_QueMsgEpCallBack,
         	                      pfThrdQueMsgRecvCb);
#else
	ulRet = THREAD_epoll_EventAdd(iEventFd,
         	                      EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLET,
         	                      pstThrdSrv->iThreadID,
         	                      Thread_server_QueMsgEpCallBack,
         	                      pfThrdQueMsgRecvCb);
#endif
    if (ERROR_SUCCESS != ulRet)
    {
		close(iEventFd);
		
        ERR_PRINTF("EVENT-FD THREAD_epoll_EventAdd failed!");
        return ERROR_FAILE;
    }

    pstThrdSrv->stThrdQueMsgInfo.bThrdQueMsgIsOn = BOOL_TRUE;

	return ERROR_SUCCESS;
}

STATIC ULONG thread_server_FiniQueMsgEpollEvent(IN THREAD_INFO_S *pstThrdSrv)
{
	
	THREAD_epoll_EventDelAll(pstThrdSrv->pstThrdEp);
	
	return ERROR_SUCCESS;
}												

/* 所有的线程创建函数都可以统一成一个接口丫 */
/*
	IN THRD_SRV_INIT_CB pfThrdSrvInit : 线程初始化函数
	IN THREAD_TYPE_E eThrdTyp         : 线程类型
	IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb : 线程间消息接收回调
	如果在线程中使用了while(1),线程将无法被消息【THRD_QUEMSG_TYPE_KILL】杀掉
	如果在线程中使用了whilesleep()，等，线程将法立即响应消息【THRD_QUEMSG_TYPE_KILL】

	关于线程间消息的代码，不能使用需要输出到系统日志的debug,否则会无限的递归。
*/
STATIC INT Thread_server_Create(IN THRD_SRV_INIT_CB pfThrdSrvInit,
								IN THRD_SRV_FINI_CB pfThrdSrvFini,
		 					    IN THREAD_TYPE_E eThrdType,
		 					    IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb,
		 					    IN THREAD_QUEMSGON_E enThrdQuemsgOn,
                                IN INT iEpTimeOut)
{
	INT			   iRet;
	ULONG 		   ulRet 		= ERROR_SUCCESS;
	pthread_attr_t thrd_Attr;
	THREAD_INFO_S *pstThrdSrv   = NULL;
	
	/* 分配节点 & ID */
	pstThrdSrv = thread_server_Alloc();
	if(NULL == pstThrdSrv)
	{
		ERR_PRINTF("Thread node alloc Failed! I think the link U don't init");
		goto error_thread_server_Alloc;
	}

	pstThrdSrv->eThreadType = eThrdType;
    pstThrdSrv->iEPWaitTimeOut = iEpTimeOut;
	
	/* 创建线程EPOLL池  */
	pstThrdSrv->pstThrdEp = THREAD_epoll_Create();
	if(NULL == pstThrdSrv->pstThrdEp)
	{
		ERR_PRINTF("Thread epoll Create Failed!");
		goto error_pthread_epoll_Create;
	}

	if (THREAD_QUEMSG_ON == enThrdQuemsgOn)
	{
		DBGASSERT(NULL != pfThrdQueMsgRecvCb);
		
		ulRet = thread_server_InitQueMsgEpollEvent(pstThrdSrv, pfThrdQueMsgRecvCb);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("thread server Init QueMsg EpollEvent failed!");
			goto error_pthread_Quemsg_init;
		}
	}

	pstThrdSrv->pfThrdSrvInit = pfThrdSrvInit;
	pstThrdSrv->pfThrdSrvFini = pfThrdSrvFini;

	iRet  = pthread_attr_init(&thrd_Attr);
	iRet |= pthread_attr_setdetachstate(&thrd_Attr, PTHREAD_CREATE_DETACHED);
	if (iRet != 0)
	{
        ERR_PRINTF("pthread attr set detached failed!");
		goto error_pthread_attr_init;
	}

	/* 
		启动线程
		设置为 Detached ,防止资源泄露
	*/
    ulRet = pthread_create(&(pstThrdSrv->pThreadID),  /* pstThrdSrv->pThreadID 在 thread_server_main 又来了一遍，保证安全 */
					       &thrd_Attr, 
					       thread_server_main, 
					       (VOID *)(pstThrdSrv));
    pthread_attr_destroy(&thrd_Attr);
    if(0 != ulRet)
    {
        ERR_PRINTF("thread create failed!");
        goto error_pthread_create;
    }

	return pstThrdSrv->iThreadID;
	
error_pthread_create:
	if (THREAD_QUEMSG_ON == enThrdQuemsgOn)
	{
	    thread_server_FiniQueMsgEpollEvent(pstThrdSrv);
	}
    
error_pthread_attr_init:
	(VOID)0;/* 两个各标号之间必须有东西 */
	
error_pthread_Quemsg_init:
	THREAD_epoll_Destroy(pstThrdSrv->pstThrdEp);
    
error_pthread_epoll_Create:
    thread_server_Free(pstThrdSrv->iThreadID);
    pstThrdSrv->iThreadID = -1;	

error_thread_server_Alloc:

    return -1;
}

/*
	IN THRD_SRV_INIT_CB pfThrdSrvInit : 线程初始化函数
	IN THREAD_TYPE_E eThrdTyp         : 线程类型
	IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb : 线程间消息接收回调
	如果在线程中使用了while(1),线程将无法被消息【THRD_QUEMSG_TYPE_KILL】杀掉
	如果在线程中使用了whilesleep()，等，线程将法立即响应消息【THRD_QUEMSG_TYPE_KILL】

	关于线程间消息的代码，不能使用需要输出到系统日志的debug,否则会无限的递归。
*/
INT Thread_server_CreateWithEpQMsg(IN THRD_SRV_INIT_CB pfThrdSrvInit,
								   IN THRD_SRV_FINI_CB pfThrdSrvFini,
	  							   IN THREAD_TYPE_E eThrdType,
	  							   IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb)
{
	INT iThreadId = -1;

	iThreadId = Thread_server_Create(pfThrdSrvInit,
									 pfThrdSrvFini,
									 eThrdType,
									 pfThrdQueMsgRecvCb,
									 THREAD_QUEMSG_ON,
                                     -1);

    return iThreadId;
}

VOID Thread_server_DeleteWithEpQMsg(IN THREAD_INFO_S *pstThrd)
{
	DBGASSERT(NULL != pstThrd);

	/* 或许有没有读完的线程间通信，没有释放，泄漏了 */

	THREAD_epoll_Destroy(pstThrd->pstThrdEp);
		
	thread_server_Free(pstThrd->iThreadID);
			
	return;
}

/* 与Thread_server_CreateWithEpQMsg类似，没有消息列队功能，如果使用了消息列队将会出问题 */
//TODO: how to stop it!!
INT Thread_server_CreateWithEp(IN THRD_SRV_INIT_CB pfThrdSrvInit,
							   IN THRD_SRV_FINI_CB pfThrdSrvFini,
  							   IN THREAD_TYPE_E eThrdType)
{
	INT iThreadId = -1;

	iThreadId = Thread_server_Create(pfThrdSrvInit,
									 pfThrdSrvFini,
									 eThrdType,
									 NULL,
									 THREAD_QUEMSG_OFF,
                                     -1);
	
    return iThreadId;
}

STATIC VOID TIMER_QueMsgRecv_CB(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	ULONG ulRet;
    (VOID)ulRet;

	DBGASSERT(NULL != pstThrdQueMsg);

	ERR_PRINTF("DO NOTHING");
	
    return;
}

INT Thread_server_CreateWithEpTimeOut(IN THRD_SRV_INIT_CB pfThrdSrvInit,
							          IN THRD_SRV_FINI_CB pfThrdSrvFini,
  							          IN THREAD_TYPE_E eThrdType,
                                      IN INT iEpTimeOut)
{
	INT iThreadId = -1;

	iThreadId = Thread_server_Create(pfThrdSrvInit,
									 pfThrdSrvFini,
									 eThrdType,
									 TIMER_QueMsgRecv_CB,
									 THREAD_QUEMSG_ON,
                                     iEpTimeOut);
	
    return iThreadId;
}

VOID Thread_server_DeleteWithEp(IN THREAD_INFO_S *pstThrd)
{
	Thread_server_DeleteWithEpQMsg(pstThrd);
			
	return;
}

VOID Thread_server_KillByThrdID(IN INT iThreadID)
{
	THREAD_QUEMSG_DATA_S stThrdQueMsg;
	
	mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
	stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_KILL;
	stThrdQueMsg.uiQueMsgDataLen  = 0;
	stThrdQueMsg.pQueMsgData  = (VOID *)0;
	
	(VOID)THREAD_server_QueMsg_Send(iThreadID,&(stThrdQueMsg));
	
	return;
}

/*****************************************************************************
 Prototype    : Thread_server_KillAllSonThread
 Description  : 通知所有的子线程 退出，
 				只能由主线程调用。
 				注意这里所有的线程指的是，可以使用quemsg通知删除的线程，
 				其他不监听quemsg Exit消息的线程需要单独通知退出
 				1. Thread_server_CreatWithMain()创建的线程就不会收到这个消息
 Input        : VOID  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
VOID Thread_server_KillAllSonThread(IN INT iMainThreadID)
{
	THREAD_INFO_S *pstThrdEntry = NULL;

	DCL_FOREACH_ENTRY(&g_stThreadSrvInfoHead, pstThrdEntry, stNode)
	{
		if (iMainThreadID != pstThrdEntry->iThreadID)
		{
			Thread_server_KillByThrdID(pstThrdEntry->iThreadID);
		}
	}

	return;
}

STATIC void thread_server_threadwithmain_clean(IN VOID *arg)
{
	THREAD_INFO_S *pstThrdInfo;
	THREAD_MAIN_ARGS_S *pstThrdMainArgs;
	
	DBGASSERT(NULL != arg);

	pstThrdMainArgs = (THREAD_MAIN_ARGS_S *)arg;
	pstThrdInfo = pstThrdMainArgs->pstThrdInfo;

	MSG_PRINTF("Clear Thread(%d) Resources!", pstThrdInfo->iThreadID);

	thread_server_Free(pstThrdInfo->iThreadID);
	free(pstThrdMainArgs);

	return;
}

STATIC VOID * thread_server_threadwithmain(IN VOID *ThrdMArg)
{
	ULONG ulRet;
	THREAD_MAIN_ARGS_S *pstThrdMainArgs;
	
	DBGASSERT(NULL != ThrdMArg);

	pstThrdMainArgs = (THREAD_MAIN_ARGS_S *)ThrdMArg;

	pthread_cleanup_push(thread_server_threadwithmain_clean,
						 (VOID *)pstThrdMainArgs);
	
	Dbg_Thread_SetDbgThreadID(pstThrdMainArgs->pstThrdInfo->iThreadID);
	
	ulRet = thread_server_MaskAllSignal();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Mask All Signal Failed!");
	}
	else
	{
		pstThrdMainArgs->pfThreadMainCB(pstThrdMainArgs->pMainThrdArg);
	}

	pthread_cleanup_pop(1);

	pthread_exit((VOID *)0);
}

/*
	Learn:

	pthread 线程有两种状态，joinable（非分离）状态和detachable（分离）状态，默认为joinable。
	joinable：当线程函数自己返回退出或pthread_exit时都不会释放线程所用资源，包括栈，线程
	描述符等（有人说有8k多，未经验证）。
	detachable:线程结束时会自动释放资源。
	
	Linux man page said:

	When a joinable thread terminates, its memory resources (thread descriptor and stack)
	are not deallocated until another thread performs pthread_join on it. 
	Therefore, pthread_join must be called  once  for each joinable thread created to avoid memory leaks.

	因此，joinable 线程执行完后不使用pthread_join的话就会造成内存泄漏。

	解决办法：

	1.// 创建线程前设置 PTHREAD_CREATE_DETACHED 属性

	pthread_attr_t attr;
	pthread_t thread;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (&thread, &attr, &thread_function, NULL);
	pthread_attr_destroy (&attr);

	2.当线程为joinable时，使用pthread_join来获取线程返回值，并释放资源。

	3.当线程为joinable时，也可在线程中调用 pthread_detach(pthread_self());来分离自己。
*/


/*
	由此函数创建的线程，不具有EPOLL的监听功能，其线程间的通信，不可一异步，只可以使用带等待回复的发送
*/
/*****************************************************************************
 Prototype    : Thread_server_CreatWithMain
 Description  : 创建不带EPOLL监听的线程
 Input        : IN THREAD_TYPE_E eThrdType   	线程类型      
                IN THRD_MAIN_CB  pfThreadMainCB 线程的入口函数 
                IN VOID *pArg                   线程入口函数的入参
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
INT Thread_server_CreatWithMain(IN THREAD_TYPE_E eThrdType,
							    IN THRD_MAIN_CB  pfThreadMainCB,
							    IN VOID *pArg)
{
	INT			   iRet         = 0;
	ULONG		   ulRet		= ERROR_SUCCESS;
	pthread_attr_t attr;
	THREAD_INFO_S *pstThrdSrv	= NULL;
	THREAD_MAIN_ARGS_S *pstThrdMainArgs;

	DBGASSERT(NULL != pfThreadMainCB);
		
	/* 分配节点 & ID */
	pstThrdSrv = thread_server_Alloc();
	if(NULL == pstThrdSrv)
	{
		ERR_PRINTF("Thread node alloc Failed! I think the link U don't init");
		goto error_thread_server_m_Alloc;
	}

	pstThrdSrv->eThreadType = eThrdType;

	/* 设置线程为分离(detachable) */
	iRet =  pthread_attr_init (&attr);
	iRet |= pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (iRet != 0)
	{
		ERR_PRINTF("Thread Set detach state Failed!");
		goto error_thread_server_m_Alloc;
	}

	/* 为了防止进入子线程后，该线程退出释放临时变量，用malloc */
	pstThrdMainArgs = malloc(sizeof(THREAD_MAIN_ARGS_S));
	if (NULL == pstThrdMainArgs)
	{
		ERR_PRINTF("malloc Fialed!");
		return ERROR_FAILE;
	}
	
	memset(pstThrdMainArgs, 0, sizeof(THREAD_MAIN_ARGS_S));
	pstThrdMainArgs->pstThrdInfo = pstThrdSrv;
	pstThrdMainArgs->pMainThrdArg = pArg;
	pstThrdMainArgs->pfThreadMainCB = pfThreadMainCB;

	/* 开启消息列队功能，只可以使用带等待回复的发送 */		
	pstThrdSrv->stThrdQueMsgInfo.bThrdQueMsgIsOn = BOOL_TRUE;
	
	/* 启动线程 */
	ulRet = pthread_create(&(pstThrdSrv->pThreadID), 
						   &attr, 
						   thread_server_threadwithmain, 
						   (pstThrdMainArgs));
	pthread_attr_destroy (&attr);
	if(0 != ulRet)
	{
		ERR_PRINTF("thread create failed!");
		goto error_pthread_m_create;
	}

	return pstThrdSrv->iThreadID;
	
error_pthread_m_create:
	thread_server_Free(pstThrdSrv->iThreadID);

error_thread_server_m_Alloc:

	return -1;
}


VOID Thread_server_DeleteWithMain(IN THREAD_INFO_S *pstThrd)
{
	DBGASSERT(NULL != pstThrd);
	
	thread_server_Free(pstThrd->iThreadID);
			
	return;
}


/*****************************************************************************
 Prototype    : Thread_server_InitWithEp
 Description  : 	
				 和函数 'Thread_server_CreateWithEpQMsg' 
				 的区别在于不需要起线程，
				 没有线程间消息的
				 而是只是设置当前线程，
 
 Input        : IN THREAD_TYPE_E eThrdType  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
INT Thread_server_InitWithEp(IN THREAD_TYPE_E eThrdType)
{
	ULONG ulRet;
	THREAD_INFO_S *pstThrdMain   = NULL;
	
	/* 分配节点 & ID */
	pstThrdMain = thread_server_Alloc();
	if(NULL == pstThrdMain)
	{
		ERR_PRINTF("Thread node alloc Failed! I think the link U don't init");
		goto error_thread_server_Alloc;
	}

	pstThrdMain->eThreadType = eThrdType;
	
	/* 创建线程EPOLL池  */
	pstThrdMain->pstThrdEp = THREAD_epoll_Create();
	if(NULL == pstThrdMain->pstThrdEp)
	{
		ERR_PRINTF("Thread epoll Create Failed!");
		goto error_THREAD_epoll_Create;
	}

	pstThrdMain->pThreadID = pthread_self();
	
	/*  
		除非信号在所有的线程里都阻塞，否则总能将异步信号传输给这个进程。
		因此要保证在所有的子线程里都阻塞了这些信号，
		要不然线程还是会捕获的信号的。
		这里也全关了，要用自已在先获取(MSG_sigthrd_GetCurrentThreadSigMask)再打开。
	*/
	ulRet = thread_server_MaskAllSignal();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Thread Mask All Signal Failed!");
		
		goto error_thread_MaskAllSignal;
	}

	Dbg_Thread_SetDbgThreadID(pstThrdMain->iThreadID);
	
	return pstThrdMain->iThreadID;
	
error_thread_MaskAllSignal:
	THREAD_epoll_Destroy(pstThrdMain->pstThrdEp);
    
error_THREAD_epoll_Create:
    thread_server_Free(pstThrdMain->iThreadID);
    pstThrdMain->iThreadID = -1;

error_thread_server_Alloc:

    return -1;
}

VOID Thread_server_FiniWithEp(IN INT iThreadID)
{
	THREAD_INFO_S *pstThrdSrv	= NULL;

	pstThrdSrv = Thread_server_GetByThreadId(iThreadID);
	if (NULL == pstThrdSrv)
	{
		return;
	}

	THREAD_epoll_Destroy(pstThrdSrv->pstThrdEp);

	thread_server_Free(iThreadID);

	return;
}

/*
	epoll模式中事件可能被触发多次?
	比如socket接收到数据交给一个线程处理数据，
	在数据没有处理完之前又有新数据达到触发了事件，
	另一个线程被激活获得该socket，
	从而产生多个线程操作同一socket，
	即使在ET模式下也有可能出现这种情况。
	采用EPOLLONETSHOT事件的文件描述符上的注册事件只触发一次，
	要想重新注册事件则需要调用epoll_ctl重置文件描述符上的事件，
	这样前面的socket就不会出现竞态。
*/
/*
	注意最后一个参数；处理完后他的返回非成功将导致(当前设计流程)线程退出
*/
ULONG Thread_server_EpollAdd(IN INT iThreadId,
							 IN INT iEvtFd, 
				 			 IN INT iEvents, 
				 			 IN THREAD_EPOLL_CALL_BACK pfThreadCallBack)
{
	UINT		iRet        = 0;
	ULONG 		ulRet 		= ERROR_SUCCESS;

    /* set nonblocking */
    iRet = fcntl(iEvtFd, F_SETFL, O_NONBLOCK);
    if(iRet < 0)
    {
        ERR_PRINTF("fcntl nonblocking failed!");
		return ERROR_FAILE;
    }
	
	/* 添加IO,FD */
	ulRet = THREAD_epoll_EventAdd(iEvtFd,
								  iEvents,
								  iThreadId,
								  pfThreadCallBack,
								  NULL);

	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("THREAD epoll Event Add failed!");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

VOID Thread_server_EpollDel(IN INT iEventFd, 
	 						IN THREAD_EPOLL_S *pstThrdEp)
{
	/* 从Epoll池中摘除监听 */
	THREAD_epoll_EventDel(iEventFd, pstThrdEp);

	return;
}

/*
	创建定时器并加入指定线程的EPOLL,每个链接都有个保活的定时器
	并挂到指定Fd上
*/
ULONG THREAD_server_TimerCreateForEvt(IN INT iThreadId, 
									  IN INT iEventFd,
									  IN INT iMSec,
									  IN timerout_callback_t pfTimerOutCb)
{
	INT   iTimerId;
	ULONG aulPara[4] = {0};
	THREAD_INFO_S *pstThreadInfo;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;

	aulPara[0] = (ULONG)iThreadId;
	aulPara[1] = (ULONG)iEventFd;
	
	iTimerId = TIMER_CreateForThread(iThreadId,
  								     iMSec,
  								     pfTimerOutCb,
  								     aulPara);
	if (0 > iTimerId)
	{
		ERR_PRINTF("TIMER_CreateForThread Failed");

		return ERROR_FAILE;
	}
	MSG_PRINTF("Timer_create Success!");

	pstThreadInfo = Thread_server_GetByThreadId(iThreadId);
	if (NULL == pstThreadInfo)
	{
		ERR_PRINTF("Thread is Lost !");
		return ERROR_FAILE;
	}

	pstThrdEpEvt = Thread_epoll_EpEvtGetByEtId(iEventFd,
											   pstThreadInfo->pstThrdEp);
	if (NULL == pstThrdEpEvt)
	{
		ERR_PRINTF("Thread epoll events Lost !");
		return ERROR_FAILE;
	}

	if (-1 != pstThrdEpEvt->iTimerId)
	{
		TIMER_DeleteForThread(iThreadId, pstThrdEpEvt->iTimerId);
	}
	
	pstThrdEpEvt->iTimerId = iTimerId;

	return ERROR_SUCCESS;
}

VOID inline THREAD_server_StatusChange(THREAD_STATUS_E eThreadStatus)
{
	IN THREAD_INFO_S *pstCurThrdInfo;

	SHOW_PRINTF("Thread Status chang to %s", Thread_debug_GetThreadStatus(eThreadStatus));
	
	pstCurThrdInfo = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstCurThrdInfo);

	pstCurThrdInfo->eThreadStatus = eThreadStatus;

	return;
}

STATIC ULONG thread_server_QueMsg_SendWithLock(IN THREAD_INFO_S *pstDestThrdInfo,   /* 目标线程id */
											   IN INT iQMSNum,   /* 消息序列号 */
			 							       IN BOOL_T bNeedResp,	 /* 是否需要回复该消息 */					    
			    						       IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg) /* 消息(TLV) */
{
    INT    iRet      = 0;
    ULONG  ulRet     = 0;
#ifndef NS_EVENTFD
    UINT64 uiEvtData = 1; /* 必须有值(否则目标线程没有事件)，但是 eventfd write read 的 UINT64 之间的关系不明白 */
#endif
	pthread_mutex_t *pThrdQ_mutex;

	DBGASSERT(NULL != pstThrdQueMsg);

	pThrdQ_mutex = (pthread_mutex_t *)&(pstDestThrdInfo->stThrdQueMsgInfo.ThreadQuemsg_mutex);

	iRet = pthread_mutex_lock(pThrdQ_mutex);
	if (0 != iRet)
	{
	    ERR_PRINTF("Tpthread mutex lock Failed!");
        ulRet = ERROR_FAILE;
		goto exit_func;
	}

	SHOW_PRINTF("THREAD(%d) quemsg Write LOCK", pstDestThrdInfo->iThreadID);

    /* 写入目标线程消息列队 */
    ulRet = THREAD_quemsg_Write(iQMSNum, bNeedResp, &pstDestThrdInfo->stThrdQueMsgInfo, pstThrdQueMsg);
    if (ERROR_SUCCESS != ulRet)
    {
	    ERR_PRINTF("THREAD quemsg Write Failed!");
        iRet = ERROR_FAILE;
		goto error_quemsg_Write;
    }
    
	/* 如果线程不带EPOLL监听，则无需写epollfd,会调用wake up  */
	if (-1 == pstDestThrdInfo->stThrdQueMsgInfo.iThrdQueMsgEventFd)
	{
		MSG_PRINTF("Not need to write Eventfd.");
		goto not_need_writefd;
	}
	
#ifndef NS_EVENTFD
    /* 向目标线程发通知 */
    iRet |= write(pstDestThrdInfo->stThrdQueMsgInfo.iThrdQueMsgEventFd, 
                &uiEvtData, 
                sizeof(uiEvtData));
#else
    iRet |= EVENT_SendEvt(pstDestThrdInfo->stThrdQueMsgInfo.iThrdQueMsgEventFd);
#endif
    if (0 > iRet)
    {
		ERR_PRINTF("QueMSg EVENT write Failed!");
		//THREAD_quemsg_Free(0);/* 未实现 */
		THREAD_quemsg_DestroyAll(&(pstDestThrdInfo->stThrdQueMsgInfo.stThreadQuemsgHead));
		iRet = ERROR_FAILE;
    }

not_need_writefd:
error_quemsg_Write:
	SHOW_PRINTF("THREAD(%d) quemsg Write UNLOCK", pstDestThrdInfo->iThreadID);
 	pthread_mutex_unlock(pThrdQ_mutex);

exit_func:
    return ulRet;
}

STATIC ULONG thread_server_QueMsg_RecvWitLock(OUT   INT *piSrcThrdId,
		 									  INOUT INT *piQMSNum, 
		 									  OUT   BOOL_T *pbNeedResp,
		 									  IN    THREAD_INFO_S *pstThrdInfo,
		 									  OUT   THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    INT   iRet = 0;
    ULONG ulRet;
	pthread_mutex_t *pThrdQ_mutex;
    
	DBGASSERT(NULL != pstThrdInfo);
	DBGASSERT(NULL != piSrcThrdId);
	DBGASSERT(NULL != piQMSNum);
	DBGASSERT(NULL != pbNeedResp);
	DBGASSERT(NULL != pstThrdQueMsg);
	
	pThrdQ_mutex = (pthread_mutex_t *)&(pstThrdInfo->stThrdQueMsgInfo.ThreadQuemsg_mutex);
		
	iRet = pthread_mutex_lock(pThrdQ_mutex);
	if (0 != iRet)
	{
	    ERR_PRINTF("Tpthread mutex lock Failed!");
        ulRet = ERROR_FAILE;
		goto exit_func_read;
	}

	SHOW_PRINTF("THREAD(%d) quemsg Read LOCK", pstThrdInfo->iThreadID);
	
    ulRet = THREAD_quemsg_Read(&(pstThrdInfo->stThrdQueMsgInfo),
							   piSrcThrdId,
							   piQMSNum,
							   pbNeedResp,
							   pstThrdQueMsg);
    /*
    if (ERROR_SUCCESS != ulRet)
    {
        ERR_PRINTF("THREAD quemsg Read Failed");
        return ERROR_FAILE;
    }
    */

//error_quemsg_read:
	SHOW_PRINTF("THREAD(%d) quemsg Read UNLOCK", pstThrdInfo->iThreadID);
	pthread_mutex_unlock(pThrdQ_mutex);

exit_func_read:
	return ulRet;
}

STATIC ULONG thread_server_QueMsg_WaitReap(IN const THREAD_INFO_S *pstDestThrdInfo,
    									   INOUT THREAD_INFO_S *pstSrcThrdInfo,
    									   OUT INT *piSrcThrdId,
    									   IN INT iQMSNum,
    									   OUT THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	ULONG  ulRet;
	BOOL_T bNeedResp;
	struct timeval  now;
	struct timespec outtime;
	pthread_cond_t  *pThrdQWaticond;
	pthread_mutex_t *pThrdQWait_mutex;

	mem_set0(&now,     sizeof(struct timeval));
	mem_set0(&outtime, sizeof(struct timespec));
	
	pThrdQWaticond = &(pstSrcThrdInfo->stThrdQueMsgInfo.ThreadQuemsWaticond);
	pThrdQWait_mutex = &(pstSrcThrdInfo->stThrdQueMsgInfo.ThreadQuemsgWait_mutex);

	SHOW_PRINTF("***WaitReap [cond: 0x%x] [mutex: 0x%x]", (LONG)pThrdQWaticond,(LONG)pThrdQWait_mutex);

    pthread_mutex_lock(pThrdQWait_mutex); 
    /*************** 临界区 *****************/

    while(1)
    {
		ulRet = thread_server_QueMsg_RecvWitLock(
												 piSrcThrdId,
												 &iQMSNum,
												 &bNeedResp,
												 pstSrcThrdInfo,
												 pstThrdQueMsg);
		if (ERROR_SUCCESS == ulRet)
		{
			break;
		}

		SHOW_PRINTF("Try read Quemsg ,but it is empty, continue wait.");
		
    	/*************** 非临界区 ****************/
		//ulRet = pthread_cond_wait(pThrdQWaticond, pThrdQWait_mutex);
		gettimeofday(&now, NULL);
		outtime.tv_sec  = now.tv_sec + 2;
		outtime.tv_nsec = now.tv_usec * 1000;
    	ulRet = pthread_cond_timedwait(pThrdQWaticond, pThrdQWait_mutex, &outtime);
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
			SHOW_PRINTF("Thread(%d) is Wake Up by thread cond time out!", pstSrcThrdInfo->iThreadID);
			SHOW_PRINTF("Thread(%d) is DO not Recvie Response From Thread(%d)!",
						pstSrcThrdInfo->iThreadID,
						pstDestThrdInfo->iThreadID);
			break;
		}
		else
		{
			SHOW_PRINTF("Thread(%d) is Wake Up by thread(%d)", 
 							 pstSrcThrdInfo->iThreadID,
 							 pstDestThrdInfo->iThreadID);
		}

    }
    
    pthread_mutex_unlock(pThrdQWait_mutex);             
    //临界区数据操作完毕，释放互斥锁 
    /*************** 非临界区 ****************/

	pstSrcThrdInfo->stThrdQueMsgInfo.iCurThrdQueMsgSeqNum = iQMSNum;

	return ERROR_SUCCESS;
}

STATIC ULONG thread_server_QueMsg_WakeUpDest(IN const THREAD_INFO_S *pstDestThrdInfo,
    									     IN const THREAD_INFO_S *pstSrcThrdInfo)
{
	pthread_cond_t  *pThrdQWaticond;
	pthread_mutex_t *pThrdQWait_mutex;
	
	pThrdQWaticond = (pthread_cond_t *)&(pstDestThrdInfo->stThrdQueMsgInfo.ThreadQuemsWaticond);
	pThrdQWait_mutex = (pthread_mutex_t *)&(pstDestThrdInfo->stThrdQueMsgInfo.ThreadQuemsgWait_mutex);

	SHOW_PRINTF("***Wake UP [cond: 0x%x] [mutex: 0x%x]", 
				(LONG)pThrdQWaticond,
				(LONG)pThrdQWait_mutex);
	
	pthread_mutex_lock(pThrdQWait_mutex);	 //需要操作临界资源，先加锁，	
	//if (ERROR_SUCCESS == ulRet)
	//{
		pthread_cond_signal(pThrdQWaticond); 
	//}
	
	pthread_mutex_unlock(pThrdQWait_mutex); //解锁		

	return ERROR_SUCCESS;
}

STATIC ULONG thread_server_QueMsg_SendAndWait(IN  INT iDestThrdId,	
    						                  IN  THREAD_QUEMSG_DATA_S *pstThrdQueMsg,
 											  INOUT INT *piSrcThrdId,
    						                  OUT THREAD_QUEMSG_DATA_S *pstRecvThrdQueMsg)
{
	INT   iQMSNum;
	ULONG ulRet;
	BOOL_T bDesThrdIsWaitingForThis = BOOL_FALSE; /* 目标在等自己 */
    THREAD_INFO_S   *pstDestThrdInfo;
    THREAD_INFO_S   *pstSrcThrdInfo;
	
	pstSrcThrdInfo = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstSrcThrdInfo);
	
	pstDestThrdInfo = Thread_server_GetByThreadId(iDestThrdId);
	if (NULL == pstDestThrdInfo)
	{
	    ERR_PRINTF("Thread(id = %d) is not exist !", iDestThrdId);
        return ERROR_FAILE;
	}
	
	if (pstSrcThrdInfo->iThreadID == pstDestThrdInfo->iThreadID) {
		ERR_PRINTF("src thread id == dest thread id == (%d) !", iDestThrdId);
		return ERROR_FAILE;
	}

	if (BOOL_TRUE != pstDestThrdInfo->stThrdQueMsgInfo.bThrdQueMsgIsOn)
	{
	    ERR_PRINTF("This Thread(%s)[%d] is Not Support <Thread Queue Message>!",
	    		   DBG_THRD_NAME_GET(pstDestThrdInfo->iThreadID),
	    		   pstDestThrdInfo->iThreadID);
        return ERROR_QUEMSG_NOT_SUPPORT;
	}
	
	MSG_PRINTF("Dest status(%d) [CurThreadSeqNum %d] == [DestWaitThreadSeqNum %d]",
					pstDestThrdInfo->eThreadStatus,
					pstSrcThrdInfo->stThrdQueMsgInfo.iCurThrdQueMsgSeqNum,
					pstDestThrdInfo->stThrdQueMsgInfo.iThrdWaitQueMsgSeqNum);
				
	/*
	SHOW_PRINTF("pstDestThrdInfo->eThreadStatus = %d", pstDestThrdInfo->eThreadStatus);
	SHOW_PRINTF("pstSrcThrdInfo->stThrdQueMsg.iCurThrdQueMsgSeqNum = %d",
						pstSrcThrdInfo->stThrdQueMsg.iCurThrdQueMsgSeqNum);
	SHOW_PRINTF("pstDestThrdInfo->stThrdQueMsg.iThrdWaitQueMsgSeqNum = %d",
						pstDestThrdInfo->stThrdQueMsg.iThrdWaitQueMsgSeqNum);
	*/
				
	/* 目标线程等着 */
	if (THREAD_STATUS_WAIT == pstDestThrdInfo->eThreadStatus)
	{
		/* 目标线程是否在等当前线程 */
		if (pstSrcThrdInfo->stThrdQueMsgInfo.iCurThrdQueMsgSeqNum
		    == pstDestThrdInfo->stThrdQueMsgInfo.iThrdWaitQueMsgSeqNum)
		{
			SHOW_PRINTF("Dest Thread is waiting for response!");
			bDesThrdIsWaitingForThis = BOOL_TRUE;
			iQMSNum = pstSrcThrdInfo->stThrdQueMsgInfo.iCurThrdQueMsgSeqNum;
		}
	}
	else
	{
		SHOW_PRINTF("New Thread seqence..");
		/* 目标没有等自己，自己发起新的消息序列 */
		iQMSNum = Thread_quemsg_GenerateSeqNum();
	}

	/* 放在这里是使的，在需要目标线程回复时，源线程(当前发送线程)未
		进入wait状态时，目标线程就知道需要回复特定的消息序列号, 并且判断
		是否去唤醒等待的线程 */
	if (NULL != piSrcThrdId && NULL != pstRecvThrdQueMsg)
	{
		//pstSrcThrdInfo->eThreadStatus	= THREAD_STATUS_WAIT;
		
		pstSrcThrdInfo->stThrdQueMsgInfo.iThrdWaitQueMsgSeqNum = iQMSNum;
		THREAD_server_StatusChange(THREAD_STATUS_WAIT); /* 位置不要动，要不然，对方先收到先处理发现发送线程没有WAIT就不回复了 */
	}

	ulRet = thread_server_QueMsg_SendWithLock(pstDestThrdInfo,
	 										  iQMSNum,
	 										  BOOL_FALSE,
	 										  pstThrdQueMsg);
	if (ERROR_SUCCESS != ulRet)
	{
	    ERR_PRINTF("thread server QueMsg Send With Lock failed !");
        return ERROR_FAILE;
	}

	if (bDesThrdIsWaitingForThis == BOOL_TRUE)
	{
		SHOW_PRINTF("---Thread(%d) to Wakeup Dest Thread(%d) ---", 
						pstSrcThrdInfo->iThreadID,
						pstDestThrdInfo->iThreadID);
		thread_server_QueMsg_WakeUpDest(pstDestThrdInfo, pstSrcThrdInfo);
	}

	if (NULL != piSrcThrdId && NULL != pstRecvThrdQueMsg)
	{

		SHOW_PRINTF("...Thread(%d) Wait for Response of thread(%d) ...",
					pstSrcThrdInfo->iThreadID,pstDestThrdInfo->iThreadID);

		thread_server_QueMsg_WaitReap(pstDestThrdInfo,
									  pstSrcThrdInfo,
									  piSrcThrdId,
									  iQMSNum,
									  pstRecvThrdQueMsg);
		
		THREAD_server_StatusChange(THREAD_STATUS_RUN);
	}

	return ulRet;
}

ULONG THREAD_server_QueMsg_Recv(IN INT iThrdId,	
								OUT INT *piSrcThrdId,
						        INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	INT iQMSNum = 0;
	ULONG ulRet;
	BOOL_T pbNeedResp = BOOL_FALSE;
	THREAD_INFO_S *pstSrcThrdInfo;
	
	pstSrcThrdInfo = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstSrcThrdInfo);

	ulRet = thread_server_QueMsg_RecvWitLock(
										     piSrcThrdId,
										     &iQMSNum,
										     &pbNeedResp,
										     pstSrcThrdInfo,
										     pstThrdQueMsg);
										     
	if (ERROR_SUCCESS == ulRet)
	{
			pstSrcThrdInfo->stThrdQueMsgInfo.iCurThrdQueMsgSeqNum = iQMSNum;
	}
	
	return ulRet;
}



ULONG THREAD_server_QueMsg_SendWithResp(IN  INT   iDestThrdId,   /* 目标线程id */
    						            IN  THREAD_QUEMSG_DATA_S *pstThrdQueMsg,
										OUT INT *piSrcThrdId, /* 消息来自哪个线程，应该与参数 iDestThrdId 一样 */
    						            OUT THREAD_QUEMSG_DATA_S *pstRecvThrdQueMsg)
{
	ULONG ulRet;
	ulRet = thread_server_QueMsg_SendAndWait(iDestThrdId,
											 pstThrdQueMsg,
											 piSrcThrdId,
											 pstRecvThrdQueMsg);
	return ulRet;
}

/*
	NOTE:
		异步(无需等待目标线程回复)的消息中pQueMsgData挂的数据要是结构体需要malloc 并有目标线程释放。
		
		同步(需要等待目标线程回复)的消息中可以直接使用临时变量，因为在回复之前函数不会退出的。
		                                           也可以使用malloc的数据，也是需要目标使用完之后释放。

		在回复同步消息时，如果回复不需要再回复，也就是异步消息，同样使用malloc的吧
*/
ULONG THREAD_server_QueMsg_Send(IN INT iThrdId,						    
    						    IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg) /* pstThrdQueMsg 不需要malloc，会自动malloc并拷贝 */
{
	ULONG ulRet;
	ulRet = thread_server_QueMsg_SendAndWait(iThrdId,pstThrdQueMsg,NULL,NULL);
	return ulRet;
}


#if 0
ULONG THREAD_server_QueMsg_SendAsResp(IN INT iThrdId,
 	    						      IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	INT   iQMSNum = 0;
	ULONG ulRet = 0;
    THREAD_INFO_S   *pstSrcThrdInfo;
    THREAD_INFO_S   *pstDestThrdInfo;
    
	pstDestThrdInfo = Thread_server_GetByThreadId(iThrdId);
	if (NULL == pstDestThrdInfo)
	{
	    ERR_PRINTF("Thread(id = %d) is not exist !", iThrdId);
        return NULL;
	}
	
	pstSrcThrdInfo = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstSrcThrdInfo);
	
	iQMSNum = pstSrcThrdInfo->stThrdQueMsg.iCurThrdQueMsgSeqNum;


	ulRet = thread_server_QueMsg_SendWithLock(pstDestThrdInfo, 
											  iQMSNum,
											  BOOL_FALSE, 
											  pstThrdQueMsg);
	pthread_mutex_lock(pThrdQWait_mutex);	 //需要操作临界资源，先加锁，	
	if (ERROR_SUCCESS == ulRet)
	{
		pthread_cond_signal(pThrdQWaticond); 
	}
	
	pthread_mutex_unlock(pThrdQWait_mutex); 	 //解锁  

	return ERROR_SUCCESS;
}
#endif


