#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <ns_base.h>
#include <ns_msgque.h>
#include <ns_process.h>
#include <ns_msg_signal.h>

STATIC INT g_iDbgPID = -1;

ULONG DBG_process_FriendTransForm(IN MSG_QUE_PRINT_S *pstMsgQuePrint,IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	INT iBufP 	 = 0;

	DBGASSERT(NULL != pstMsgQuePrint);
	DBGASSERT(NULL != pstDbgPrintBuf);

	pstMsgQuePrint->stDbgPrintBuf.ulDbgSeq  = pstDbgPrintBuf->ulDbgSeq;
	pstMsgQuePrint->stDbgPrintBuf.enDbgType = pstDbgPrintBuf->enDbgType;
	pstMsgQuePrint->stDbgPrintBuf.iDbgLine  = pstDbgPrintBuf->iDbgLine;

	/* sprintf 返回的长度不包含 '\0' 说以要手动加上这个长度 */
	
	pstMsgQuePrint->stDbgPrintBuf.pcDbgFileName= (CHAR *)0;
	iBufP = sprintf(pstMsgQuePrint->ucMSgQueBuf, pstDbgPrintBuf->pcDbgFileName);
	iBufP += 1;
	//if (iBufP >= MSG_QUE_PRINT_BUF_LEN)
	//{
	//	goto exit_DBG_print_FriendTransForm;
	//}
	
	pstMsgQuePrint->stDbgPrintBuf.pcDbgFunName = (CHAR *)(LONG)iBufP;
	iBufP += sprintf(pstMsgQuePrint->ucMSgQueBuf + iBufP, pstDbgPrintBuf->pcDbgFunName);
	iBufP += 1;
	//if (iBufP >= MSG_QUE_PRINT_BUF_LEN)
	//{
	//	goto exit_DBG_print_FriendTransForm;
	//}
	
	pstMsgQuePrint->stDbgPrintBuf.pcString = (CHAR *)(LONG)iBufP;
	iBufP += sprintf(pstMsgQuePrint->ucMSgQueBuf + iBufP, pstDbgPrintBuf->pcString);
	
	return ERROR_SUCCESS;

//exit_DBG_print_FriendTransForm:
	DBGERR_PRINTF("Deubg message is too long!");
	
	return ERROR_FAILE;
}

VOID DBG_process_FriendDeTransForm(IN MSG_QUE_PRINT_S *pstMsgQuePrint)
{
	pstMsgQuePrint->stDbgPrintBuf.pcDbgFileName = pstMsgQuePrint->ucMSgQueBuf;
	pstMsgQuePrint->stDbgPrintBuf.pcDbgFunName  = (CHAR *)((ULONG)pstMsgQuePrint->ucMSgQueBuf 
											    + (ULONG)(pstMsgQuePrint->stDbgPrintBuf.pcDbgFunName));
	pstMsgQuePrint->stDbgPrintBuf.pcString      = (CHAR *)((ULONG)pstMsgQuePrint->ucMSgQueBuf 
											    + (ULONG)(pstMsgQuePrint->stDbgPrintBuf.pcString));

	return;
}

STATIC ULONG dbg_print_SendSigMSG(VOID)
{
	INT iRet;
	
	iRet = kill(g_iDbgPID, SIG_MSG);
	if (iRet < 0)
	{
		DBGERR_PRINTF("kill SIG_MSG to Debug Process Failed!");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

VOID Dbg_process_ByFriend(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	UINT  uiReTryTime = 0;
	ULONG ulRet       = ERROR_SUCCESS;
	//useconds_t uctSleepInter = 1;
	unsigned long uctSleepInter = 1;
	MSG_QUE_PRINT_S stMsgQuePrint;
    STATIC UCHAR ucDbgThrdRun = 1;
	
	if (g_iDbgPID < 0)
	{
        if (ucDbgThrdRun == 1)
        {
            DBGERR_PRINTF("Debug ID is invalid,Maybe Debug process is no Ready!");
        }
        ucDbgThrdRun = 0;
		return;
	}
    else
    {
        DBGERR_PRINTF("Maybe Debug process is no ok!");
        ucDbgThrdRun = 1;
    }

	memset(&stMsgQuePrint, 0, sizeof(MSG_QUE_PRINT_S));
	ulRet = DBG_process_FriendTransForm(&stMsgQuePrint, pstDbgPrintBuf);
	if (ERROR_SUCCESS != ulRet)
	{
		return;
	}
	
	stMsgQuePrint.lMsgQueType = 1;

	while(1)
	{
		ulRet = dbg_print_SendSigMSG();
		if (ERROR_SUCCESS != ulRet)
		{
			break;
		}
	
		ulRet = MsgQue_Send(&stMsgQuePrint);
		if (ERROR_QUEMSG_TRY_AGAIN == ulRet)
		{
			if (uiReTryTime > 100)
			{
				DBGERR_PRINTF("Retry too much time, give up!");
				break;
			}
			usleep(uctSleepInter);
			uiReTryTime++;
			uctSleepInter += uiReTryTime;
			
			continue;
		}
		else
		{
			/* 发送成功直接跳出 */
			break;
		}
	}
	
	return;
}

ULONG Debug_process_Init(VOID)
{
	INT  iDebugPID  = 0;
	
	iDebugPID = P_GetPID("debug");
	if (iDebugPID < 0)
	{
		ERR_PRINTF("Can not Find Debug Process!");
		return ERROR_FAILE;
	}

	g_iDbgPID = iDebugPID;

	return ERROR_SUCCESS;
}

