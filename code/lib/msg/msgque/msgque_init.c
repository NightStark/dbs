/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : msgque_init.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/9/26
  Last Modified :
  Description   : 进程间消息队列初始化
  Function List :
  History       :
  1.Date        : 2014/9/26
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h> 
#include <errno.h> 

#include <ns_base.h>
#include <ns_msgque.h>
#include <ns_thread.h>

INT iQusMsdid = -1; 

INT MsgQue_GetMsgFd(VOID)
{
	return iQusMsdid;
}

ULONG MsgQue_Send(MSG_QUE_PRINT_S *pstMsgQueData)
{ 
    INT iRet = 0;

	if (-1 == iQusMsdid)
	{
        DBGERR_PRINTF("iQusMsdid is invalid!"); 
        return ERROR_FAILE;
	}

	/* 向队列发送数据 */ 
	/*
		如果没有读取端，读取消息列队，有可能在消息列队慢了是，阻塞主，
		接收到signal可以退出阻塞，
		但是该线程mask了所有的信号，因此将会一直阻塞，
		因此在退出时主线程无法接收到该线程退出的通知。
		
		如为IPC_NOWAIT时表示空间不足时不会阻塞
	*/
    iRet = msgsnd(iQusMsdid, pstMsgQueData, MSG_QUE_MSG_LEN_MAX, IPC_NOWAIT);
    if(iRet < 0)  
    {  
		if (EAGAIN == errno)
		{
			DBGERR_PRINTF("Try Again!"); 
			
			return ERROR_QUEMSG_TRY_AGAIN;
		}
    
        DBGERR_PRINTF("msgsnd failed;ERROR : %d", errno); 
        return ERROR_FAILE;
    }  

	return ERROR_SUCCESS;
}

/* 发送给 Out Put 线程 */
ULONG MsgQue_SendToOutPutThread(IN const MSG_QUE_PRINT_S *pstMsgQueData)
{
	ULONG ulRet;
	THREAD_INFO_S 		 *pstThrd;
	MSG_QUE_PRINT_S 	 *pstMsgQueBuf;
	THREAD_QUEMSG_DATA_S  stThrdQueMsg;

	DBGASSERT(NULL != pstMsgQueData);

	pstThrd = Thread_server_GetByThreadType(THREAD_TYPE_DEBUG_OUTPUT);
	if (NULL == pstThrd)
	{
		DBGERR_PRINTF("Out-Put Thread is not Ready!");
		return ERROR_FAILE;
	}

	memset(&stThrdQueMsg, 0, sizeof(THREAD_QUEMSG_DATA_S));

	/* 申请真正的数据空间 */
	pstMsgQueBuf = malloc(sizeof(MSG_QUE_PRINT_S));
	if (NULL == pstMsgQueBuf)
	{
		DBGERR_PRINTF("Malloc fo memory Failed!");
		return ERROR_NOT_ENOUGH_MEM;
	}
	memset(pstMsgQueBuf, 0, sizeof(MSG_QUE_PRINT_S));

	memcpy(pstMsgQueBuf, pstMsgQueData, sizeof(MSG_QUE_PRINT_S));

	stThrdQueMsg.pQueMsgData = (VOID *)pstMsgQueBuf;
	stThrdQueMsg.uiQueMsgDataLen = sizeof(MSG_QUE_PRINT_S);

	ulRet = THREAD_server_QueMsg_Send(pstThrd->iThreadID, &stThrdQueMsg);

	return ulRet;
}

ULONG MsgQue_Recv(VOID)
{
    INT iRet = 0;
    ULONG ulRet = ERROR_SUCCESS;
    CHAR ucMsgQueBuf[MSG_QUE_MSG_LEN_MAX];
    MSG_QUE_PRINT_S stMsgQueData;
    
    /* 从队列中获取消息，直到遇到end消息为止 */ 
    while(1)  
    {  
    	memset(ucMsgQueBuf, 0, sizeof(MSG_QUE_MSG_LEN_MAX));
    	memset(&stMsgQueData, 0, sizeof(MSG_QUE_PRINT_S));
    	/*
			IPC_NOWAIT
					  Return immediately if no message of the requested type is in the
					  queue.  The system call fails with errno set to ENOMSG.
    	*/
    	iRet = msgrcv(iQusMsdid, (VOID*)&stMsgQueData, MSG_QUE_MSG_LEN_MAX, 1, IPC_NOWAIT);
        if (-1 == iRet)  
        {  
        	if (ENOMSG == errno)
			{
				DBGERR_PRINTF("No Message to do with\n");  
				return ERROR_PROCESS_NOMSG;
			}
        	
        	if (E2BIG == errno)
        	{
				DBGERR_PRINTF("E2BIG\n");  
				continue;
        	}
        	
            DBGERR_PRINTF("msgrcv failed with errno: %d\n", errno);  
			return ERROR_FAILE;
        }  
        
		if (0 == iRet)
		{
			break;
		}

		ulRet = MsgQue_SendToOutPutThread(&stMsgQueData);
        //DBG_process_FriendDeTransForm(&stMsgQueData);
        //DBG_print_ByType(&stMsgQueData.stDbgPrintBuf);
        //DBG_print_OnRemoteTerminal(&stMsgQueData.stDbgPrintBuf);
        
	}  

	return ulRet;
}

ULONG MsgQue_Init(VOID)
{ 
    /* 
    	建立消息队列 
		这里使用了固定的key值，
		也可以使用ftok()函数，
    */  
    iQusMsdid = msgget((key_t)MSG_QUE_KEY, 0666 | IPC_CREAT);
    if (-1 == iQusMsdid)
    {
    	ERR_PRINTF("Create msgget Failed!");
		return ERROR_FAILE;
    }

	return ERROR_SUCCESS;
}

ULONG MsgQue_Fini(VOID)
{
    INT iRet = 0;
    
	/* 删除消息队列 */ 
	iRet = msgctl(iQusMsdid, IPC_RMID, 0);
    if(-1 == iRet)  
    {  
        ERR_PRINTF("msgctl(IPC_RMID) failed");  
		return ERROR_FAILE;
    } 

    return ERROR_SUCCESS;
}

