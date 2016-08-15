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
#include <ns_fsm.h>

ULONG FSM_msg_proc_AskServiceReq(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiBufPoint = 0;
	MSG_CLIENT_INFO_S stMsgClientInfo;

	memset(&stMsgClientInfo, 0, sizeof(MSG_CLIENT_INFO_S));

	uiBufPoint += MSG_de_DecData(pstMsgRecvInfo->pRecvDataBuf,
								 &(pstMsgRecvInfo->uiDataType),
								 &stMsgClientInfo);
								 
	ulRet = Server_ProcServerAskReq(pstMsgRecvInfo, &stMsgClientInfo);

	return ulRet;
}

ULONG FSM_msg_proc_ClientCreateTable(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	ULONG ulRet = ERROR_SUCCESS; 
	UINT uiBufPoint = 0;
	CLIENT_NSDB_CREATE_TABLE_INFO_S stTableInfo = {};

	uiBufPoint += MSG_de_DecData(pstMsgRecvInfo->pRecvDataBuf,
 							     &(pstMsgRecvInfo->uiDataType),
 							     &stTableInfo);
 							     
	ulRet = Server_ProcClientCreateTable(pstMsgRecvInfo, &stTableInfo);
	
	return ulRet;
}



