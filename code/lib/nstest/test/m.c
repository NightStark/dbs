#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 
#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_lilist.h>
#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_table_type.h>
#include <ns_nsdb.h>
#include <ns_net.h>
#include <ns_msg.h>

#include <ns_test.h>

/* 可升级为由字典生成 */

ULONG CreateClientTable(UINT uiCreateNum)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	 
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

	for(uiLoop = 0; uiLoop < uiCreateNum; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);
				
		sprintf(pstTableEle[0].szTypeName,
				"Client_ID-%d",uiLoop);
		sprintf(pstTableEle[1].szTypeName,
				"Client_HostName-%d",uiLoop);
		sprintf(pstTableEle[2].szTypeName,
				"Client_InterfaceName-%d",uiLoop);
		sprintf(pstTableEle[3].szTypeName,
				"Client_InterfaceAddr-%d",uiLoop);
		sprintf(pstTableEle[4].szTypeName,
				"Client_InterfaceMACAddr-%d",uiLoop);
		sprintf(pstTableEle[5].szTypeName,
				"Client_LinkFd-%d",uiLoop);

		ulRet = NSDB_CreateTable((UCHAR *)ucTableName, pstTableEle); 	
		if(ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Cant't get Table Fd!");
			return ERROR_FAILE;
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,uiCreateNum);
	}
	printf("\n");


	return ERROR_SUCCESS;
}

ULONG Table_CK(UINT uiCreateNum)
{
	TABLE_ELE_S pstTableEle[6];

	ULONG    ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	
	for(uiLoop = 0; uiLoop < uiCreateNum; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);
				
		sprintf(pstTableEle[0].szTypeName,
				"Client_ID-%d",uiLoop);
		sprintf(pstTableEle[1].szTypeName,
				"Client_HostName-%d",uiLoop);
		sprintf(pstTableEle[2].szTypeName,
				"Client_InterfaceName-%d",uiLoop);
		sprintf(pstTableEle[3].szTypeName,
				"Client_InterfaceAddr-%d",uiLoop);
		sprintf(pstTableEle[4].szTypeName,
				"Client_InterfaceMACAddr-%d",uiLoop);
		sprintf(pstTableEle[5].szTypeName,
				"Client_LinkFd-%d",uiLoop);
				
		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL == pstTable)
		{
			ERR_PRINTF("Table_Get Failed![%s]", ucTableName);
			return ERROR_DATA_NOT_EXIST;
		}
		pstType = pstTable->pstTableTypes;

		
		for(uiTableEleI = 0; uiTableEleI < 6; uiTableEleI++)
		{
			ulRet = str_safe_cmp(pstTableEle[uiTableEleI].szTypeName,
	  								pstType->pucTypeName[uiTableEleI],
	  								sizeof(pstTableEle[uiTableEleI].szTypeName));
			if (0 != ulRet)
			{
				ERR_PRINTF("Table_Type Failed!");
				return ERROR_FAILE;				
			}
			else
			{
			}
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,uiCreateNum);
	}
	printf("\n");

	MSG_PRINTF("[All Table is Find]");
	return ERROR_SUCCESS;
}

ULONG DelClientTable(UINT uiCreateNum)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";

	 
	

	for(uiLoop = 0; uiLoop < uiCreateNum; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		ulRet = NSDB_DeleteTable((UCHAR *)ucTableName); 	
		if(ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("delete Table Failed!");
			return ERROR_FAILE;
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,uiCreateNum);
	}
	printf("\n");


	return ERROR_SUCCESS;
}


ULONG DelTable_CK(UINT uiCreateNum)
{

	TABLE_ELE_S pstTableEle[6];

	ULONG    ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	
	for(uiLoop = 0; uiLoop < uiCreateNum; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);
								
		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL != pstTable)
		{
			ERR_PRINTF("Table Delete Failed![%s]", ucTableName);
		}
				
				DBG_Print_Rate_of_Progress(uiLoop + 1,uiCreateNum);
	}
	printf("\n");

	return ERROR_SUCCESS;
}



#define TABLE_NUM (1000)

NSTEST(Table_Create_1)
{
	BitMap_Init();
	DataBase_Init();
	printf("Create table\n");
	CreateClientTable(1);
	printf("ck Createed table\n");
	Table_CK(1);
	printf("Delete table\n");
	DelClientTable(1);
	printf("ck Createed table\n");
	DelTable_CK(1);
	DataBase_Fini();	
	return;
}

