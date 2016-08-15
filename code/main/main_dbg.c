/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : main_dbg.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/9/25
  Last Modified :
  Description   : debug进程，为其他线程提供log打印和保存的服务
  Function List :
              init
              main
  History       :
  1.Date        : 2014/9/25
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include <ns_base.h>
#include <ns_daemon.h>
#include <ns_bitmap.h>
#include <ns_ab.h>
#include <ns_avl.h>
#include <ns_thread.h>
#include <ns_msgque.h>

#include <ns_websetting.h>
#include <ns_web_action.h>
#include <ns_web.h>
#include <ns_msg_signal.h>
#include <ns_process.h>

STATIC ULONG main_dbg_MainThrdExit(IN VOID *pArg)
{
	printf("................\n");

	return ERROR_SUCCESS;
}

VOID Dbg_recv_Thread_QueMsgRecv(IN INT iThreadId,
								IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsgData)
{
	

	return;
}

ULONG thread_queMsgRecvCallBack(IN UINT uiEvents, IN VOID * pArg/* THREAD_EPOLL_EVENTS_S* */)
{
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;

	DBGASSERT(NULL !=  pArg);
	
	MSG_PRINTF("thread_queMsgRecvCallBack!");
	
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)pArg;

	if ((uiEvents & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		MsgQue_Recv();
	}

	return ERROR_SUCCESS;
}

ULONG Dbg_recv_ThreadInit(VOID *pArg)
{
	INT   iRet;
	ULONG ulRet;
	INT   iSigMsgFd;  
	sigset_t SigMask;
	THREAD_INFO_S *pstThreadInfo;

	DBGASSERT(NULL != pArg);

	MSG_PRINTF_ONT("** Dbg_recv_ThreadInit **");
	
	iRet  = sigemptyset(&SigMask);
	iRet |= sigaddset(&SigMask, SIG_MSG);
	if (iRet < 0)
	{
		ERR_PRINTF("sig set is Failed!");
		
		return ERROR_FAILE;
	}

	/* SIGUSR1 进行了signalfd操作的同时，执行了sigaction操作，epoll_wait会 报告error的呦 !  */
	iSigMsgFd = MSG_sigthrd_SigToFd(&SigMask);
	if (iSigMsgFd < 0)
	{
		ERR_PRINTF("Sig To Fd is Failed!");
		
		return ERROR_FAILE;
	}

	pstThreadInfo = (THREAD_INFO_S *)pArg;

/*
	iMsgQueFd = MsgQue_GetMsgFd();

	if (-1 == iMsgQueFd)
	{
		ERR_PRINTF("Message Queue is Wrong!");
		
		return ERROR_FAILE;
	}

	难道msgrcv不支持他妈的EPOLL，监听了没效果呀，放弃，妈蛋 

	实践证明消息列队IPC只是 读写一个链表
*/

	ulRet = Thread_server_EpollAdd(pstThreadInfo->iThreadID,
						   		   iSigMsgFd,
						   		   EPOLLIN | EPOLLET,
						   		   thread_queMsgRecvCallBack);
	
	return ulRet;
}

ULONG Dbg_recv_ThreadFini(VOID *pArg)
{	

	//是否需要close SigFd呢

	printf("**** Dbg_recv_ThreadFini ****\n");

	return ERROR_SUCCESS;
}

ULONG Dbg_recv_CreateThread(VOID)
{
	INT iThreadID;
	iThreadID = Thread_server_CreateWithEpQMsg(Dbg_recv_ThreadInit,
											   Dbg_recv_ThreadFini,
											   THREAD_TYPE_DEBUG_RECV,
											   Dbg_recv_Thread_QueMsgRecv);
	if (iThreadID < 0)
	{
		ERR_PRINTF("Create Debug Receive Thread Failed!");
		return ERROR_FAILE;
	}
	
    DBG_THRD_NAME_REG(iThreadID, "DebugReceive");
	
	return ERROR_SUCCESS;
}


STATIC VOID dbg_OutPut_Thread_QueMsgRecv(IN INT iThreadId,
								         IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsgData)
{
	MSG_QUE_PRINT_S *pstMsgQueData;
	
	DBGASSERT(NULL != pstThrdQueMsgData);

	pstMsgQueData = (MSG_QUE_PRINT_S *)pstThrdQueMsgData->pQueMsgData;
	
	DBG_process_FriendDeTransForm(pstMsgQueData);
	DBG_print_ByType(&pstMsgQueData->stDbgPrintBuf);
	DBG_print_OnRemoteTerminal(&(pstMsgQueData->stDbgPrintBuf));

	return;
}

/* 专门用于把数据输出到终端或文件 */
STATIC ULONG dbg_OutPut_CreateThread(VOID)
{
	INT iThreadID;

	iThreadID = Thread_server_CreateWithEpQMsg(NULL, NULL, THREAD_TYPE_DEBUG_OUTPUT,
											   dbg_OutPut_Thread_QueMsgRecv);
	if (iThreadID < 0)
	{
		ERR_PRINTF("Create Debug OutPut Thread Failed!");
		return ERROR_FAILE;
	}

    DBG_THRD_NAME_REG(iThreadID, "DebugOutPut");
	return ERROR_SUCCESS;
}

