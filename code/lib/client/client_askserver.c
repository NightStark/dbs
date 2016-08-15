#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_client.h>
#include <ns_thread_quemsg.h>
#include <ns_wait.h>

#define LOCAL_IF_NAME "p3p1"

ULONG client_ReadAskSrvRespMsg(VOID *pData)
{
	ULONG ulRet = ERROR_SUCCESS;

	//ulRet = THREAD_quemsg_Read(pData, CLIENT_THRDQUE_MSG_TRSCHL_ASKSRV_RESP);
	if(ERROR_SUCCESS != ulRet)
	{
		MSG_PRINTF("Keep Waiting ...");
	}
	return ulRet;
}

ULONG client_WaitAskSrvReap(VOID *pData)
{
	ULONG ulRet;	
	
	ulRet = THREAD_wait_CondInit();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait cond init failed!");
		return ulRet;
	}
	
	
	ulRet = THREAD_wait(pData, client_ReadAskSrvRespMsg);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait create failed!");
		return ulRet;
	}
	
	ulRet = THREAD_wait_CondFint();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait destroy failed!");
		return ulRet;
	}

	return ERROR_SUCCESS;
}

ULONG CLIENT_AskServer(IN INT iClientfd)
{
	ULONG ulRet = 0;	
	UINT  uiMsgLen = 0;
	CHAR  aucDataBuf[NSDB_MSG_LEN_MAX];
	MSG_CLIENT_INFO_S stClientInfo;
	MSG_SERVER_RESP_CODE_S stSrvRespCode = {};
	THREAD_QUEMSG_DATA_S stQuemsgData = {};

	mem_set0(aucDataBuf, NSDB_MSG_LEN_MAX);
	NET_GetHosName(stClientInfo.szClientHostName,
				   sizeof(stClientInfo.szClientHostName));
	NET_GetLocalIPByIFName("p3p1", &stClientInfo.stCNetIFData);
	
	uiMsgLen = MSG_ec_EncMsg( MSG_TYPE_NSDB_ASK_SERVICE_REQ,
							  MSG_DATA_TYPE_CLIENT_INFO,
							  &stClientInfo,
							  sizeof(stClientInfo),
							  aucDataBuf);

	/* 向服务器发送服务请求消息 */
	ulRet = CLIRNT_msg_send(iClientfd,aucDataBuf,uiMsgLen);
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("sent error!");
		return ulRet;
	}
	
	stQuemsgData.pQueMsgData  = &stSrvRespCode;
	stQuemsgData.uiQueMsgDataLen  = sizeof(MSG_SERVER_RESP_CODE_S);
	stQuemsgData.uiQueMsgType = THREAD_QUEMSG_TYPE_ASK_SRV_RESP_CODE;

	ulRet = client_WaitAskSrvReap(&stQuemsgData);
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("WaitAskSrvReap error");
		return ulRet;
	}

	if (stSrvRespCode.uiRespCode == SERVER_ASK_IS_ACCEPT)
	{
		MSG_PRINTF("SERVER_ASK_IS_ACCEPT");
	}

	return ERROR_SUCCESS;
}


