#include <stdio.h>
#include <stdlib.h>
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
#include <ns_table_type.h>
#include <ns_nsdb.h>

//#define TABLE_NAME_CLIENT 		"NSDB_CLIENT_INFO_TABLE"
#define TABLE_CLIENT_ID_BASE 	(0x8FFF)

typedef struct tag_nsdbClientInfo
{
	ADD_DATA_ELE_S stClient_ID;
	ADD_DATA_ELE_S stClient_HostName;
	ADD_DATA_ELE_S stClient_IFName;
	ADD_DATA_ELE_S stClient_IFAddr;
	ADD_DATA_ELE_S stClient_IFMACAddr;
	ADD_DATA_ELE_S stClient_LinkFd;	
	//ADD_DATA_ELE_S stClient_State;		  /* client的状态 */
	//ADD_DATA_ELE_S stClient_OnLineMoment; /* client的上线时刻 */
	//ADD_DATA_ELE_S stClient_OnLineTime;	  /* client的连接时常*/
	//ADD_DATA_ELE_S stClient_OffLineMoment;/* client的下线时常*/
}NSDB_CLIENT_INFO_S;

/* 创建连接客户端的表 
Client_ID
Client_HostName
Client_InterfaceName
Client_InterfaceAddr
Client_InterfaceMACAddr
*/


ULONG CLIENT_nsdb_CreateTable(INT iClientfd,
							  CHAR *tableName, 
							  TABLE_ELE_S *pstTableEle)
{
	ULONG ulRet    = 0;
	UINT  uiMsgLen = 0;
	CHAR  aucDataBuf[NSDB_MSG_LEN_MAX] = {};
	CLIENT_NSDB_CREATE_TABLE_INFO_S stTableInfo;

	memset(&stTableInfo, 0, sizeof(CLIENT_NSDB_CREATE_TABLE_INFO_S));
	str_safe_cpy(stTableInfo.szTableName, tableName, sizeof(stTableInfo.szTableName));
	stTableInfo.pstTableEle = pstTableEle;

	uiMsgLen = MSG_ec_EncMsg( MSG_TYPE_NSDB_SELECT_CREATE_TABLE_REQ,
							  MSG_DATA_TYPE_NSDB_TABLE_INFO,
							  &stTableInfo,
							  sizeof(stTableInfo),
							  aucDataBuf);

	DBG_PRINT_LOG("client_create_table.dat", aucDataBuf, uiMsgLen);

	
	ulRet = CLIRNT_msg_send(iClientfd, aucDataBuf, uiMsgLen);


	return ulRet;
}