NSTEST(Table_Create_2)
{
	BitMap_Init();
	DataBase_Init();
	printf("Create table\n");
	CreateClientTable(2);
	printf("ck Createed table\n");
	Table_CK(2);
	printf("Delete table\n");
	DelClientTable(2);
	printf("ck Createed table\n");
	DelTable_CK(2);
	DataBase_Fini();	
	return;
}

NSTEST(Table_Create_3)
{
	BitMap_Init();
	DataBase_Init();
	printf("Create table\n");
	CreateClientTable(3);
	printf("ck Createed table\n");
	Table_CK(3);
	printf("Delete table\n");
	DelClientTable(3);
	printf("ck Createed table\n");
	DelTable_CK(3);
	DataBase_Fini();	
	return;
}


NSTEST(Table_Create_side_first)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	TABLE_S *pstTable 		= NULL;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";

	BitMap_Init();
	DataBase_Init();
	
	printf("Create table\n");
	CreateClientTable(3);
	
	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",0);

	ulRet = NSDB_DeleteTable((UCHAR *)ucTableName); 	
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("delete Table Failed!");
		return ERROR_FAILE;
	}
	
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL != pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}

	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",1);
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}
	
	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",2);
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}
	DataBase_Fini();	
		
	return;
}

NSTEST(Table_Create_side_end)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	TABLE_S *pstTable 		= NULL;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = "                    ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";

	BitMap_Init();
	DataBase_Init();
	
	printf("Create table\n");
	CreateClientTable(3);
	
	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",2);

	ulRet = NSDB_DeleteTable((UCHAR *)ucTableName); 	
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("delete Table Failed!");
		return ERROR_FAILE;
	}
	
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL != pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}

	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",0);
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}
	
	sprintf(ucTableName,
			"NSDB_CLIENT_INFO_TABLE-%d",1);
	pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
	}
	DataBase_Fini();	
		
	return;
}


NSTEST(Table_Create_DelLots)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = " 				   ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	TABLE_S *pstTable 		= NULL;
	
	BitMap_Init();
	DataBase_Init();
	printf("Create 1000 tables\n");
	CreateClientTable(1000);

	printf("del 125 ~ 777 tables\n");
	for(uiLoop = 125; uiLoop < 778; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		ulRet = NSDB_DeleteTable((UCHAR *)ucTableName); 	
		if(ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("delete Table Failed!");
			return ERROR_FAILE;
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1 - 125,778-125);
	}
	
	printf("\ncheck 0 ~ 124 tables\n");
	for(uiLoop = 0; uiLoop < 125; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL == pstTable)
		{
			ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,125);
	}
	
	printf("\ncheck 127 ~ 777 tables\n");
	for(uiLoop = 125; uiLoop < 778; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL != pstTable)
		{
			ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1 - 125,778-125);
	}
	
	printf("\ncheck 777 ~ 999 tables\n");
	for(uiLoop = 778; uiLoop < 1000; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL == pstTable)
		{
			ERR_PRINTF("Table Delete side_first Failed![%s]", ucTableName);
		}
		
		DBG_Print_Rate_of_Progress(uiLoop + 1 - 778,1000-778);
	}
	printf("\n");
	
	DataBase_Fini();	
	
	return;
}

NSTEST(Table_Create_Big)
{
	BitMap_Init();
	DataBase_Init();
	printf("Create table\n");
	CreateClientTable(TABLE_NUM);
	printf("ck Createed table\n");
	Table_CK(TABLE_NUM);
	printf("Delete table\n");
	DelClientTable(TABLE_NUM);
	printf("ck Createed table\n");
	DelTable_CK(TABLE_NUM);
	DataBase_Fini();	
	
	return;
}



