#ifndef __THREAD_EPOLL_C__
#define __THREAD_EPOLL_C__

#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <errno.h>  


#include <ns_base.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_thread.h>
#include <ns_thread_epoll.h>
#include <ns_timer.h>

#ifdef  DBG_MOD_ID
#undef  DBG_MOD_ID
#endif
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_GET, 1)

//DCL_HEAD_S g_stThreadEpollHead;

/* 问题是一个线程有可能有好多的epoll,那就有点麻烦了 
THREAD_EPOLL_EVENTS_S *thread_epoll_info_GetByThreadId(INT iThreadId)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpInfo;

	DCL_FOREACH_ENTRY(&g_stThreadEpollHead, pstThrdEpInfo, stNode)
	{
		if (iThreadId == pstThrdEpInfo->iThreadId)
		{
			break;
		}
	}

	return pstThrdEpInfo;
}
*/

THREAD_EPOLL_EVENTS_S *Thread_epoll_EpEvtGetByEtId(IN INT iEventId,
												   IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpInfo;

	DCL_FOREACH_ENTRY(&(pstThrdEp->stThrdEpEvtHead), pstThrdEpInfo, stNode)
	{
		if (iEventId == pstThrdEpInfo->iEventFd)
		{
			break;
		}
	}

	return pstThrdEpInfo;
}

THREAD_EPOLL_EVENTS_S *Thread_epoll_EpEvtGetNext(IN THREAD_EPOLL_EVENTS_S *pstThrdEpEvts,
												 IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvtEntry = NULL;

	if (NULL == pstThrdEpEvts)
	{
		return DCL_FIRST_ENTRY(&(pstThrdEp->stThrdEpEvtHead),pstThrdEpEvtEntry, stNode);
	}
	else
	{
		return DCL_NEXT_ENTRY(&(pstThrdEp->stThrdEpEvtHead),pstThrdEpEvtEntry, stNode);
	}
}

STATIC THREAD_EPOLL_EVENTS_S *thread_epoll_EpEvtCreate(IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;
	
	pstThrdEpEvt = malloc(sizeof(THREAD_EPOLL_EVENTS_S));
	if (NULL == pstThrdEpEvt)
	{
		return NULL;
	}
	memset(pstThrdEpEvt, 0, sizeof(THREAD_EPOLL_EVENTS_S));

	pstThrdEpEvt->iEventFd      = -1;
	pstThrdEpEvt->iThreadId     = -1;
	pstThrdEpEvt->iTimerId      = -1;
	pstThrdEpEvt->pfCallBack    = NULL;
	pstThrdEpEvt->pPriData      = NULL;
	pstThrdEpEvt->uiEvents      = 0;

	DCL_AddHead(&(pstThrdEp->stThrdEpEvtHead), &(pstThrdEpEvt->stNode));

	return pstThrdEpEvt;
}

STATIC VOID thread_epoll_EpEvtDestroy(IN INT iEventFd, IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;

	pstThrdEpEvt = Thread_epoll_EpEvtGetByEtId(iEventFd, pstThrdEp);
	if (NULL == pstThrdEpEvt)
	{
		return;
	}

#if 0 //FIXME:in here is not good?
	/* 关闭可能打开的定时器 */
	if (-1 != pstThrdEpEvt->iTimerId)
	{
		TIMER_DeleteForThread(pstThrdEpEvt->iThreadId, pstThrdEpEvt->iTimerId);
		pstThrdEpEvt->iTimerId = -1;
	}
#endif

	DCL_Del(&(pstThrdEpEvt->stNode));
	free(pstThrdEpEvt);

	return;
}

VOID THREAD_epoll_EpEvDestroyAll(IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;
	
	DCL_FOREACH_ENTRY(&(pstThrdEp->stThrdEpEvtHead),pstThrdEpEvt,stNode);
	{
		DCL_DelFirst(&(pstThrdEp->stThrdEpEvtHead), free);
	}

	return;
}

ULONG THREAD_epoll_EpEvt_Init(VOID)
{
	//DCL_Init(&g_stThreadEpollHead);

	return ERROR_SUCCESS;
}

VOID THREAD_epoll_EpEvt_Fini(VOID)
{
	
	return;
}

/*****************************************************************************/