ULONG CLIENT_CreateClientTable(IN INT iClientfd)
{
	ULONG ulTableFd = TABLE_FD_INVALID;
	 
	TABLE_ELE_S pstTableEle[] = {
		[0] = {
			AT_ULONG,
			"Client_ID",
			Table_type_GetTypeLen(AT_ULONG),
		},
		[1] = {
			AT_SZ,
			"Client_HostName",
			NSDB_HOST_NAME_LEN_MAX,
		},
		[2] = {
			AT_SZ,
			"Client_InterfaceName",
			NET_INTERFACE_NAME_LEN_MAX,
		},
		[3] = {
			AT_SZ,
			"Client_InterfaceAddr",
			NET_INTERFACE_ADDR_LEN_MAX,
		},
		[4] = {
			AT_SZ,
			"Client_InterfaceMACAddr",
			NET_INTERFACE_MACADDR_LEN_MAX,
		},
		[5] = {
			AT_INT,
			"Client_LinkFd",
			Table_type_GetTypeLen(AT_INT),
		},
		
		[6] = {
			AT_NONE,
			"\0",
			0,
		},
	};

	ulTableFd = CLIENT_nsdb_CreateTable(iClientfd, "myFirstRealTable", pstTableEle); 
		
	if(ERROR_SUCCESS == ulTableFd)
	{
		MSG_PRINTF("CLIENT_nsdb_CreateTable SUCCESS!");
		//return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}





#if 0

ULONG CLIENT_AddClientInfo(IN MSG_CLIENT_INFO_S *pstMsgClientInfo, IN UINT uiClient_LinkFd)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT  uiClient_ID       = TABLE_CLIENT_ID_BASE + 1;
	ADD_DATA_ELE_S  stAddDataEle[] = {
		{
			"Client_ID",
			(&uiClient_ID),
		},
		{
			"Client_HostName",
			(pstMsgClientInfo->szClientHostName),
		},
		{
			"Client_InterfaceName",
			(pstMsgClientInfo->stCNetIFData.szIFName),
		},
		{
			"Client_InterfaceAddr",
			(pstMsgClientInfo->stCNetIFData.szIFAddr),
		},
		{
			"Client_InterfaceMACAddr",
			(pstMsgClientInfo->stCNetIFData.szIFMACAddr),
		},
		{
			"Client_LinkFd",
			(&uiClient_LinkFd),
		},
	};

	DBGASSERT(NULL != pstMsgClientInfo);
		
	ulRet = NSDB_AddData((UCHAR *)TABLE_NAME_CLIENT, 
						(VOID *)(stAddDataEle), 
						6);	

	return ulRet;
}



ULONG CLIENT_ShowClientInfo(VOID)
{
	int i;
	ULONG ulRet = ERROR_SUCCESS;
	ULONG ulDataConut = 0;

	UINT  uiClient_ID = 0;
	UCHAR szClient_IFHostName[NSDB_HOST_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFName[NET_INTERFACE_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFAddr[NET_INTERFACE_ADDR_LEN_MAX]       = {};
	UCHAR szClient_IFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX] = {};
	UINT  uiClient_LinkFD = 0;

	UCHAR szIFName[] = "p3p1";

	ULONG ulTableFd = TABLE_FD_INVALID;

	
	GET_DATA_ELE_S astGetDataEle[] = {
		[0] = {
			"Client_ID",
			AT_ULONG,
			(&uiClient_ID),
		},
		[1] = {
			"Client_HostName",
			AT_SZ,
			szClient_IFHostName,
		},
		[2] = {
			"Client_InterfaceName",
			AT_SZ,
			szClient_IFName,
		},
		[3] = {
			"Client_InterfaceAddr",
			AT_SZ,
			szClient_IFAddr,
		},
		[4] = {
			"Client_InterfaceMACAddr",
			AT_SZ,
			szClient_IFMACAddr,
		},
		[5] = {
			"Client_LinkFd",
			AT_INT,
			(&uiClient_LinkFD),
		}
	};
				
	ulRet = NSDB_FindData(TABLE_NAME_CLIENT, 
	 				     "Client_InterfaceName",
	 			         szIFName,
	 			         astGetDataEle);

	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Cant find data! at:0x%x", ulRet);
	}
	
	for(i = 0; i < 6; i++)
	{
		if ( AT_SZ== astGetDataEle[i].ucEleType)
		{
			MSG_PRINTF("%s : %s", astGetDataEle[i].szTypeName, (UCHAR *)(astGetDataEle[i].pData));
		}
		else
		{
			MSG_PRINTF("%s : %d", astGetDataEle[i].szTypeName, *(UINT *)(astGetDataEle[i].pData));
		}
	}

	//free(astGetDataEle);

	return ulDataConut;
}


ULONG CLIENT_ProcServerAskReq(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo)
{	
	ULONG ulRet = ERROR_SUCCESS;
	MSG_CLIENT_INFO_S *pstMsgClientInfo = NULL;

	DBGASSERT(NULL != pstMsgRecvInfo);

	pstMsgClientInfo = (MSG_CLIENT_INFO_S *)pstMsgRecvInfo->pRecvData;

	ERR_PRINTF("Client Host Name : %s" , pstMsgClientInfo->szClientHostName);

	ulRet = Server_AddClientInfo(pstMsgClientInfo, pstMsgRecvInfo->uiLinkFD);
	Server_ShowClientInfo();

	ERR_PRINTF("Client info do...");
	sleep(2);
	ERR_PRINTF("Client info Done!");

	if(ERROR_SUCCESS == ulRet)
	{
		MSG_server_AskServerResp((INT)pstMsgRecvInfo->uiLinkFD, (UINT)SERVER_ASK_IS_ACCEPT);
	}
	else
	{
	
	}
		
	return ERROR_SUCCESS;
}

#endif
