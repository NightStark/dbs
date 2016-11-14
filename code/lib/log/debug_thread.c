/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : debug_thread.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/11/24
  Last Modified :
  Description   : Debug线程和线程间通信

  History       :
  1.Date        : 2014/11/24
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <sys/epoll.h>
#ifndef NS_EVENTFD
#include <sys/eventfd.h>
#include <bits/eventfd.h>
#endif
#include <signal.h>

#include <ns_base.h>

#include <ns_process.h>
#include <ns_event.h>
#include <ns_thread.h>
#include <ns_msgque.h>
#include <ns_msg_signal.h>

/* 线程私有数据，快速标识线程身份 */
/* STATIC __thread不能使static的 */
__thread INT iThreadPriID    = 0;
/* 线程私有数据，快速标识(Debug)线程身份 */
STATIC INT iDbgThreadPriID = 0;

/* 用于标识线程退出时告知，待优化 */
STATIC DBG_THRD_PRINTF_FLAG_E g_enDbgThreadPrintFlag = DBG_TPF_NONE;
STATIC INT g_iDbgThreadEventFd = -1;
/* 标识线程的状态，便于打印，和主线程的退出时判断 */
STATIC THREAD_STATUS_E enDbgThreadStatus = THREAD_STATUS_IDEL;

/* 判断当前线程是否为Debug线程 */
//TODO:need a lock?
inline BOOL_T Dbg_Thread_ISDebugThread(VOID)
{
	if(iThreadPriID == iDbgThreadPriID)
	{
		return BOOL_TRUE;
	}
	else
	{
		return BOOL_FALSE;
	}
}

//TODO:need a lock?
inline VOID Dbg_Thread_SetDbgThreadPrintFlag(DBG_THRD_PRINTF_FLAG_E enDbgFlag)
{
	g_enDbgThreadPrintFlag = enDbgFlag;
	return;
}

inline DBG_THRD_PRINTF_FLAG_E Dbg_Thread_GetDbgThreadPrintFlag(VOID)
{
	return g_enDbgThreadPrintFlag;
}

inline INT Dbg_Thread_GetDbgThreadEventFd(VOID)
{
	return g_iDbgThreadEventFd;
}

VOID Dbg_Thread_SetDbgThreadID(IN INT iDbgThrdId)
{
	iThreadPriID = iDbgThrdId;

	return;
}

VOID Dbg_Thread_SetRun(VOID)
{
	enDbgThreadStatus = THREAD_STATUS_RUN;
	
	DBGMSG_PRINTF("**** Debug thread is START ****");
	
	return;
}

VOID Dbg_Thread_SetStop(VOID)
{
	enDbgThreadStatus = THREAD_STATUS_STOP;
	
	DBGMSG_PRINTF("**** Debug thread is STOP ****");
	
	return;
}

BOOL_T DBG_thread_IsRun(VOID)
{
	if (enDbgThreadStatus == THREAD_STATUS_RUN)
	{
		return BOOL_TRUE;
	}
	else
	{
		return BOOL_FALSE;
	}
}

BOOL_T DBG_thread_IsStop(VOID)
{
	if (enDbgThreadStatus == THREAD_STATUS_STOP)
	{
		return BOOL_TRUE;
	}
	else
	{
		return BOOL_FALSE;
	}
}

STATIC DCL_HEAD_S g_DbgPrintListHead;
STATIC pthread_mutex_t DbgThrdMutex;

STATIC VOID dbg_print_ListInit(VOID)
{
	DCL_Init(&g_DbgPrintListHead);
	pthread_mutex_init(&DbgThrdMutex, NULL);

	return;
}

STATIC VOID dbg_print_ListFini(VOID)
{
	pthread_mutex_destroy(&DbgThrdMutex);
	DCL_Init(&g_DbgPrintListHead);

	return;
}