ULONG __AddClientInfo(IN MSG_CLIENT_INFO_S *pstMsgClientInfo, 
							IN UINT uiClient_LinkFd,
							UCHAR *szTableName,
							UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT  uiClient_ID       = uiClient_LinkFd + 0xFF000000;
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

	
	sprintf(stAddDataEle[0].szTypeName,
			"Client_ID-%d",uiTableIloop);
	sprintf(stAddDataEle[1].szTypeName,
			"Client_HostName-%d",uiTableIloop);
	sprintf(stAddDataEle[2].szTypeName,
			"Client_InterfaceName-%d",uiTableIloop);
	sprintf(stAddDataEle[3].szTypeName,
			"Client_InterfaceAddr-%d",uiTableIloop);
	sprintf(stAddDataEle[4].szTypeName,
			"Client_InterfaceMACAddr-%d",uiTableIloop);
	sprintf(stAddDataEle[5].szTypeName,
			"Client_LinkFd-%d",uiTableIloop);

	DBGASSERT(NULL != pstMsgClientInfo);
		
	ulRet = NSDB_AddData((UCHAR *)szTableName, 
						(VOID *)(stAddDataEle), 
						6);	

	return ulRet;
}


ULONG AddClientInfo(UINT uiDataNum, UCHAR *szTableName,UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT uiLoop;
	UCHAR szClientHostName[NSDB_HOST_NAME_LEN_MAX]	= {};
	UCHAR szIFName[NET_INTERFACE_NAME_LEN_MAX]		= {};
	UCHAR szIFAddr[NET_INTERFACE_ADDR_LEN_MAX]		= {};
	UCHAR szIFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX]= {};
	UINT  uiClient_LinkFd;

    struct sockaddr_in ip;

	MSG_CLIENT_INFO_S stMsgClientInfo = {0};
	
	for(uiLoop = 0; uiLoop < uiDataNum; uiLoop++)
	{
	
		sprintf(stMsgClientInfo.szClientHostName, "client-ns-%d", uiLoop);
		
		sprintf(stMsgClientInfo.stCNetIFData.szIFName,		  "p3p-%d", uiLoop);
		
		ip.sin_addr.s_addr=0xC0A80001 + uiLoop;
		sprintf(stMsgClientInfo.stCNetIFData.szIFAddr,		  "%s", inet_ntoa(ip.sin_addr));

		stMsgClientInfo.stCNetIFData.szIFMACAddr[0] = (0xFF & (uiLoop + 0xAA));
		stMsgClientInfo.stCNetIFData.szIFMACAddr[1] = (0xFF & (uiLoop + 0x55));

		//sprintf(szClientHostName, "client-ns-%d", uiLoop);

		uiClient_LinkFd = uiLoop + 0x880800;
			
		ulRet |= __AddClientInfo(&stMsgClientInfo, 
								uiClient_LinkFd,
								szTableName,
								uiTableIloop);

		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_AddData Failed!");		
		}

	}

	return ulRet;
}

