#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_table_type.h>
#include <ns_nsdb.h>


ULONG MSG_server_ClientCreateTableResp(IN INT iClientfd, IN UINT uiRespCode)
{
	ULONG ulRet = 0;	
	UINT  uiMsgLen = 0;
	CHAR  aucDataBuf[NSDB_MSG_LEN_MAX];
	CLIENT_NSDB_CREATE_TABLE_RESP stClientCreateTableRespCode;

	mem_set0(aucDataBuf, NSDB_MSG_LEN_MAX);
	mem_set0(&stClientCreateTableRespCode, sizeof(CLIENT_NSDB_CREATE_TABLE_RESP));

	stClientCreateTableRespCode.uiRespCode = uiRespCode;
	
	uiMsgLen = MSG_ec_EncMsg( MSG_TYPE_NSDB_SELECT_CREATE_TABLE_RESP,
							  MSG_DATA_TYPE_NSDB_TABLE_CREATE_RESP_CODE,
							  &stClientCreateTableRespCode,
							  sizeof(stClientCreateTableRespCode),
							  aucDataBuf);

	ulRet = send(iClientfd, aucDataBuf, uiMsgLen, 0);
	
	if(-1 == ulRet)
	{
		ERR_PRINTF("sent error!");
	}
	
	MSG_PRINTF("ClientCreateTableResp len %d", uiMsgLen);
	MSG_PRINTF("ClientCreateTableResp len %lu", ulRet);
	DBG_PRINT_LOG("ClientCreateTableResp.bat", aucDataBuf, ulRet);

	return ERROR_SUCCESS;
}



ULONG Server_ProcClientCreateTable(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo,
										 CLIENT_NSDB_CREATE_TABLE_INFO_S *pstTableInfo)
{
	ULONG ulRet = 0;
	UINT uiRespCode = 0;

	ulRet = NSDB_CreateTable(pstTableInfo->szTableName , pstTableInfo->pstTableEle);
	if (ERROR_SUCCESS == ulRet)
	{
		uiRespCode = SERVER_CLIENT_NSdB_CREATE_TABLE_SUCCESS;	    
	}
	else
	{
		uiRespCode = SERVER_CLIENT_NSdB_CREATE_TABLE_FAILED;	    
	}
	MSG_server_ClientCreateTableResp((INT)pstMsgRecvInfo->uiLinkFD,uiRespCode);

	/*
	UINT uiIndex = 0;
	CHAR cTableEleCnt = 0;
	UINT *puiTableEleCnt = 0;
	TABLE_ELE_S stTableEle[10];
	cTableEleCnt = NSDB_GetTableInfo(pstTableInfo->szTableName, &stTableEle);

	for (uiIndex = 0; uiIndex < cTableEleCnt; uiIndex++)
	{
		ERR_PRINTF("sz-- : %s", stTableEle[uiIndex].szTypeName);
	}
	*/

	return ERROR_SUCCESS;
}

