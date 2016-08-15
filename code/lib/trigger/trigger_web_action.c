#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_base.h>
#include <ns_web_server.h>
#include <ns_thread.h>
#include <ns_web_action.h>

#include <ns_trigger.h>
#include <ns_websetting.h>

ULONG quemsg_trigeer_WebAction_Ask(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    ULONG ulRet = ERROR_SUCCESS;
	THREAD_QUEMSG_DATA_S      stThrdQueMsg;
	WEB_HTTP_REQMSGINFO_S    *pstWebHttpReqMsgInfo;
	WEB_HTTP_REQSUBMITDATA_S *pstHttpReqSubmitData;
	
	pstWebHttpReqMsgInfo = (WEB_HTTP_REQMSGINFO_S *)(pstThrdQueMsg->pQueMsgData);
	pstHttpReqSubmitData =	pstWebHttpReqMsgInfo->pstHttpReqSubmitData;
	
	UINT uiII = 0;
	for(uiII = 0; uiII < pstThrdQueMsg->uiQueMsgDataLen; uiII++)
	{
		MSG_PRINTF("[ACTION GET][%s] = [%s]", 
					pstHttpReqSubmitData[uiII].pcSMName,
					pstHttpReqSubmitData[uiII].pcSMValue);
	}

	/*
	ulRet = web_action_SendMsgToLinkThrd(pstThrdQueMsg);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Sent Msg to Link Thread Failed !");
	}
	*/
	
	
	mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
	stThrdQueMsg.uiQueMsgDataLen  = 4;
	stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_WEB_EVENT_RESP;
	stThrdQueMsg.pQueMsgData  = (VOID *)(ULONG)WEB_THRD_RESP_CODE_ACTION_SUCCESS;
	THREAD_server_QueMsg_Send(iSrcThrdId,&(stThrdQueMsg));

	return ulRet;
}

ULONG quemsg_trigeer_WebAction_SetAsk(IN VOID *pReqBuf,
								      IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo, 
								      IN WEB_ACTION_RESP_S *pstWebActionResp)
{
    ULONG ulRet = ERROR_SUCCESS;
    ULONG uiRet = 0;
	
	uiRet = Create_setting_ForHttp(pReqBuf);
	
	pstWebActionResp->uiWebEventRespType = THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_RESP;
	pstWebActionResp->uiRespBufLen = uiRet;
	pstWebActionResp->pRespBuf     = pReqBuf;
	
	return ulRet;
}

ULONG quemsg_trigeer_WebAction_ControlAsk_ConnSever(IN VOID *pReqBuf,
													IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo, 
													IN WEB_ACTION_RESP_S *pstWebActionResp)
{
	UINT  uiRet = 0;
    ULONG ulRet = ERROR_SUCCESS;

	
	uiRet = Create_control_ForHttp(pReqBuf, 1);

	pstWebActionResp->uiWebEventRespType = THRD_QUEMSG_TYPE_WEB_EVENT_CONTROL_RESP;
	pstWebActionResp->uiRespBufLen = uiRet;
	pstWebActionResp->pRespBuf     = pReqBuf;
	
	return ulRet;
}
ULONG quemsg_trigeer_WebAction_ControlAsk_DisConnSever(IN VOID *pReqBuf,
	 												   IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo, 
	 												   IN WEB_ACTION_RESP_S *pstWebActionResp)
{
	UINT  uiRet = 0;
    ULONG ulRet = ERROR_SUCCESS;
	
	uiRet = Create_control_ForHttp(pReqBuf, 0);			

	pstWebActionResp->uiWebEventRespType = THRD_QUEMSG_TYPE_WEB_EVENT_CONTROL_RESP;
	pstWebActionResp->uiRespBufLen = uiRet;
	pstWebActionResp->pRespBuf     = pReqBuf;
	
	return ulRet;
}


ULONG quemsg_trigeer_WebAction_SaveSet(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    ULONG ulRet = ERROR_SUCCESS;

	return ulRet;
}



QUEMSG_TRIGGER_FUN g_TriggerWebActionList[THRD_QUEMSG_TYPE_WEB_EVENT_MAX] = 
{
	//[THRD_QUEMSG_TYPE_WEB_EVENT_ASK]		  = NULL,//quemsg_trigeer_WebAction_Ask,
	
	//[WET_SET_ASK]  = quemsg_trigeer_WebAction_SetAsk,
	//[WET_SET_SAVE] = quemsg_trigeer_WebAction_SaveSet,

	[WET_CTRL_CONN_SER]    = quemsg_trigeer_WebAction_ControlAsk_ConnSever,
	[WET_CTRL_DISCONN_SER] = quemsg_trigeer_WebAction_ControlAsk_DisConnSever,
};

ULONG triiger_proc_Event(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
    ULONG ulRet = ERROR_SUCCESS;
	WEB_ACTION_REQ_S		 *pstWebActionReq;
	WEB_ACTION_RESP_S		 *pstWebActionResp;
	THREAD_QUEMSG_DATA_S      stThrdQueMsg;
	WEB_HTTP_REQMSGINFO_S    *pstWebHttpReqMsgInfo;
	WEB_EVENT_TYPE_E          enThrdQueMsgWebEvent;

	pstWebActionResp = mem_alloc(sizeof(WEB_ACTION_RESP_S));
	if(NULL == pstWebActionResp)
	{
		return ERROR_FAILE;
	}
	
	pstWebActionReq      = (WEB_ACTION_REQ_S *)pstThrdQueMsg->pQueMsgData;
	pstWebHttpReqMsgInfo = (WEB_HTTP_REQMSGINFO_S *)(pstWebActionReq->pHttpReqMsgInfo);
	enThrdQueMsgWebEvent = pstWebActionReq->uiWebEventType & WEB_EVENT_TYPE_MASK;

	/****/
	if (enThrdQueMsgWebEvent < WET_SET_ASK ||
	    enThrdQueMsgWebEvent > WET_CTRL_MAX)
	{
		ERR_PRINTF("Invalid Web Action Queue msg Event[%d]!", enThrdQueMsgWebEvent);
		return ERROR_FAILE;
	}

	MSG_PRINTF("TRIGGER Handle msg from Thread(%d) of event(%d)", iSrcThrdId, enThrdQueMsgWebEvent);

	if (NULL != g_TriggerWebActionList[enThrdQueMsgWebEvent])
	{
		ulRet = g_TriggerWebActionList[enThrdQueMsgWebEvent](pstWebActionReq->pReqBuf, pstWebHttpReqMsgInfo, pstWebActionResp);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("g_TriggerWebActionList Error !");
		}
	}

	/****/
	
	mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
	stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_WEB_EVENT_RESP;
	stThrdQueMsg.uiQueMsgDataLen  = sizeof(WEB_ACTION_RESP_S);
	stThrdQueMsg.pQueMsgData  = (VOID *)(pstWebActionResp);
	
	ulRet = THREAD_server_QueMsg_Send(iSrcThrdId,&(stThrdQueMsg));

	return ulRet;
}

ULONG Trigger_Handle_Msg(IN INT iSrcThrdId, 
 						 IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	ULONG ulReg = ERROR_SUCCESS;

	DBGASSERT(NULL != pstThrdQueMsg);

	ulReg = triiger_proc_Event(iSrcThrdId, pstThrdQueMsg);

	return ulReg;
}