STATIC inline ULONG dbg_thread_RecvEvent(IN INT iEvent)
{
	INT    iReadEventfdLen = 0;
	UINT64 uiEvtData   = 0;

	iReadEventfdLen = read(iEvent, &uiEvtData, sizeof(uiEvtData));
	if (8 != iReadEventfdLen)
	{
	    ERR_PRINTF("eventfd read failed!");
	    return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

inline ULONG Dbg_thread_WriteDbgMsgList(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	DBGASSERT(NULL != pstDbgPrintBuf);

	pthread_mutex_lock(&DbgThrdMutex);
	DCL_AddTail(&g_DbgPrintListHead, &(pstDbgPrintBuf->stNode));
	pthread_mutex_unlock(&DbgThrdMutex);

	return ERROR_SUCCESS;
}

STATIC inline DBG_PRINT_BUFFER_S *dbg_thread_ReadDbgMsgList(VOID)
{
		DBG_PRINT_BUFFER_S *pstDbgPrintBuf = NULL;
		
		pstDbgPrintBuf = DCL_FIRST_ENTRY(&g_DbgPrintListHead, pstDbgPrintBuf, stNode);
		if (NULL == pstDbgPrintBuf)
		{
			return NULL;
		}
		
		pthread_mutex_lock(&DbgThrdMutex);
		DCL_Del(&(pstDbgPrintBuf->stNode));
		pthread_mutex_unlock(&DbgThrdMutex);

	return pstDbgPrintBuf;
}

ULONG Dbg_Thread_Printf(VOID)
{
	DBG_PRINT_BUFFER_S *pstDbgPrintBuf = NULL;

	/* 如果标识被置为 DBG_TPF_EXIT 则DBG线程不再接收打印信息，清空链表，准备推出 */
	if (DBG_TPF_EXIT == g_enDbgThreadPrintFlag)
	{
		DBGERR_PRINTF("**** [Get Dbg Exit Event] ****\n");
		enDbgThreadStatus = THREAD_STATUS_IDEL;
	}

	while(1)
	{
		pstDbgPrintBuf = dbg_thread_ReadDbgMsgList();

		if (NULL != pstDbgPrintBuf)
		{
            //TODO:宅这里可以使用ns_log控制器输出方向了
			DBG_print_ByType(pstDbgPrintBuf);
						
			Dbg_process_ByFriend(pstDbgPrintBuf);
			
			Debug_DestroyPrintData(pstDbgPrintBuf);
		}
		else
		{
			break;
		}
	}
	
	if (DBG_TPF_EXIT == g_enDbgThreadPrintFlag)
	{
		return ERROR_THREAD_EXIT;
	}

	return ERROR_SUCCESS;
}
INT dbg_Thread_SignalMask(VOID)
{
	INT		 iRet;
    sigset_t SigMask;
	
	/* 在信号集中打开所有的信号 */
	iRet = sigfillset(&SigMask);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal Fill Set Failed!");
		return -1;
	}

	/* 排除 SIGQUIT 信号，此信号要处理 
	iRet = sigdelset(&SigMask, SIGQUIT);
	if (iRet < 0)
	{
		ERR_PRINTF("Signal:SIGQUIT del Set Failed!");
		return -1;
	}
	*/

	/* 阻塞掉当前线程的所有()的信号捕获 */
	iRet = pthread_sigmask(SIG_BLOCK, &SigMask, NULL);
	if (-1 == iRet)
	{
		ERR_PRINTF("Signal mask Failed!");
		return -1;
	}

	/*
	struct sigaction stAct, stOldAct;

	sigaction函数的功能是检查或修改与指定信号相关联的处理动作（可同时两种操作）。
		他是POSIX的信号接口，而signal()是标准C的信号接口(如果程序必须在非POSIX系统上运行，那么就应该使用这个接口)
		给信号signum设置新的信号处理函数act， 同时保留该信号原有的信号处理函数oldact
	int sigaction(int signo,const struct sigaction *restrict act,
				  struct sigaction *restrict oact);
	结构sigaction定义如下：
		struct sigaction{
			void (*sa_handler)(int);
			sigset_t sa_mask;
			int sa_flag;
			void (*sa_sigaction)(int,siginfo_t *,void *);
		};
	sa_handler字段包含一个信号捕捉函数的地址
	sa_mask字段说明了一个信号集，在调用该信号捕捉函数之前，这一信号集要加进进程的信号屏蔽字中。仅当从信号捕捉函数返回时再将进程的信号屏蔽字复位为原先值。
	sa_flag是一个选项，主要理解两个
		SA_INTERRUPT 由此信号中断的系统调用不会自动重启
		SA_RESTART 由此信号中断的系统调用会自动重启
		SA_SIGINFO 提供附加信息，一个指向siginfo结构的指针以及一个指向进程上下文标识符的指针
	最后一个参数是一个替代的信号处理程序，当设置SA_SIGINFO时才会用他。


	注：
	(1)    如果在信号SIGINT(Ctrl + c)的信号处理函数show_handler执行过程中，本进程收到信号SIGQUIT(Crt+\)，将阻塞该信号，直到show_handler执行结束才会处理信号SIGQUIT。
	
	(2)    SA_NODEFER		一般情况下， 当信号处理函数运行时，内核将阻塞<该给定信号 -- SIGINT>。但是如果设置了SA_NODEFER标记， 那么在该信号处理函数运行时，内核将不会阻塞该信号。 SA_NODEFER是这个标记的正式的POSIX名字(还有一个名字SA_NOMASK，为了软件的可移植性，一般不用这个名字)	 
		   SA_RESETHAND    当调用信号处理函数时，将信号的处理函数重置为缺省值。 SA_RESETHAND是这个标记的正式的POSIX名字(还有一个名字SA_ONESHOT，为了软件的可移植性，一般不用这个名字)	
	
	(3)    如果不需要重置该给定信号的处理函数为缺省值；并且不需要阻塞该给定信号(无须设置sa_flags标志)，那么必须将sa_flags清零，否则运行将会产生段错误。但是sa_flags清零后可能会造成信号丢失！

	stAct.sa_handler = dbg_handle_ThreadQuit;
	//sigaddset(&stAct.sa_mask, SIGQUIT);		//见注(1)
	stAct.sa_flags = SA_NODEFER; //见注(2)
	stAct.sa_flags = 0; 						//见注(3)
	
	iRet = sigaction(SIGQUIT, &stAct, &stOldAct);
	if (iRet < 0)
	{
		ERR_PRINTF("Dbg Thread Sigacton Failed!");
		
		return -1;
	}
	*/

	return 0;
}

STATIC ULONG dbg_thread_QueMsgEpCallBack(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;

	DBGASSERT(NULL !=  arg);
		
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode  = dbg_thread_RecvEvent(pstThrdEpEvents->iEventFd);
		if (ERROR_SUCCESS != ulErrCode)
		{
			ERR_PRINTF("Recv EventFd Failed!\n");
		
			return ERROR_FAILE;
		}
		ulErrCode  = Dbg_Thread_Printf();
		if(ERROR_THREAD_EXIT == ulErrCode)
		{
			ERR_PRINTF("Thread[Debug] is going to Exit !\n");
		
			return ERROR_THREAD_EXIT;
		}
		if (ERROR_SUCCESS != ulErrCode)
		{
			ERR_PRINTF("DBG Thrad Printf Failed!\n");
		
			return ERROR_FAILE;
		}
	}

	if ((events & EPOLLHUP) && (pstThrdEpEvents->uiEvents & EPOLLHUP))
	{
		ERR_PRINTF("dbg_thread_QueMsgEpCallBack is EPOLLHUP!");
		ulErrCode = ERROR_FAILE;
	}
	
    return ulErrCode;
}