ULONG CK_AddClientInfo(UINT uiDataNum, UCHAR *szTableName,UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT uiLoop;
	UINT  uiClient_ID       = 0;
	UCHAR szClientHostName[NSDB_HOST_NAME_LEN_MAX]	= {};
	UCHAR szIFName[NET_INTERFACE_NAME_LEN_MAX]		= {};
	UCHAR szIFAddr[NET_INTERFACE_ADDR_LEN_MAX]		= {};
	UCHAR szIFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX]= {};
	UINT  uiClient_LinkFd;
    struct sockaddr_in ip;

	int i;
	UINT  uiGetClient_ID = 0;
	UCHAR szClient_IFHostName[NSDB_HOST_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFName[NET_INTERFACE_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFAddr[NET_INTERFACE_ADDR_LEN_MAX]       = {};
	UCHAR szClient_IFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX] = {};
	UINT  uiClient_LinkFD = 0;
	ULONG ulTableFd = TABLE_FD_INVALID;
	
	GET_DATA_ELE_S astGetDataEle[] = {
		[0] = {
			"Client_ID",
			AT_ULONG,
			(&uiGetClient_ID),
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

	sprintf(astGetDataEle[0].szTypeName,
			"Client_ID-%d",uiTableIloop);
	sprintf(astGetDataEle[1].szTypeName,
			"Client_HostName-%d",uiTableIloop);
	sprintf(astGetDataEle[2].szTypeName,
			"Client_InterfaceName-%d",uiTableIloop);
	sprintf(astGetDataEle[3].szTypeName,
			"Client_InterfaceAddr-%d",uiTableIloop);
	sprintf(astGetDataEle[4].szTypeName,
			"Client_InterfaceMACAddr-%d",uiTableIloop);
	sprintf(astGetDataEle[5].szTypeName,
			"Client_LinkFd-%d",uiTableIloop);

	
	for(uiLoop = 0; uiLoop < uiDataNum; uiLoop++)
	{	
		sprintf(szClientHostName, "client-ns-%d", uiLoop);
		
		sprintf(szIFName,		  "p3p-%d", uiLoop);
		
		ip.sin_addr.s_addr=0xC0A80001 + uiLoop;
		sprintf(szIFAddr,		  "%s", inet_ntoa(ip.sin_addr));
		
		szIFMACAddr[0] = (0xFF & (uiLoop + 0xAA));
		szIFMACAddr[1] = (0xFF & (uiLoop + 0x55));
		
		uiClient_LinkFd = uiLoop + 0x880800;
		uiClient_ID 	= uiClient_LinkFd + 0xFF000000;

		ulRet = NSDB_FindData(szTableName,
							  astGetDataEle[1].szTypeName,
							  szClientHostName,
							  astGetDataEle);

		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_FindData Cant find data! at:0x%x", ulRet);
			continue;
		}

		if(uiClient_ID != *(ULONG*)(astGetDataEle[0].pData))
		{
			ERR_PRINTF("find uiClient_LinkFd error data!");
		}
		
		ulRet = str_safe_cmp(astGetDataEle[1].pData,
							szClientHostName,
							sizeof(szClientHostName));
		if (0 != ulRet){
			ERR_PRINTF("find szClientHostName error data!");
		}
		ulRet = str_safe_cmp(astGetDataEle[2].pData,
							szIFName,
							sizeof(szIFName));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFName error data!");
		}
		ulRet = str_safe_cmp(astGetDataEle[3].pData,
							szIFAddr,
							sizeof(szIFAddr));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFAddr error data!");
		}
		ulRet = mem_safe_cmp(astGetDataEle[4].pData,
							 szIFMACAddr,
							 sizeof(szIFMACAddr));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFMACAddr error data!");
		}
		
		if(uiClient_LinkFd != *(UINT*)(astGetDataEle[5].pData))
		{
			ERR_PRINTF("find uiClient_LinkFd error data!");
		}

	}
	
	

	return ulRet;
}




ULONG __UpdateClientInfo(IN MSG_CLIENT_INFO_S *pstMsgClientInfo, 
							 IN UINT uiClient_LinkFd,
							 UCHAR *szTableName,
							 UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT  uiClient_ID       = uiClient_LinkFd + 0xFF000000;
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

	
	sprintf(stAddDataEle[0].szTypeName,
			"Client_ID-%d",uiTableIloop);
	sprintf(stAddDataEle[1].szTypeName,
			"Client_HostName-%d",uiTableIloop);
	sprintf(stAddDataEle[2].szTypeName,
			"Client_InterfaceName-%d",uiTableIloop);
	sprintf(stAddDataEle[3].szTypeName,
			"Client_InterfaceAddr-%d",uiTableIloop);
	sprintf(stAddDataEle[4].szTypeName,
			"Client_InterfaceMACAddr-%d",uiTableIloop);
	sprintf(stAddDataEle[5].szTypeName,
			"Client_LinkFd-%d",uiTableIloop);

	DBGASSERT(NULL != pstMsgClientInfo);

	/* 更新数据 */
	ulRet = NSDB_UpdateData((UCHAR *)szTableName,
			 	                (VOID *)(stAddDataEle),
			 	                6,
			 	                stAddDataEle[0].szTypeName,
			 	                &uiClient_ID);
	          
	return ulRet;
}



