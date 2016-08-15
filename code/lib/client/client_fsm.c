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
#include <ns_client_fsm.h>

typedef ULONG (* PFCLIENTFSMMSGPROC)(MSG_MSG_RECV_INFO_S *);

STATIC PFCLIENTFSMMSGPROC g_apfClient_FSM_MsgProcList[MSG_TYPE_NSDB_TOP] = 
{
	[MSG_TYPE_NSDB_ASK_SERVICE_RESP] 	     = Client_fsm_msg_proc_AskServiceResp,
	[MSG_TYPE_NSDB_SELECT_CREATE_TABLE_RESP] = Client_fsm_msg_proc_ClientCreateTableResp,
};

ULONG Client_fsm_msg_Proc(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	ULONG ulRet = ERROR_SUCCESS;
	
	DBGASSERT(NULL != pstMsgRecvInfo);
	
	ulRet = g_apfClient_FSM_MsgProcList[pstMsgRecvInfo->ucMsgType](pstMsgRecvInfo);

	return ulRet;
}

ULONG Client_fsm_init(VOID)
{
	ULONG ulRet = ERROR_SUCCESS;
	
	return ulRet;
}

