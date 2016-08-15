#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_client.h>
#include <ns_client_fsm.h>
#include <ns_thread_quemsg.h>
#include <ns_wait.h>

VOID *client_AskSrvReapCode_Alloc(VOID)
{	
	VOID *pData = NULL;
	pData = mem_alloc(sizeof(UINT));
	return pData;
}

ULONG client_AskSrvReapCode_DataIn(VOID *pData, VOID *pCode)
{
	memcpy(pData, pCode, sizeof(UINT));

	return ERROR_SUCCESS;
}


ULONG client_SendAskSrvReapCode(VOID *pData)
{
	/*
	THREAD_QUEMSG_DATA_S stQuemsgData = {};

	stQuemsgData.pQueMsgData = pData;
	stQuemsgData.uiQueMsgDataLen = sizeof(MSG_SERVER_RESP_CODE_S);
	stQuemsgData.uiQueMsgType= THREAD_QUEMSG_TYPE_ASK_SRV_RESP_CODE;
	*/
	//return THREAD_quemsg_Write(&stQuemsgData, CLIENT_THRDQUE_MSG_TRSCHL_ASKSRV_RESP);
	return ERROR_SUCCESS;
	
}

ULONG Client_fsm_msg_proc_AskServiceResp(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	ULONG ulRet = 0;
	MSG_SERVER_RESP_CODE_S stSrvRespCode = {};
	
	MSG_de_DecData(pstMsgRecvInfo->pRecvDataBuf,
							   &(pstMsgRecvInfo->uiDataType),
							    &stSrvRespCode);

	if (SERVER_ASK_IS_ACCEPT == stSrvRespCode.uiRespCode)
	{
		ulRet = THREAD_wait_wakeup((VOID *)&stSrvRespCode, client_SendAskSrvReapCode);
	}

	return ulRet;
}

ULONG Client_fsm_msg_proc_ClientCreateTableResp(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	CLIENT_NSDB_CREATE_TABLE_RESP stCreateTableRespCode = {};
	
	MSG_de_DecData(pstMsgRecvInfo->pRecvDataBuf,
							   &(pstMsgRecvInfo->uiDataType),
							    &stCreateTableRespCode);

	if (SERVER_CLIENT_NSdB_CREATE_TABLE_SUCCESS == stCreateTableRespCode.uiRespCode)
	{
		MSG_PRINTF("Table Create Success!");
	}

	return ERROR_SUCCESS;
}