ULONG UpdateClientInfo(UINT uiDataNum, UCHAR *szTableName,UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT uiLoop = 0;
	UCHAR szClientHostName[NSDB_HOST_NAME_LEN_MAX]	= {};
	UCHAR szIFName[NET_INTERFACE_NAME_LEN_MAX]		= {};
	UCHAR szIFAddr[NET_INTERFACE_ADDR_LEN_MAX]		= {};
	UCHAR szIFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX]= {};
	UINT  uiClient_LinkFd;

    struct sockaddr_in ip;

	MSG_CLIENT_INFO_S stMsgClientInfo = {0};
	
	for(uiLoop = 0; uiLoop < uiDataNum; uiLoop++)
	{
	
		sprintf(stMsgClientInfo.szClientHostName,      "client-ns-%d", uiLoop);
		sprintf(stMsgClientInfo.stCNetIFData.szIFName, "new-p3p-%d", uiLoop);
		ip.sin_addr.s_addr=0xC0A80001 + uiLoop;
		sprintf(stMsgClientInfo.stCNetIFData.szIFAddr, "%s", inet_ntoa(ip.sin_addr));

		stMsgClientInfo.stCNetIFData.szIFMACAddr[0] = (0xFF & (uiLoop + 0xAA));
		stMsgClientInfo.stCNetIFData.szIFMACAddr[1] = (0xFF & (uiLoop + 0x55));

		//sprintf(szClientHostName, "client-ns-%d", uiLoop);

		uiClient_LinkFd = uiLoop + 0x880800;

		ulRet |= __UpdateClientInfo(&stMsgClientInfo,uiClient_LinkFd,szTableName,uiTableIloop);

		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_AddData Failed!");		
		}

	}

	return ulRet;
}


ULONG CK_UpdateClientInfo(UINT uiDataNum, UCHAR *szTableName,UINT uiTableIloop)
{
	ULONG ulRet             = ERROR_SUCCESS;
	UINT uiLoop;
	UINT  uiClient_ID       = 0;
	UCHAR szClientHostName[NSDB_HOST_NAME_LEN_MAX]	= {};
	UCHAR szIFName[NET_INTERFACE_NAME_LEN_MAX]		= {};
	UCHAR szIFAddr[NET_INTERFACE_ADDR_LEN_MAX]		= {};
	UCHAR szIFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX]= {};
	UINT  uiClient_LinkFd;
    struct sockaddr_in ip;

	int i;
	UINT  uiGetClient_ID = 0;
	UCHAR szClient_IFHostName[NSDB_HOST_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFName[NET_INTERFACE_NAME_LEN_MAX]       = {};
	UCHAR szClient_IFAddr[NET_INTERFACE_ADDR_LEN_MAX]       = {};
	UCHAR szClient_IFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX] = {};
	UINT  uiClient_LinkFD = 0;
	ULONG ulTableFd = TABLE_FD_INVALID;
	
	GET_DATA_ELE_S astGetDataEle[] = {
		[0] = {
			"Client_ID",
			AT_ULONG,
			(&uiGetClient_ID),
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

	sprintf(astGetDataEle[0].szTypeName,
			"Client_ID-%d",uiTableIloop);
	sprintf(astGetDataEle[1].szTypeName,
			"Client_HostName-%d",uiTableIloop);
	sprintf(astGetDataEle[2].szTypeName,
			"Client_InterfaceName-%d",uiTableIloop);
	sprintf(astGetDataEle[3].szTypeName,
			"Client_InterfaceAddr-%d",uiTableIloop);
	sprintf(astGetDataEle[4].szTypeName,
			"Client_InterfaceMACAddr-%d",uiTableIloop);
	sprintf(astGetDataEle[5].szTypeName,
			"Client_LinkFd-%d",uiTableIloop);

	
	for(uiLoop = 0; uiLoop < uiDataNum; uiLoop++)
	{	
		sprintf(szClientHostName, "client-ns-%d", uiLoop);
		sprintf(szIFName, "new-p3p-%d", uiLoop);		
		ip.sin_addr.s_addr=0xC0A80001 + uiLoop;
		sprintf(szIFAddr,		  "%s", inet_ntoa(ip.sin_addr));
		
		szIFMACAddr[0] = (0xFF & (uiLoop + 0xAA));
		szIFMACAddr[1] = (0xFF & (uiLoop + 0x55));
		
		uiClient_LinkFd = uiLoop + 0x880800;
		uiClient_ID 	= uiClient_LinkFd + 0xFF000000;

		ulRet = NSDB_FindData(szTableName,
							  astGetDataEle[1].szTypeName,
							  szClientHostName,
							  astGetDataEle);

		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_FindData Cant find data! at:0x%x", ulRet);
			continue;
		}

		if(uiClient_ID != *(ULONG*)(astGetDataEle[0].pData))
		{
			ERR_PRINTF("find uiClient_LinkFd error data!");
		}
		
		ulRet = str_safe_cmp(astGetDataEle[1].pData,
							szClientHostName,
							sizeof(szClientHostName));
		if (0 != ulRet){
			ERR_PRINTF("find szClientHostName error data!");
		}
		ulRet = str_safe_cmp(astGetDataEle[2].pData,
							szIFName,
							sizeof(szIFName));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFName error data!");
		}
		ulRet = str_safe_cmp(astGetDataEle[3].pData,
							szIFAddr,
							sizeof(szIFAddr));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFAddr error data!");
		}
		ulRet = mem_safe_cmp(astGetDataEle[4].pData,
							 szIFMACAddr,
							 sizeof(szIFMACAddr));
		if (0 != ulRet) {
			ERR_PRINTF("find szIFMACAddr error data!");
		}
		
		if(uiClient_LinkFd != *(UINT*)(astGetDataEle[5].pData))
		{
			ERR_PRINTF("find uiClient_LinkFd error data!");
		}

	}
	
	

	return ulRet;
}