STATIC ULONG dbg_thread_InitCb(VOID * pArg)
{
	ULONG ulRet;
	INT iEventFd = -1;
	THREAD_INFO_S *pstThrdInfo;
	
	DBGASSERT(NULL != pArg);

	pstThrdInfo = (THREAD_INFO_S *)pArg;

	MSG_PRINTF("**** Dbg thread init ... ****");

#ifndef NS_EVENTFD
    /* Debug接收 */                
	iEventFd = eventfd(0, EFD_SEMAPHORE);
#else
    iEventFd = EVENT_Create();
#endif
	if (iEventFd < 0)
	{
        ERR_PRINTF("eventfd create failed!");
        return -1;
	}

#ifndef NS_EVENTFD
	g_iDbgThreadEventFd = iEventFd;
#else
	g_iDbgThreadEventFd = EVENT_ReadFD_2_WriteFd(iEventFd);
#endif
	
    /* 线程间消息 添加监听 */
	ulRet = THREAD_epoll_EventAdd(iEventFd,
         	                      EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLET,
         	                      pstThrdInfo->iThreadID,
         	                      dbg_thread_QueMsgEpCallBack,
         	                      NULL);
	
	iDbgThreadPriID = pstThrdInfo->iThreadID;
	
	Dbg_Thread_SetRun();
	
	return ulRet;
}

