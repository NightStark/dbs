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
/*
#define LOCAL_IF_NAME "p3p1"

ULONG CLIENT_AskServer(IN INT iClientfd)
{
	ULONG ulRet = 0;	
	UINT  uiMsgLen = 0;
	UINT  uiRecvMsgLen = 0;
	CHAR aucDataBuf[NSDB_MSG_LEN_MAX];
	MSG_CLIENT_INFO_S stClientInfo;

	mem_set0(aucDataBuf, NSDB_MSG_LEN_MAX);
	NET_GetHosName(stClientInfo.szClientHostName,
				   sizeof(stClientInfo.szClientHostName));
	NET_GetLocalIPByIFName("p3p1", &stClientInfo.stCNetIFData.szIFName);
	uiMsgLen = MSG_ec_EncMsg( MSG_TYPE_NSDB_ASK_SERVICE_REQ,
							  MSG_DATA_TYPE_CLIENT_INFO,
							  &stClientInfo,
							  sizeof(stClientInfo),
							  aucDataBuf);

	ulRet = send(iClientfd, aucDataBuf, uiMsgLen, 0);
	if(-1 == ulRet)
	{
		ERR_PRINTF("sent error!");
	}
	DBG_PRINT_LOG("MSG.bat", aucDataBuf, ulRet);
	ERR_PRINTF("send len %d", uiMsgLen);
	ERR_PRINTF("msg len %d", ulRet);

	ulRet = THREAD_wait_CondInit();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait cond init failed!");
		return ulRet;
	}
	
	ulRet = THREAD_wait();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait create failed!");
		return ulRet;
	}
	THREAD_quemsg_Read(VOID * pData,
	INT iTranChlID,
	QUEMSG_DATA_DELETE pfQueMsgDataDelete,
	QUEMSG_DATA_OUT pfQueMsgDataOut);
	
	ulRet = THREAD_wait_CondFint();
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("wait destroy failed!");
		return ulRet;
	}

	ERR_PRINTF("Client is wake UP");

	return ERROR_SUCCESS;
}
*/