// set event  
ULONG THREAD_epoll_EventSet(IN    INT   iEvtFd, 
							IN    INT   events,
     						IN    VOID *call_back,
     					    INOUT THREAD_EPOLL_EVENTS_S *pstThrdEpEv)  
{
	pstThrdEpEv->iEventFd   = iEvtFd;
	pstThrdEpEv->pfCallBack = call_back;
	pstThrdEpEv->uiEvents   = events;
 
    return ERROR_SUCCESS;
}  

// add/mod an event to epoll  
ULONG THREAD_epoll_EventAdd(IN INT iEvtFd, 
							IN INT iEvent, 
							IN INT iThreadId,
							IN THREAD_EPOLL_CALL_BACK pfThrdEpCallBadk,
							IN VOID *pPriData)  
{  
	UINT uiRet = 0;
	THREAD_INFO_S* pstThrd;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;
    struct epoll_event epv = {0, {0}}; 

	pstThrd = Thread_server_GetByThreadId(iThreadId);
	if (NULL == pstThrd)
	{
		ERR_PRINTF("Thread_server_GetByThreadId Failed!");
		return ERROR_FAILE;
	}
    
	pstThrdEpEvt = thread_epoll_EpEvtCreate(pstThrd->pstThrdEp);
	if (NULL == pstThrdEpEvt)
	{
		ERR_PRINTF("thread_epoll_EpEvtCreate Failed!");
		return ERROR_FAILE;
	}

	THRD_EPEVT_SET(pstThrdEpEvt,
					iEvtFd,
					pstThrd->iThreadID,
					iEvent,
					pfThrdEpCallBadk,
					pPriData);	

    epv.events 	 = iEvent; 
	epv.data.ptr = pstThrdEpEvt;  

	if (0 > pstThrd->pstThrdEp->iEpFd || 0 > iEvtFd)
	{
		ERR_PRINTF("Epoll Fd OR Event Fd is INVALID !");
		return ERROR_FAILE;
	}

    uiRet = epoll_ctl(pstThrd->pstThrdEp->iEpFd, 
					  EPOLL_CTL_ADD, 
					  iEvtFd, 
					  &epv);
    if(uiRet < 0)  
    {
        ERR_PRINTF("Event Add failed  [fd=%d], evnets[%0X] ,[epollfd=%d]", 
			        pstThrdEpEvt->iEventFd, 
			        epv.events,
			        pstThrd->pstThrdEp->iEpFd);  
        return ERROR_FAILE;
    }
    else  
    {
        MSG_PRINTF("Event Add SUCCESS [fd=%d], evnets[%0X] ,[epollfd=%d]", 
			        pstThrdEpEvt->iEventFd, 
			        epv.events,
			        pstThrd->pstThrdEp->iEpFd);  
    }

    return ERROR_SUCCESS;
}

ULONG THREAD_epoll_EventMod(IN INT iEvtFd, 
						    IN INT iEvent, 
						    IN INT iThreadId,
						    IN THREAD_EPOLL_CALL_BACK pfThrdEpCallBadk,
						    IN VOID *pPriData)
{
	UINT uiRet = 0;
	THREAD_INFO_S* pstThrd;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt;
    struct epoll_event epv = {0, {0}}; 

	pstThrd = Thread_server_GetByThreadId(iThreadId);
	if (NULL == pstThrd)
	{
		ERR_PRINTF("Thread_server_GetByThreadId Failed!");
		return ERROR_FAILE;
	}
    
	pstThrdEpEvt = Thread_epoll_EpEvtGetByEtId(iEvtFd, pstThrd->pstThrdEp);
	if (NULL == pstThrdEpEvt)
	{
		ERR_PRINTF("thread epoll event is Already Deleted!");
		return ERROR_FAILE;
	}

	THRD_EPEVT_SET(pstThrdEpEvt,
					iEvtFd,
					pstThrd->iThreadID,
					iEvent,
					pfThrdEpCallBadk,
					pPriData);	

    epv.events 	 = iEvent; 
	epv.data.ptr = pstThrdEpEvt;  

	if (0 > pstThrd->pstThrdEp->iEpFd || 0 > iEvtFd)
	{
		ERR_PRINTF("Epoll Fd OR Event Fd is INVALID !");
		return ERROR_FAILE;
	}

    uiRet = epoll_ctl(pstThrd->pstThrdEp->iEpFd, 
					  EPOLL_CTL_MOD, 
					  iEvtFd, 
					  &epv);
    if(uiRet < 0)  
    {
        ERR_PRINTF("Event MOD failed  [fd=%d], evnets[%0X]", 
			        pstThrdEpEvt->iEventFd, 
			        epv.events);  
        return ERROR_FAILE;
    }
    else  
    {
        MSG_PRINTF("Event MOD SUCCESS [fd=%d], evnets[%0X]", 
			        pstThrdEpEvt->iEventFd, 
			        epv.events);  
    }

    return ERROR_SUCCESS;
}