STATIC ULONG dbg_thread_FiniCb(VOID * pArg)
{
	Dbg_Thread_SetStop();

	return ERROR_SUCCESS;
}

STATIC VOID dbg_thread_QuemsgRecvCb(IN INT iThread,IN const THREAD_QUEMSG_DATA_S *pstThreadQuemsgData)
{
	/*
		不能使用同用的线程消息列队要不然或无限循环的呦 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	DBG_PRINT_BUFFER_S *pstDbgPrintBuf = NULL;

	pstDbgPrintBuf = (DBG_PRINT_BUFFER_S *)(pstThreadQuemsgData->pQueMsgData);
	
	DBG_print_ByType(pstDbgPrintBuf);
				
	Dbg_process_ByFriend(pstDbgPrintBuf);
	
	Debug_DestroyPrintData(pstDbgPrintBuf);
	*/
	
	return;
}

/*****************************************************************************
 Prototype    : DBG_Thread_Init
 Description  : 初始化一个线程用于 处理DGB的打印和。。
 				会引起无线递归循环奶奶的，
 Input        : VOID  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/14
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG DBG_Thread_Init(VOID)
{
	INT   iRet       = 0;
	ULONG ulRet      = 0;
	INT   iThreadId  = 0;
	UINT  iReTestCnt = 0;

	ulRet = Debug_process_Init();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Can NOT SHOW Debug message With Debug Process!");
	}

	dbg_print_ListInit();
	
	//iThreadId = Dbg_Thread_Create();
	iThreadId = Thread_server_CreateWithEpQMsg(dbg_thread_InitCb,
											   dbg_thread_FiniCb,
											   THREAD_TYPE_DEBUG,
											   dbg_thread_QuemsgRecvCb);
	if (iThreadId < 0)
	{
		ERR_PRINTF("Dbg Thread Create Failed!");
		return ERROR_FAILE;
	}
	DBG_THRD_NAME_REG(iThreadId, "DEBUG-THREAD");

	/* 等待线程启动以便于其他线程打印Bug */
	while(1)
	{
		if (BOOL_TRUE == DBG_thread_IsRun())
		{
			iRet = 0;
			break;
		}
		usleep(1000);
		iReTestCnt ++;
		if (iReTestCnt > 1000)
		{
			iRet = -1;
			break;
		}
	}

	if (0 != iRet)
 	{
		ERR_PRINTF("Debug thread start Failed!");
		return ERROR_FAILE;
	}
	
	MSG_PRINTF("**** Dbg thread RUN SUCCESS ! ****");

	return ERROR_SUCCESS;
}

VOID DBG_thread_Fini(VOID)
{
	dbg_print_ListFini();

	return;
}
