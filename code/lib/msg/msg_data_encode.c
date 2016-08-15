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

typedef UINT (* MsgDataEncFunc)(VOID *, VOID *);

STATIC UINT msg_datadenc_ClientInfo(VOID *pDataStruct, VOID *pDataBuf)
{
	/* 临时发，以后要每个成员单独赋值 */
	memcpy(pDataBuf, pDataStruct, sizeof(MSG_CLIENT_INFO_S));

	return sizeof(MSG_CLIENT_INFO_S);
}

STATIC UINT msg_datadenc_ServerRespCode(VOID *pDataStruct, VOID *pDataBuf)
{
	/* 临时发，以后要每个成员单独赋值 */
	memcpy(pDataBuf, pDataStruct, sizeof(MSG_SERVER_RESP_CODE_S));

	return sizeof(MSG_SERVER_RESP_CODE_S);
}


STATIC UINT msg_datadenc_NsdbTableCreate(VOID *pDataStruct, VOID *pDataBuf)
{
	UINT uiDataLen = 0;
	UINT uiIndex = 0;
	UINT *puiTableEleCnt = 0;
	CHAR *pcDataBufPoint = NULL;
	
	CLIENT_NSDB_CREATE_TABLE_INFO_S *pstTableInfo = NULL;
	
	pstTableInfo = (CLIENT_NSDB_CREATE_TABLE_INFO_S *)pDataStruct;
	pcDataBufPoint = (CHAR *)pDataBuf;
	
	strcpy((CHAR *)pDataBuf, pstTableInfo->szTableName);
	uiDataLen       = sizeof(pstTableInfo->szTableName);
	pcDataBufPoint += sizeof(pstTableInfo->szTableName);
	
	puiTableEleCnt  = (UINT *)pcDataBufPoint;
	uiDataLen      += sizeof(UINT);
	pcDataBufPoint += sizeof(UINT);

	do
	{
		memcpy(pcDataBufPoint, &(pstTableInfo->pstTableEle[uiIndex]), sizeof(TABLE_ELE_S));
		pcDataBufPoint  += sizeof(TABLE_ELE_S);
		uiDataLen       += sizeof(TABLE_ELE_S);
	}while(AT_NONE != pstTableInfo->pstTableEle[uiIndex++].ucEleType);

	(*puiTableEleCnt) = uiIndex - 1;
	
	return uiDataLen;
}

STATIC UINT msg_datadenc_NsdbTableCreateRespCode(VOID *pDataStruct, VOID *pDataBuf)
{
	/* 临时发，以后要每个成员单独赋值 */
	memcpy(pDataBuf, pDataStruct, sizeof(CLIENT_NSDB_CREATE_TABLE_RESP));

	return sizeof(CLIENT_NSDB_CREATE_TABLE_RESP);
}


MsgDataEncFunc g_pDataEncFunc[20] = {
	[MSG_DATA_TYPE_CLIENT_INFO]          = msg_datadenc_ClientInfo,
	[MSG_DATA_TYPE_ASK_SERVER_RESP_CODE] = msg_datadenc_ServerRespCode,
	[MSG_DATA_TYPE_NSDB_TABLE_INFO]      = msg_datadenc_NsdbTableCreate,
	[MSG_DATA_TYPE_NSDB_TABLE_CREATE_RESP_CODE]= msg_datadenc_NsdbTableCreateRespCode,

}; 

UINT  MSG_datadenc_DataStruct(UINT uiDataType, 
							  VOID *pDataStruct, 
							  VOID *pDataBuf)
{
	return g_pDataEncFunc[uiDataType](pDataStruct, pDataBuf);
}

UINT MSG_dataenc_Uint(IN UINT uiData, OUT CHAR *pcDataBuf)
{
	UINT *puiDataBuf = NULL;
	DBGASSERT(NULL != pcDataBuf);

	puiDataBuf = (UINT *)pcDataBuf;

	*puiDataBuf = uiData;

	return sizeof(UINT);
}


UINT NS_MSG_DATA_ENCODE_USHORT (INOUT UCHAR *pucBufStart, USHORT usData)
{
    USHORT *pusB = NULL; 

    DBGASSERT(NULL != pucBufStart);

    pusB = (USHORT *)pucBufStart;
    
    *pusB = htons(usData);

    return sizeof(USHORT);
}

UINT NS_MSG_DATA_ENCODE_UINT (INOUT UCHAR *pucBufStart, UINT uiData)
{
    UINT *puiB = NULL; 

    DBGASSERT(NULL != pucBufStart);

    puiB = (UINT *)pucBufStart;
    
    *puiB = htonl(uiData);

    return sizeof(UINT);
}

UINT NS_MSG_DATA_ENCODE_ULONG (INOUT UCHAR *pucBufStart, ULONG ulData)
{
    ULONG *pulB = NULL; 

    DBGASSERT(NULL != pucBufStart);

    pulB = (ULONG *)pucBufStart;
    
    *pulB = htonl(ulData);

    return sizeof(ULONG);
}

/* pucData ->encode -> pucBufStart */
UINT NS_MSG_DATA_ENCODE_DATAFLOW(INOUT UCHAR *pucBufStart, UINT uiBufLen, UCHAR *pucData, UINT uiDataLen)
{
    if (uiBufLen < uiDataLen) {
        ERR_PRINTF("buffer is not enougth.");
        return -1;
    }

    memcpy(pucBufStart, pucData, uiDataLen);

    return uiDataLen;
}

UINT NS_MSG_DATA_ENCODE_STRING(OUT UCHAR *pucBufStart, UINT uiBufLen, IN const CHAR *pucString, UINT uiStrLen)
{
    UINT uiLen = 0; 

    if (uiBufLen < uiStrLen) {
        ERR_PRINTF("buffer is not enougth.");
        return -1;
    }

    uiLen = snprintf((CHAR *)pucBufStart, uiBufLen, "%s", pucString);

    return uiLen;
}
