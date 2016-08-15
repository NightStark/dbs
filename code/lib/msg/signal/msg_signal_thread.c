/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : msg_signal_thread.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/10/6
  Last Modified :
  Description   : 线程 信号操作相关
  Function List :
  History       :
  1.Date        : 2014/10/6
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <ns_base.h>
#include <ns_msg_signal.h>

/*****************************************************************************
 Prototype    : MSG_sigthrd_GetCurrentThreadSigMask
 Description  : 获取当前线程的signal mask
 Input        : INOUT SIGSET_T *pSigSet 接收获取到的 signal mask
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/10/6
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG MSG_sigthrd_GetCurrentThreadSigMask(INOUT sigset_t *pSigSet)
{
	INT iRet = 0;
	
	DBGASSERT(NULL != pSigSet);

	/* 
		int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset);
		除SIG_SETMASK外,如果set是个空指针，则不改变该进程的信号屏蔽字，how的值也无意义。
		SIG_SETMASK与set空指针结合使用，即清空所有屏蔽的信号。
	*/
	
	iRet = sigprocmask(SIG_BLOCK, NULL, pSigSet);
	if (iRet < 0)
	{
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : MSG_sigthrd_SigToFd
 Description  : 将signal number s 转成 signalfd
 Input        : IN sigset_t *pSigMask   要转换的信号集合
 Output       : None
 Return Value : INT : -1失败
 					  其他，成功转换的signalfd
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/10/6
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
INT MSG_sigthrd_SigToFd(IN sigset_t *pSigMask)
{
	INT   iSigNumi;
	INT   iRet;
	INT   iSigFd;  
	ULONG ulRet;
	sigset_t DelMask;  
	sigset_t FdMask;  

	/* 获取当前线程的signal mask */
	ulRet = MSG_sigthrd_GetCurrentThreadSigMask(&DelMask);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Get Current Thread SigMask Failed!");
		return -1;
	}
	
	iRet = sigemptyset(&FdMask);
    if (iRet < 0)
	{
		ERR_PRINTF("sigemptyset Failed!");
		return -1;
	}

	/* 摘除 pSigMask */
	for(iSigNumi = 0; iSigNumi < 256; iSigNumi++)
	{
		if (1 != sigismember(pSigMask, iSigNumi))
		{
			continue;
		}
		
		iRet = sigdelset(&DelMask,SIG_MSG);
		if (iRet < 0)
		{
			ERR_PRINTF("sigdelset SIG_MSG Failed!");
			return -1;
		}
		
		iRet = sigaddset(&FdMask,SIG_MSG);
		if (iRet < 0)
		{
			ERR_PRINTF("sigaddset SIG_MSG Failed!");
			return -1;
		}
	}

	/* 设置更新 signal mask */
    iRet = pthread_sigmask(SIG_SETMASK, &DelMask, NULL);
    if (iRet < 0)
	{
		ERR_PRINTF("pthread_sigmask SIG_MSG Failed!");
		return -1;
	}

	/* SIG_MSG 转为 fd */
	iSigFd = signalfd(-1, &FdMask, 0);  
    if (iSigFd < 0)
	{
		ERR_PRINTF("SIG_MSG signalfd Failed!");
		return -1;
	}
	
	return iSigFd;
}


