#ifdef __cplusplus
extern "C"
{
#endif

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
#include <ns_fsm_msg_proc.h>

typedef ULONG (* PFFSMMSGPROC)(MSG_MSG_RECV_INFO_S *);

STATIC PFFSMMSGPROC g_apfFSMMsgProcList[MSG_TYPE_NSDB_TOP] = 
{		//FOR C++(g++) not support "[MSG_TYPE_NSDB_ASK_SERVICE_REQ] = FSM_msg_proc_AskServiceReq" fuck
		/* MSG_TYPE_NSDB_NONE = 0, */ NULL,

		/* [MSG_TYPE_NSDB_ASK_SERVICE_REQ] =        */ FSM_msg_proc_AskServiceReq,
		/* MSG_TYPE_NSDB_ASK_SERVICE_RESP, */ NULL,

		/* MSG_TYPE_NSDB_SELECT_REQ, */ NULL,
		/* MSG_TYPE_NSDB_SELECT_RESP, */ NULL,

		/* [MSG_TYPE_NSDB_SELECT_CREATE_TABLE_REQ] = */ FSM_msg_proc_ClientCreateTable,
		/* MSG_TYPE_NSDB_SELECT_CREATE_TABLE_RESP, */ NULL,
};

ULONG FMS_msg_Proc(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{
	ULONG ulRet = ERROR_SUCCESS;
	
	DBGASSERT(NULL != pstMsgRecvInfo);
	
	ulRet = g_apfFSMMsgProcList[pstMsgRecvInfo->ucMsgType](pstMsgRecvInfo);

	return ulRet;
}

ULONG FSM_Init(VOID)
{
	return ERROR_SUCCESS;
}

#ifdef __cplusplus
}
#endif
