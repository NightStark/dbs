/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : web_main_init.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/9/14
  Last Modified :
  Description   : [主线程] 初始化 处理
  Function List :
              Web_server_main
              thread_EpollWait
              web_thread_main_epcallback
  History       :
  1.Date        : 2014/9/14
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <asm/errno.h>
#include <sys/signalfd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_base.h>
#include <ns_web_server.h>
#include <ns_thread.h>
#include <ns_timer.h>

#ifdef  DBG_MOD_ID
#undef  DBG_MOD_ID
#endif
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_THREAD, 1)

#ifndef NS_EVENTFD
/*****************************************************************************
 Prototype    : web_thread_main_epcallback
 Description  : 处理信号
 Input        : IN UINT uiEvents  
                IN VOID *pArg     
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG web_thread_main_epcallback(IN UINT uiEvents, IN VOID *pArg)
{	
    ssize_t  SigFdLen;
    INT iSignalFd;
    struct signalfd_siginfo stSigFdInfo;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;

	DBGASSERT(NULL !=  pArg);
		
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)pArg;
	iSignalFd = pstThrdEpEvents->iEventFd;

	if ((uiEvents & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		/*for (;;)*/
		{
			SigFdLen = read(iSignalFd, &stSigFdInfo, sizeof(struct signalfd_siginfo));
			if (SigFdLen != sizeof(struct signalfd_siginfo))
			   ERR_PRINTF("read");
		
			if (stSigFdInfo.ssi_signo == SIGINT) {
			   SHOW_PRINTF("Got SIGINT");
			} else if (stSigFdInfo.ssi_signo == SIGQUIT) {
			   SHOW_PRINTF("Got SIGQUIT");
			  
			   return ERROR_THREAD_EXIT;
			} else {
			   ERR_PRINTF("Read unexpected signal\n");
			}
		}
	}

	return ERROR_SUCCESS;
}
#endif

