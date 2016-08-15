#ifndef __THREAD_EPOLL_H__
#define __THREAD_EPOLL_H__

#include <ns_lilist.h>

#define THREAD_SERVER_EPOLL_
#define THREAD_SERVER_EPOLL_EVENT_NUM_MAX (32)

typedef ULONG (*THREAD_EPOLL_CALL_BACK)(IN UINT/* events */, IN VOID * /* THREAD_EPOLL_EVENTS_S* */);

typedef enum tag_thread_epoll_status
{
	THREAD_EPOLL_STATUS_IDEL,
	THREAD_EPOLL_STATUS_RUN,
}THREAD_EPOLL_STATUS_S;

#define THRD_EPEVT_SET(pstThrdEpEvt, iEvtFd, iThrdId, iEvts ,pfThrdCB, pPriD) \
	(pstThrdEpEvt)->iEventFd   = (iEvtFd);		\
	(pstThrdEpEvt)->iThreadId  = (iThrdId);		\
	(pstThrdEpEvt)->uiEvents   = (iEvts);		\
	(pstThrdEpEvt)->pfCallBack = (pfThrdCB);	\
	(pstThrdEpEvt)->pPriData   = (pPriD);

typedef struct tag_thread_events  
{  
	DCL_NODE_S stNode;
	
	INT    iThreadId;		/* 所在线程 ID 		*/
	INT    iEventFd;    	/* 监听的Fd 		*/
	UINT   uiEvents;		/* 监听的事件 		*/
	VOID  *pfCallBack;		/* 回调 			*/
	UINT  *aulData[4];		/* 未使用 			*/
	INT    iTimerId;		/* 事件监听定时器 	*/
	VOID  *pPriData;		/* 私有数据 (有可能存在泄漏)		*/
}THREAD_EPOLL_EVENTS_S;


typedef struct tagThreadEpollInfo
{
	INT  iEpFd;

	DCL_HEAD_S stThrdEpEvtHead;
}THREAD_EPOLL_S;

THREAD_EPOLL_EVENTS_S *Thread_epoll_EpEvtGetByEtId(IN INT iEventId,
												  IN THREAD_EPOLL_S *pstThrdEp);
												  
ULONG THREAD_epoll_EpEvt_Init(VOID);

VOID THREAD_epoll_EpEvt_Fini(VOID);

/*****************************************************************************/

ULONG THREAD_epoll_EventSet(IN    INT   iEvtFd, 
							IN    INT   events,
     						IN    VOID *call_back,
     					    INOUT THREAD_EPOLL_EVENTS_S *pstThrdEpEv); 
     					    
ULONG THREAD_epoll_EventAdd(IN INT iEvtFd, 
							IN INT iEvent, 
							IN INT iThreadId,
							IN THREAD_EPOLL_CALL_BACK pfThrdEpCallBadk,
							IN VOID *pPriData);
							
ULONG THREAD_epoll_EventMod(IN INT iEvtFd, 
							IN INT iEvent, 
							IN INT iThreadId,
							IN THREAD_EPOLL_CALL_BACK pfThrdEpCallBadk,
							IN VOID *pPriData);
							
VOID THREAD_epoll_EventDel(IN INT iEventFd, IN THREAD_EPOLL_S *pstThrdEp); 

VOID THREAD_epoll_EventDelAll(IN THREAD_EPOLL_S *pstThrdEp);
							
THREAD_EPOLL_S *THREAD_epoll_Create(VOID);

VOID THREAD_epoll_Destroy(IN THREAD_EPOLL_S *pstThrdEp);

//*******************************************************
//While Delete

//*******************************************************

#endif //__THREAD_EPOLL_H__