STATIC INT main_dbg_init(PRUNTYPE_E enPRunType)
{
	THREAD_MAIN_PROC_S stThrdMainProc;

	AB_Init();
	BitMap_Init();
	THREAD_quemsg_Init();
	AVL_Sys_Init();
	MsgQue_Init();
	Thread_server_Init();

	Thread_main_init();

	DBG_print_SetRemoteTerminalName("./log/rt.html");///dev/pts/3
	DBG_print_OpenRTerminal();

	dbg_OutPut_CreateThread();
	Dbg_recv_CreateThread();

	memset(&stThrdMainProc, 0, sizeof(THREAD_MAIN_PROC_S));
	stThrdMainProc.pfThrdMainExit = main_dbg_MainThrdExit;
	
	Thread_main_Loop(&stThrdMainProc);

	DBG_print_CloseRTerminal();
	
	Thread_server_Fint();
	MsgQue_Fini();
	AVL_Sys_Fini();
	THREAD_quemsg_Fint();

	SHOW_PRINTF("Progress is Exit!");
	
	return 0;
}

typedef enum tag_dbg_proc_only_set_flg{
	DBG_PROCESS_ONLY_SET_NONE = 0,
	DBG_PROCESS_ONLY_SET_START,
	DBG_PROCESS_ONLY_SET_STOP,

	DBG_PROCESS_ONLY_SET_MAX,
}DBG_PROC_ONLY_SET_EN;
#define DBG_PROC_ONLY_CMD_DEL_FILE "rm -rfv "DBG_PROC_ONLY_FILE

/* we only need this procees one. */
static INT only_set(DBG_PROC_ONLY_SET_EN enDbgSet)
{
	INT	  iLen = 0, iRet = 0;
	FILE *fdOnly = NULL;
	CHAR  acBuf[DBG_PROC_ONLY_BUF_SIZE] = {0};
	pid_t myPid = 0;

	if (DBG_PROCESS_ONLY_SET_START == enDbgSet){
		iRet = access(DBG_PROC_ONLY_FILE, F_OK);
		if (0 == iRet){
			ERR_PRINTF("Another debug thread is start. You need stop it and delete %s .", DBG_PROC_ONLY_FILE);	
			return ERROR_FAILE;
		}
		fdOnly = fopen(DBG_PROC_ONLY_FILE, "w+");
		if (NULL == fdOnly){
			ERR_PRINTF("fopen %s failed.", DBG_PROC_ONLY_FILE);
			return ERROR_FAILE;
		}
		myPid = getpid();
		iLen  = snprintf(acBuf, sizeof(acBuf), "PID:%d\n", myPid);
		iRet  = fwrite(acBuf, 1, iLen, fdOnly);
		if (iRet != iLen){
			ERR_PRINTF("fwrite msg fialed.");
			fclose(fdOnly);
			system(DBG_PROC_ONLY_CMD_DEL_FILE);	
			return ERROR_FAILE;
		}
		fclose(fdOnly);
	} else if (DBG_PROCESS_ONLY_SET_STOP == enDbgSet){
		system(DBG_PROC_ONLY_CMD_DEL_FILE);	
	}

	return ERROR_SUCCESS;
}

INT main(INT argc, CHAR *argv[])
{
	INT iRet = 0;
	pid_t DbgProcessId = 0;
	PRUNTYPE_E enPRunType = PRUNTYPE_NORMAL;
	CHAR acPidBuf[DBG_ENV_PID_LEN + 1] = {0};

	DbgProcessId = getpid();

	sprintf(acPidBuf,"%d", DbgProcessId);

	iRet = setenv(DBG_ENV_PNAME, acPidBuf, 1);
	if (iRet < 0)
	{
		ERR_PRINTF("set env of debug pid failed");

		return 0;
	}
	MSG_PRINTF("DEBUG=%s" ,getenv(DBG_ENV_PNAME));
	
	MSG_PRINTF("Debug Process ID = %d", DbgProcessId);

	iRet = only_set(DBG_PROCESS_ONLY_SET_START);
	if (ERROR_SUCCESS != iRet){
		return 0;
	}

	if (argc > 1)
	{
		/* ./Debug daemon : 以守护进程的方式启动进程 */
		iRet = strcmp(argv[1], "daemon");
		if (0 == iRet)
		{
			iRet = NS_Daemon(0, 0);
			if (iRet < 0)
			{
				ERR_PRINTF("Init as deamon progress Failed!");
				return -1;
			}
			enPRunType = PRUNTYPE_DAEMON;
		}
	}

	main_dbg_init(enPRunType);
	
	/* 进程退出时，必须删掉 */
	only_set(DBG_PROCESS_ONLY_SET_STOP);

	return 0;
}




