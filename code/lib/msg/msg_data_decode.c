#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>

typedef UINT (* MsgDataDecFunc)(VOID *, VOID *);

STATIC UINT MSG_datadec_ClientInfo(IN VOID *pDataBuf, OUT VOID *pDataStruct)
{
	/* 临时发，以后要每个成员单独赋值 */
	memcpy(pDataStruct, pDataBuf, sizeof(MSG_CLIENT_INFO_S));

	return sizeof(MSG_CLIENT_INFO_S);
}

STATIC UINT MSG_datadec_ClientCreateTable(IN VOID *pDataBuf, OUT VOID *pDataStruct)
{
	UINT uiDataLen = 0;
	CHAR *pcDataBufPoint = NULL;
	
	CLIENT_NSDB_CREATE_TABLE_INFO_S *pstTableInfo = NULL;
	
	pstTableInfo = (CLIENT_NSDB_CREATE_TABLE_INFO_S *)pDataStruct;
	pcDataBufPoint = (CHAR *)pDataBuf;

	/* 获得表名 */
	strcpy(pstTableInfo->szTableName, (CHAR *)pDataBuf);
	uiDataLen       = sizeof(pstTableInfo->szTableName);
	pcDataBufPoint += sizeof(pstTableInfo->szTableName);

	/* 获得表元素列表头指针 */
	pstTableInfo->uiTableEleCnt  = *(UINT *)pcDataBufPoint;
	uiDataLen      += sizeof(UINT);
	pcDataBufPoint += sizeof(UINT);
	MSG_PRINTF("pstTableInfo.uiTableEleCnt = %d", pstTableInfo->uiTableEleCnt);

	pstTableInfo->pstTableEle = (TABLE_ELE_S *)pcDataBufPoint;
	/*
	UINT uiIndex = 0;
	UINT *puiTableEleCnt = 0;
	pstTableInfo.pstTableEle = mem_alloc(sizeof(TABLE_ELE_S) * pstTableInfo.uiTableEleCnt);
	do
	{
		memcpy( &(pstTableInfo.pstTableEle[uiIndex]), pcDataBufPoint, sizeof(TABLE_ELE_S));
		pcDataBufPoint  += sizeof(TABLE_ELE_S);
		uiDataLen       += sizeof(TABLE_ELE_S);
		ERR_PRINTF("sz : %s", pstTableInfo.pstTableEle[uiIndex].szTypeName);
	}while(AT_NONE != pstTableInfo.pstTableEle[uiIndex++].ucEleType);
	*/
	
	return uiDataLen;
}

STATIC UINT MSG_datadec_AskSrcvRespCode(IN VOID *pDataBuf, OUT VOID *pDataStruct)
{
	memcpy(pDataStruct, pDataBuf, sizeof(MSG_SERVER_RESP_CODE_S));

	return sizeof(MSG_SERVER_RESP_CODE_S);
}

STATIC UINT MSG_datadec_TableCreateRespCode(IN VOID *pDataBuf, OUT VOID *pDataStruct)
{
	memcpy(pDataStruct, pDataBuf, sizeof(CLIENT_NSDB_CREATE_TABLE_RESP));

	return sizeof(CLIENT_NSDB_CREATE_TABLE_RESP);
}

MsgDataDecFunc g_pDataDecFunc[20] = {
	[MSG_DATA_TYPE_CLIENT_INFO] = MSG_datadec_ClientInfo,
	[MSG_DATA_TYPE_NSDB_TABLE_INFO] = MSG_datadec_ClientCreateTable,
	[MSG_DATA_TYPE_ASK_SERVER_RESP_CODE] = MSG_datadec_AskSrcvRespCode,
	[MSG_DATA_TYPE_NSDB_TABLE_CREATE_RESP_CODE] = MSG_datadec_TableCreateRespCode,
}; 

UINT MSG_datadec_DataStruct(IN  UINT uiDataType, 
 					        IN  VOID *pDataBuf ,
 					        OUT VOID *pDataStruct)
{
	return g_pDataDecFunc[uiDataType](pDataBuf, pDataStruct);
}

UINT MSG_datadec_Uint(IN CHAR *pcDataBuf, OUT UINT *puiData)
{
	DBGASSERT(NULL != pcDataBuf);
	DBGASSERT(NULL != puiData);

	*puiData = *(UINT *)pcDataBuf;

	return sizeof(UINT);
}

UINT MSG_datadec_Char(IN CHAR *pcDataBuf, OUT CHAR *pcData)
{
	DBGASSERT(NULL != pcDataBuf);
	DBGASSERT(NULL != pcData);

	*pcData = *(CHAR *)pcDataBuf;

	return sizeof(CHAR);
}

UINT MSG_datadec_UChar(IN CHAR *pcDataBuf, OUT UCHAR *pucData)
{
	DBGASSERT(NULL != pcDataBuf);
	DBGASSERT(NULL != pucData);

	*pucData = *(UCHAR *)pcDataBuf;

	return sizeof(UCHAR);
}


UINT NS_MSG_DATA_DECODE_USHORT (IN UCHAR *pucBufStart, INOUT USHORT *pusData)
{
    USHORT *pusB = NULL; 

    DBGASSERT(NULL != pucBufStart);
    DBGASSERT(NULL != pusData);

    pusB = (USHORT *)pucBufStart;
    
    *pusData = ntohs(*pusB);

    return sizeof(USHORT);
}

UINT NS_MSG_DATA_DECODE_UINT (IN UCHAR *pucBufStart, INOUT UINT *puiData)
{
    UINT *puiB = NULL; 

    DBGASSERT(NULL != pucBufStart);
    DBGASSERT(NULL != puiData);

    puiB = (UINT *)pucBufStart;
    
    *puiData = ntohl(*puiB);

    return sizeof(UINT);
}

UINT NS_MSG_DATA_DECODE_ULONG (IN UCHAR *pucBufStart, INOUT ULONG *pulData)
{
    ULONG *pulB = NULL; 

    DBGASSERT(NULL != pucBufStart);
    DBGASSERT(NULL != pulData);

    pulB = (ULONG *)pucBufStart;
    
    *pulData = ntohl(*pulB);

    return sizeof(ULONG);
}

/* @pucBufStart ==> @pucData */
UINT NS_MSG_DATA_DECODE_DATAFLOW(OUT UCHAR *pucData, UINT uiDataLen, INOUT UCHAR *pucBufStart, UINT uiBufLen)
{
    if (uiBufLen > uiDataLen) {
        ERR_PRINTF("buffer is not enougth.");
        return -1;
    }

    memcpy(pucData, pucBufStart, uiBufLen);

    return uiDataLen;
}

UINT NS_MSG_DATA_DECODE_STRING(OUT CHAR *pucString, OUT UINT uiStrBufLen, IN const CHAR *pucBufStart, IN UINT uiBufLen)
{
    UINT uiLen = 0; 

    if (uiStrBufLen < uiBufLen) {
        ERR_PRINTF("buffer is not enougth.[%d] < [%d]", uiStrBufLen, uiBufLen);
        return -1;
    }

    uiLen = snprintf((CHAR *)pucString, uiStrBufLen, "%s", pucBufStart);

    return uiLen;
}
