#include <stdio.h>
#include <stdlib.h>
#include <ns_base.h>

#include <ns_web_server.h>
#include <ns_thread.h>
#include <ns_web_action.h>

STATIC ULONG web_action_ThreadInit(VOID *arg)
{
	THREAD_INFO_S *pstThrd;
	
	DBGASSERT(NULL != arg);

	pstThrd = (THREAD_INFO_S *)arg;

	MSG_PRINTF("Web Action Thread(%d) Initialize ...", pstThrd->iThreadID);
	
	return ERROR_SUCCESS;
}

STATIC VOID web_action_QueMsgRecv(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	ULONG ulRet;

	DBGASSERT(NULL != pstThrdQueMsg);

	ulRet = WEB_action_Handle(iSrcThrdId, pstThrdQueMsg);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Web Action Handle Failed!");
	}
	
    return;
}

ULONG WEB_action_Init(VOID)
{
	INT iThreadID;

	iThreadID = Thread_server_CreateWithEpQMsg(web_action_ThreadInit,
											   NULL,
											   THREAD_TYPE_WEB_ACTION,
											   web_action_QueMsgRecv);

	if (0 > iThreadID)
	{
		ERR_PRINTF("Thread Action Create Failed!");
		return ERROR_SUCCESS;
	}
	DBG_THRD_NAME_REG(iThreadID, "Web-Action");

	return ERROR_SUCCESS;
}