/*****************************************************************************
 Prototype    : thread_EpollWait
 Description  : Epoll Wait 信号 等事件
 Input        : IN VOID * arg  
 Output       : None
 Return Value : VOID
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/13
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC VOID *thread_EpollWait(IN THREAD_MAIN_PROC_S *pThrdMainProc)
{
	ULONG ulRet 	= ERROR_SUCCESS;
    INT   iEWIndex  =  0;
    INT   iEpWaitGetCnt = -1;
    INT   iMainThrdId   = -1;
	THREAD_INFO_S  		  *pstThrd    = NULL;
	THREAD_EPOLL_EVENTS_S *pstThrdEp    	= NULL;
	THREAD_EPOLL_CALL_BACK pfThrdEpCallBack = NULL;
	struct epoll_event    *pstEpEvtGet      = NULL;
	struct epoll_event     stEpEvtGet[THREAD_SERVER_EPOLL_EVENT_NUM_MAX];

	DBGASSERT(NULL != pThrdMainProc);

	pstThrd   = Thread_server_GetCurrent();
	if (NULL == pstThrd)
	{
		return (VOID *)-1;
	}

	iMainThrdId = pstThrd->iThreadID;

	SHOW_PRINTF("Thread[%s] in Epoll Wait !\n"
	            "           Tid:%-3d ptid:0%0X Eid:%-3d TType:%-3d", 
				DBG_THRD_NAME_GET(pstThrd->iThreadID),
				pstThrd->iThreadID, 
				(UINT)pstThrd->pThreadID, 
				pstThrd->pstThrdEp->iEpFd,
				pstThrd->eThreadType);

	if (NULL != pThrdMainProc->pfThrdMainInit)
	{
		pThrdMainProc->pfThrdMainInit(pThrdMainProc->pInitArg);
	}
	
	pstEpEvtGet   = stEpEvtGet;

    /* wait  */
	while(1)
	{
		mem_set0(pstEpEvtGet, 
			     sizeof(struct epoll_event)  * THREAD_SERVER_EPOLL_EVENT_NUM_MAX);
		iEpWaitGetCnt = epoll_wait(pstThrd->pstThrdEp->iEpFd, 
							       pstEpEvtGet, 
							       THREAD_SERVER_EPOLL_EVENT_NUM_MAX, 
							       -1);
		if(iEpWaitGetCnt < 0)
		{
			ERR_PRINTF("epoll_wait Error!");
			break;
		}
		
		/* epoll 事件都会在这 被CallBack  */
		for (iEWIndex = 0; iEWIndex < iEpWaitGetCnt; iEWIndex++)
		{
			pstThrdEp = (THREAD_EPOLL_EVENTS_S *)pstEpEvtGet[iEWIndex].data.ptr;
			pfThrdEpCallBack = (THREAD_EPOLL_CALL_BACK)pstThrdEp->pfCallBack;
			MSG_PRINTF("Epoll  Wait  events = 0x%0x", pstEpEvtGet[iEWIndex].events);
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

	SHOW_PRINTF("Thread[%s] is goint to EXIT!\n"
	            "           Tid:%-3d ptid:0%0X Eid:%-3d TType:%-3d", 
				DBG_THRD_NAME_GET(pstThrd->iThreadID),
				pstThrd->iThreadID, 
				(UINT)pstThrd->pThreadID, 
				pstThrd->pstThrdEp->iEpFd,
				pstThrd->eThreadType);

				
	SHOW_PRINTF("Kill all Son Thread over Quemsg!...");

#ifndef NS_EVENTFD
	MSG_PRINTF("Destroy All Timer !");
    TIMER_thread_Fini();
#endif

	Thread_server_KillAllSonThread(iMainThrdId);
	
	MSG_PRINTF("Free Main Thread info !");
	Thread_server_FiniWithEp(pstThrd->iThreadID);
	pstThrd = NULL;
	
	if (NULL != pThrdMainProc->pfThrdMainExit)
	{
		pThrdMainProc->pfThrdMainExit(pThrdMainProc->pExitArg);
	}

	/* 以下两个while等待循环，有待加入计时，超出后强制退出 */
	/* 等待所有的线程释放完毕 */
#ifndef NS_EVENTFD
    while(1)
    {
		if (NULL == Thread_server_GetNext(NULL))
		{
			break;
		}

    }
#else
#endif
	SHOW_PRINTF("All Son Thread is Killed!!");	
	
	
	SHOW_PRINTF("Thread[ MAIN ] is EXIT! OVER\n");

	return (VOID *)0;

}

STATIC VOID sa_s_handler(IN INT iSigNum)
{
    MSG_PRINTF("sig num = %d");
    //TODO:sent a event to main/ or create a imp:my_sigfd
    exit(0);
    return;
}

/*****************************************************************************
 Prototype    : thread_main_init
 Description  : 主线初始化，并监听信号
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
ULONG Thread_main_init(VOID)
{
	INT		 iRet;
    sigset_t SigMask;
#ifndef NS_EVENTFD
	ULONG    ulRet;
    INT      iSignalFd;
#else
    struct sigaction sa_s;
#endif
    INT      iThrdID;
	
	iThrdID = Thread_server_InitWithEp(THREAD_TYPE_MIAN);
	if (iThrdID < 0)
	{
		ERR_PRINTF("Main Thread init Failed!");
		goto error_exit;
	}
	DBG_THRD_NAME_REG(iThrdID, "MAIN");
	
#ifndef NS_EVENTFD
	iRet  = sigemptyset(&SigMask);
	iRet |= sigaddset(&SigMask, SIGINT);
	iRet |= sigaddset(&SigMask, SIGQUIT);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal set Failed!");
		goto error_sigmask_set;
	}

//#/TODO:
	iSignalFd = signalfd(-1, &SigMask, 0);
	if (-1 == iSignalFd)
	{
		ERR_PRINTF("Signal mask Failed!");
		goto error_sigfd;
	}

	ulRet = THREAD_epoll_EventAdd(iSignalFd, 
						  		  EPOLLIN | EPOLLET,
						  		  iThrdID,
						  		  web_thread_main_epcallback,
						  		  NULL);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Signal Fd Add Epoll Failed!");
		goto error_signal_epoll_add;
	}
#else
    ERR_PRINTF("Main Thread init ********************************");
    /******************************************
     * like this, block a empty can not clear, JUST block new nothing
	 * iRet  = sigemptyset(&SigMask);
     * sigprocmask(SIG_BLOCK, &SigMask, &oldset);
     *
     * !! want clear a sig must user SIG_UNBLOCK!!
     ****************************************/
	iRet  = sigfillset(&SigMask);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal set Failed!");
		goto error_sigmask_set;
	}
    sigset_t oldset;
    sigprocmask(SIG_UNBLOCK, &SigMask, &oldset);
    sigprocmask(SIG_BLOCK, NULL, &oldset);
    ERR_PRINTF("[0x%X]", oldset);
    sa_s.sa_flags = 0;
    sa_s.sa_handler = sa_s_handler;
    sigaction(SIGINT,  &sa_s, NULL);
    sigaction(SIGQUIT, &sa_s, NULL);
    sigaction(SIGTERM, &sa_s, NULL);
    //signal(SIGINT,  sa_s_handler);
    //signal(SIGQUIT,  sa_s_handler);
    //signal(SIGTERM,  sa_s_handler);
#endif

	return ERROR_SUCCESS;
	
#ifndef NS_EVENTFD
error_signal_epoll_add:
	close(iSignalFd);
error_sigfd:
	(VOID)0;
#endif
error_sigmask_set:
	Thread_server_FiniWithEp(iThrdID);
error_exit:
	
	return ERROR_FAILE;
}

VOID Thread_main_Loop(IN THREAD_MAIN_PROC_S *pThrdMainProc)
{
	thread_EpollWait(pThrdMainProc);

	return;
}

