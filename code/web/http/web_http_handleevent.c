#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_base.h>
#include <ns_thread.h>
#include <ns_web_server.h>
#include <ns_web_action.h>

/*
	0xFFFF_FFFF;
	16-19位，操作类型
	 0-15位，操作事件

	0x0008_000x;
*/

typedef enum tag_EventType
{
	EVENT_TYPE_NONE = 0,
	
	EVENT_TYPE_SETTING,
	EVENT_TYPE_CONTROL,

	EVENT_TYPE_MAX,
}EVENT_TYPE_E;

/*
	0x0001_0003
	0x0001_0004
*/
#define WEB_EVENT_SETTING_ASK  (((EVENT_TYPE_SETTING) < 0x16) || (THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_ASK))
#define WEB_EVENT_SETTING_SAVE (((EVENT_TYPE_SETTING) < 0x16) || (THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_SAVE))

/*****************************************************************************
 Prototype    : web_http_AskToActionThrd
 Description  : 向Action线程发出请求，让Action处理，并接受Action线程的返回数据
 Input        : IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo  
                INOUT CHAR *pcRespBUF                           
                INOUT UINT *puiReapBufLen                       
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG web_http_AskToActionThrd(IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo,
							   INOUT CHAR *pcRespBUF, 
						   	   INOUT UINT *puiRespBufLen)
{
	ULONG		   ulRet        = 0;
	INT            iSrcThrdId   = 0;
	UINT           uiReqEevetID = 0;
	THREAD_INFO_S *pstThrd      = NULL;
	WEB_ACTION_REQ_S     stWebActionReq;
	WEB_ACTION_RESP_S	*pstWebActionResp;
	THREAD_QUEMSG_DATA_S stThrdQueMsg;
	THREAD_QUEMSG_DATA_S stThrdQueMsgResponse;

	uiReqEevetID = pstWebHttpReqMsgInfo->stHttpReqLine.uiReqEevetID;

   	//pstThrd->eThreadType != THREAD_TYPE_LINK;
	/* 现在只有一个Action 线程 */
	pstThrd = Thread_server_GetByThreadType(THREAD_TYPE_WEB_ACTION);
		   
	DBGASSERT(NULL != pstThrd);

	if (-1 == pstThrd->stThrdQueMsgInfo.iThrdQueMsgEventFd)
	{
	   ERR_PRINTF("iThrdQueMsgEventFd = -1!");
	   return ERROR_SUCCESS;
	}

	MSG_PRINTF("Thread Web server to Thread (%d)", pstThrd->iThreadID);
			  
	MSG_PRINTF("event fd = %d", pstThrd->stThrdQueMsgInfo.iThrdQueMsgEventFd);

	/* 填充action event request 数据 */
	mem_set0(&stWebActionReq, sizeof(WEB_ACTION_REQ_S));
	stWebActionReq.pHttpReqMsgInfo = (VOID *)pstWebHttpReqMsgInfo;
	stWebActionReq.pReqBuf         =  pcRespBUF; /* 让Action线程去填充数据 */
	stWebActionReq.uiReqBufLen     = *puiRespBufLen;
	stWebActionReq.uiWebEventType  =  uiReqEevetID;

	SHOW_PRINTF("uiQuemsgType = %d", uiReqEevetID);

	/* 填充消息数据 */
	mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
	stThrdQueMsg.uiQueMsgDataLen  = sizeof(WEB_ACTION_REQ_S);
	stThrdQueMsg.uiQueMsgType     = THRD_QUEMSG_TYPE_WEB_EVENT_ASK;
	stThrdQueMsg.pQueMsgData      = (VOID *)(&(stWebActionReq));

	/* 初始化接收消息的结构体 */
	mem_set0(&stThrdQueMsgResponse, sizeof(THREAD_QUEMSG_DATA_S));
	///* 当前线程不支持EPOLL事件所以要等待Action线程的回复 */
	ulRet = THREAD_server_QueMsg_SendWithResp(pstThrd->iThreadID,&(stThrdQueMsg),
								      		  &iSrcThrdId,
								      		  &stThrdQueMsgResponse);

	if (ulRet != ERROR_SUCCESS)
	{
		ERR_PRINTF("WEB THREAD_server_QueMsg_SendWithResp FAILED!");
		return ERROR_FAILE;
	}

	pstWebActionResp = (WEB_ACTION_RESP_S *)stThrdQueMsgResponse.pQueMsgData;

	if (NULL == pstWebActionResp)
	{
		WARN_PRINTF("No Response Data!");
		*puiRespBufLen = 0;
	}
	else
	{
		*puiRespBufLen = pstWebActionResp->uiRespBufLen;
		free(pstWebActionResp);
	}
	
	if ((ULONG)stThrdQueMsgResponse.uiQueMsgType == THRD_QUEMSG_TYPE_WEB_EVENT_RESP)
	{
		return ERROR_SUCCESS;
	}
	else
	{
		return ERROR_FAILE;
	}
	   
}

/*****************************************************************************
 Prototype    : WEB_http_HandleEvent
 Description  : 处理Http发来的 Event 请求
 Input        : IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo  
                INOUT CHAR *pcRespBUF                           
                INOUT UINT *puiReapBufLen                       
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG WEB_http_HandleEvent(IN WEB_HTTP_REQMSGINFO_S *pstWebHttpReqMsgInfo, 
						   INOUT CHAR *pcRespBUF, 
						   INOUT UINT *puiRespBufLen)
{
	ULONG ulRet;

	DBGASSERT(NULL != puiRespBufLen);
	DBGASSERT(NULL != pcRespBUF);
	DBGASSERT(NULL != pstWebHttpReqMsgInfo);

	/* 向Action线程发出请求，让Action处理 */
	ulRet = web_http_AskToActionThrd(pstWebHttpReqMsgInfo, 
									 pcRespBUF,
									 puiRespBufLen);
	if (ERROR_SUCCESS == ulRet)
	{
		MSG_PRINTF("Action Resp Buffer Length = [%d]", *puiRespBufLen);
		/*
		*puiRespBufLen = sprintf(pcRespBUF , "%s,This is a event ask by event id %d",
						 pcRespBUF,uiReqEevetID);
		*/
	}
	else
	{
		*puiRespBufLen = sprintf(pcRespBUF , "This is a ERROR-event ask by event id %d",
						 pstWebHttpReqMsgInfo->stHttpReqLine.uiReqEevetID);
	}


	return ERROR_SUCCESS;
}