/*
	把 Epoll 从Epoll池中摘除监听
	释放其对应的Event数据结构体!
*/
VOID THREAD_epoll_EventDel(IN INT iEventFd, IN THREAD_EPOLL_S *pstThrdEp)  
{
	UINT uiRet = 0;
	
	/* 从Epoll池中摘除监听 */
    uiRet = epoll_ctl(pstThrdEp->iEpFd, EPOLL_CTL_DEL, iEventFd, NULL);  
    if(uiRet < 0)  
    {
        ERR_PRINTF("Event Delete failed");  
        return;
    }
    
	/* 释放EVENT 数据 */
	thread_epoll_EpEvtDestroy(iEventFd, pstThrdEp);

	close(iEventFd);
    
    return;
}  

VOID THREAD_epoll_EventDelAll(IN THREAD_EPOLL_S *pstThrdEp)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvt = NULL;
	//THREAD_EPOLL_EVENTS_S *pstThrdEpEvtEext = NULL;

	DBGASSERT(NULL != pstThrdEp);

	for(;;)
	{
		pstThrdEpEvt = Thread_epoll_EpEvtGetNext(pstThrdEpEvt, pstThrdEp);
		if (NULL == pstThrdEpEvt)
		{
			break;
		}
		
        //FIXME:if iEvent is Timer Need to Delete it !!
#ifndef NS_EVENTFD
#else
        if (TIMER_info_ReadFdisTimerFd(pstThrdEpEvt->iEventFd)) {
            TIMER_DeleteForThread(pstThrdEpEvt->iThreadId, pstThrdEpEvt->iEventFd);
        } else 
#endif
        {
            THREAD_epoll_EventDel(pstThrdEpEvt->iEventFd,pstThrdEp);
            pstThrdEpEvt = NULL;
        }
	}
	/*
	DCL_FOREACH_ENTRY_SAFE(&(pstThrdEp->stThrdEpEvtHead), pstThrdEpEvt, pstThrdEpEvtEext, stNode)
	{
		if (-1 != pstThrdEpEvt->iEventFd)
		{
			THREAD_epoll_EventDel(pstThrdEpEvt->iEventFd,pstThrdEp);
			pstThrdEpEvt = NULL;
		}
	}
	*/
	
	return;
}

THREAD_EPOLL_S *THREAD_epoll_Create(VOID)
{
	INT iEpFd  = -1;
	THREAD_EPOLL_S *pstThrdEp;

	pstThrdEp = mem_alloc(sizeof(THREAD_EPOLL_S));
	if (NULL == pstThrdEp)
	{
        ERR_PRINTF("mem_alloc Failed!");
		return NULL;
	}

	
	iEpFd = epoll_create(THREAD_SERVER_EPOLL_EVENT_NUM_MAX);
    if (0 > iEpFd)
    {
    	free(pstThrdEp);
        ERR_PRINTF("Thread epoll_create Failed!");
        return NULL;
    }
	
	pstThrdEp->iEpFd = iEpFd;
	DCL_Init(&(pstThrdEp->stThrdEpEvtHead));
	
    return pstThrdEp;
}

VOID THREAD_epoll_Destroy(IN THREAD_EPOLL_S *pstThrdEp)
{
	/* 摘除在该EPOLL上监听的FD */
	THREAD_epoll_EventDelAll(pstThrdEp);

	/* 释放所有的EPOLL Event 节点 */
	//thread_epoll_EpEvDestroyAll(pstThrdEp);
	
    close(pstThrdEp->iEpFd);
	free(pstThrdEp);
	return;
}

/*************************************************************************/

#endif //__THREAD_EPOLL_C__

