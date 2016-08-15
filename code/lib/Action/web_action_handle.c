#include <stdio.h>
#include <stdlib.h>
#include <ns_base.h>

#include <ns_web_server.h>
#include <ns_thread.h>
#include <ns_web_action.h>
#include <ns_trigger.h>

/* 向Client Link线程发送消息 */
ULONG web_action_SendMsgToLinkThrd(IN THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    ULONG ulRet;
	INT iSrcThrdId;
	THREAD_INFO_S *pstThrd;
	THREAD_QUEMSG_DATA_S stThrdQueMsg;

	pstThrd = Thread_server_GetByThreadType(THREAD_TYPE_LINK);
	if (NULL != pstThrd)
	{
		ERR_PRINTF("Can not get Link Thread !");
		return ERROR_FAILE;
	}

	ulRet = THREAD_server_QueMsg_SendWithResp(pstThrd->iThreadID,
											  pstThrdQueMsg,
											  &iSrcThrdId,
											  &stThrdQueMsg);

	return ulRet;
}

ULONG WEB_action_Handle(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    ULONG ulRet = ERROR_SUCCESS;
//	THREAD_QUEMSG_DATA_S      stThrdQueMsg;
//	WEB_HTTP_REQMSGINFO_S    *pstWebHttpReqMsgInfo;
//	WEB_HTTP_REQSUBMITDATA_S *pstHttpReqSubmitData;
	
	DBGASSERT(iSrcThrdId > 0);
	DBGASSERT(NULL != pstThrdQueMsg);


	ulRet = Trigger_Handle_Msg(iSrcThrdId,pstThrdQueMsg);

/*
	if (pstThrdQueMsg->uiQueMsgType == THRD_QUEMSG_TYPE_WEB_EVENT_ASK)
	{
		pstWebHttpReqMsgInfo = (WEB_HTTP_REQMSGINFO_S *)(pstThrdQueMsg->pQueMsgData);
		pstHttpReqSubmitData = 	pstWebHttpReqMsgInfo->pstHttpReqSubmitData;

		UINT uiII = 0;
		for(uiII = 0; uiII < pstThrdQueMsg->uiQueMsgLen; uiII++)
		{
			MSG_PRINTF("[ACTION GET][%s] = [%s]", 
						pstHttpReqSubmitData[uiII].pcSMName,
						pstHttpReqSubmitData[uiII].pcSMValue);
		}

		ulRet = web_action_SendMsgToLinkThrd(pstThrdQueMsg);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Sent Msg to Link Thread Failed !");
		}


		mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
		stThrdQueMsg.uiQueMsgLen  = 4;
		stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_WEB_EVENT_RESP;
		stThrdQueMsg.pQueMsgData  = (VOID *)(ULONG)WEB_THRD_RESP_CODE_ACTION_SUCCESS;
		THREAD_server_QueMsg_Send(iSrcThrdId,&(stThrdQueMsg));
	}	
*/

	return ulRet;
}

