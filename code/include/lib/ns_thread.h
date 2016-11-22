#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

#include <ns_thread_epoll.h>
#include <ns_thread_quemsg.h>

#define THREAD_SERVER_INVALID_THREAD  (-1)  
#define THREAD_SERVER_INVALID_EPOLLFD (-1)  

#define THREAD_SERVER_ID_POOL_MAX    (1024)  /* 最大EP */
#define THREAD_SERVER_NUM_MAX        (256)   /* 最大线程连接数 */
#define THREAD_SERVER_INVALID_INFOID (-1)

typedef VOID *(*THRD_MAIN_CB)(VOID *);
typedef ULONG (*THRD_SRV_INIT_CB)(VOID * /* THREAD_INFO_S * */);
typedef THRD_SRV_INIT_CB THRD_SRV_FINI_CB;

typedef enum tag_thread_status
{
	THREAD_STATUS_IDEL = 0,
	THREAD_STATUS_WAIT,
	THREAD_STATUS_BLOCK,
	THREAD_STATUS_RUN,
	THREAD_STATUS_STOP,
	THREAD_STATUS_ERROR,

	THREAD_STATUS_MAX,
}THREAD_STATUS_E;

typedef enum tag_thread_type
{
    THREAD_TYPE_NONE    = 0,
    THREAD_TYPE_MAIN_SERVER,
    THREAD_TYPE_WORK_SERVER,
    THREAD_TYPE_WORK_DISPATCH,
    
	THREAD_TYPE_WEB_DEBGU,

    THREAD_TYPE_LINK,
    THREAD_TYPE_WORK,
    THREAD_TYPE_WEB_SERVER,
	THREAD_TYPE_WEB_SERVER_WROK,
	THREAD_TYPE_WEB_SERVER_WROK_RECV,

	THREAD_TYPE_WEB_ACTION,

	THREAD_TYPE_MIAN,

	THREAD_TYPE_DEBUG,

	THREAD_TYPE_DEBUG_RECV,

	THREAD_TYPE_DEBUG_OUTPUT,
    
	THREAD_TYPE_TIMER,
    
    THREAD_TYPE_MAIN_MAX,
}THREAD_TYPE_E;

/* 线程间通信用的 eventfd信息 */
typedef struct tag_thread_info
{	
	INT 		    iThreadID;          /* 该节点结构服务的线程 */
	pthread_t 	    pThreadID;          /* pthread_creat的返回值 */
	THREAD_TYPE_E   eThreadType;
	THREAD_STATUS_E eThreadStatus;      /* 线程的状态。RUN IDLE WAIT LOCK 等等啊啊 */
	
	THRD_SRV_INIT_CB pfThrdSrvInit;     /* Thread init function call back */
	THRD_SRV_FINI_CB pfThrdSrvFini;     /* Thread Fini function call back */
	VOID*            pThrdSrvInitArg;   /* pfThrdSrvInit 's para */
	
	THREAD_EPOLL_S *pstThrdEp;
	THREAD_QUEMSG_INFO_S stThrdQueMsgInfo;       /* 线程间通信 */

    INT iEPWaitTimeOut;                 /* epoll wait timeout in milliseconds */

	DCL_NODE_S stNode;
}THREAD_INFO_S;

typedef struct tag_thread_main_args
{
	THREAD_INFO_S *pstThrdInfo;
	VOID *pMainThrdArg;
	THRD_MAIN_CB  pfThreadMainCB;
}THREAD_MAIN_ARGS_S;

ULONG Thread_server_Init(VOID);
VOID  Thread_server_Fint(VOID);

THREAD_INFO_S *Thread_server_GetByThreadId(IN INT iThreadID);
THREAD_INFO_S *Thread_server_GetCurrent(VOID);
THREAD_INFO_S *Thread_server_GetNext(IN THREAD_INFO_S *pstThrdPre);
THREAD_INFO_S *Thread_server_GetByThreadType(IN THREAD_TYPE_E enThreadType);

INT Thread_server_CreateWithEpQMsg(IN THRD_SRV_INIT_CB pfThrdSrvInit,
								   IN THRD_SRV_FINI_CB pfThrdSrvFini,
	  							   IN THREAD_TYPE_E eThrdType,
	  							   IN THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb);
INT Thread_server_CreateWithEpTimeOut(IN THRD_SRV_INIT_CB pfThrdSrvInit,
							          IN THRD_SRV_FINI_CB pfThrdSrvFini,
  							          IN THREAD_TYPE_E eThrdType,
                                      IN INT iEpTimeOut);

INT Thread_server_CreatWithMain(IN THREAD_TYPE_E eThrdType,
							    IN THRD_MAIN_CB  pfThreadMainCB,
							    IN VOID *pArg);
INT  Thread_server_InitWithEp(IN THREAD_TYPE_E eThrdType);
VOID Thread_server_FiniWithEp(IN INT iThreadID);

VOID Thread_server_DeleteWithEpQMsg(IN THREAD_INFO_S *pstThrd);

INT Thread_server_CreateWithEp(IN THRD_SRV_INIT_CB pfThrdSrvInit,
							   IN THRD_SRV_FINI_CB pfThrdSrvFini,
  							   IN THREAD_TYPE_E eThrdType);
VOID Thread_server_DeleteWithEp(IN THREAD_INFO_S *pstThrd);

VOID Thread_server_KillByThrdID(IN INT iThreadID);
VOID Thread_server_KillAllSonThread(IN INT iMainThreadID);

ULONG Thread_server_EpollAdd(IN INT iThreadId,
							 IN INT iEvtFd, 
				 			 IN INT iEvents, 
				 			 IN THREAD_EPOLL_CALL_BACK pfThreadCallBack);
				 			 
VOID Thread_server_EpollDel(IN INT iEventFd, 
						    IN THREAD_EPOLL_S *pstThrdEp);
ULONG THREAD_init(VOID);
VOID  THREAD_Fint(VOID);

#include <ns_timer.h>
ULONG THREAD_server_TimerCreateForEvt(IN INT iThreadId, 
									  IN INT iEventFd,
									  IN INT iMSec,
									  IN timerout_callback_t pfTimerOutCb);

ULONG THREAD_server_QueMsg_Send(IN INT iThrdId,						    
    						    IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg);
ULONG THREAD_server_QueMsg_Recv(IN INT iThrdId,	
								OUT INT *piSrcThrdId,
						        INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsg);
ULONG THREAD_server_QueMsg_SendWithResp(IN  INT   iThrdId,   /* 目标线程id */
    						            IN  THREAD_QUEMSG_DATA_S *pstThrdQueMsg,
										OUT INT *piSrcThrdId,
    						            OUT THREAD_QUEMSG_DATA_S *pstRecvThrdQueMsg);

typedef ULONG (* THRD_MAIN_INIT_PF)(VOID *);
typedef ULONG (* THRD_MAIN_EXIT_PF)(VOID *);

typedef struct tag_THREADMAINPROC
{	
	VOID *pInitArg;
	VOID *pExitArg;
	THRD_MAIN_INIT_PF pfThrdMainInit;	/* 主线程初始化回调 */
	THRD_MAIN_EXIT_PF pfThrdMainExit;	/* 线程退出时条件 */
}THREAD_MAIN_PROC_S;

ULONG Thread_main_init(VOID);
VOID Thread_main_Loop(IN THREAD_MAIN_PROC_S *pThrdMainProc);
						        
/**********************************************************/

#endif //__THREAD_H__