NSTEST(Table_AddData_Big)
{
	ULONG ulRet = ERROR_SUCCESS;
	UINT uiLoop;
	UINT uiTableEleI;
	UINT uiProcP = 0;
	UCHAR ucProcPGO[32] = " 				   ";
	UCHAR ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	TABLE_S *pstTable 		= NULL;
	UINT uiAddClientInfoNum = 100;
	
	BitMap_Init();
	DataBase_Init();
	
	printf("Create 1000 tables\n");
	CreateClientTable(1000);

	printf("add client info and check\n");
	for(uiLoop = 0; uiLoop < 10; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL == pstTable)
		{
			ERR_PRINTF("Table Cant Find![%s]", ucTableName);
			continue;
		}

		ulRet = AddClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_AddData Failed!");		
		}
		//printf("100 Client info add success\n");
		
		CK_AddClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,10);
	}

	/*

	delete client info and check 
	.....
		
	*/
	
	printf("\nDelete table\n");
	DelClientTable(1000);
	DataBase_Fini();	
	
	return;
}

NSTEST(Table_AddData_1)
{

}
NSTEST(Table_AddData_2)
{

}
NSTEST(Table_AddData_3)
{

}
NSTEST(Table_AddData_side_first)
{

}
NSTEST(Table_AddData_side_end)
{

}
NSTEST(Table_AddData_del_lots)
{

}

/**********************多线程测试******************************/


NSTEST(Table_AddData_MutiThread)
{
	ULONG 	ulRet = ERROR_SUCCESS;
	UINT 	uiLoop = 0;
	UINT 	uiTableEleI;
	UINT 	uiProcP = 0;
	UCHAR 	ucProcPGO[32] = " 				   ";
	UCHAR 	ucTableName[32] = "NSDB_CLIENT_INFO_TABLE";
	TABLE_S *pstTable 		= NULL;
	UINT 	uiAddClientInfoNum = 100;
	
	BitMap_Init();
	DataBase_Init();
	
	printf("Create 1000 tables\n");
	CreateClientTable(1000);

	printf("add client info and check\n");
	for(uiLoop = 0; uiLoop < 100; uiLoop++)
	{
		sprintf(ucTableName,
				"NSDB_CLIENT_INFO_TABLE-%d",uiLoop);

		pstTable = Table_GetTableByTableName((const UCHAR *)ucTableName);
		if (NULL == pstTable)
		{
			ERR_PRINTF("Table Cant Find![%s]", ucTableName);
			continue;
		}
		
		//printf("add client info \n");
		ulRet = AddClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_AddData Failed!");		
		}
		
		//printf("check add client info \n");
		CK_AddClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		//printf("update client info \n");
		ulRet = UpdateClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("NSDB_AddData Failed!");		
		}
		
		//printf("check update client info \n");
		CK_UpdateClientInfo(uiAddClientInfoNum, ucTableName, uiLoop);
		
		
		DBG_Print_Rate_of_Progress(uiLoop + 1,100);
	}

	/*

	delete client info and check 
	.....
		
	*/
	
	printf("\nDelete table\n");
	DelClientTable(1000);
	DataBase_Fini();	
	
	return;
}



