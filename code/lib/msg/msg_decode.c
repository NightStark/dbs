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

//#include "../include/msg.h"
/*
[   T    |   L    |    V                                                                         |
					 MT-------------C----------------N-----------------T----------------L-----------V
| 总类型 | 总长度 | 消息数据类型 | 有效数据条数 |   数据类型名列表   | 数据类型表(字节) | 长度表(UINT) | 数据区 | 
  1Byte      UINT       UINT          UINT-n     STRING[n]('\0'结尾)      UCHAR[n]         ULONG[n]     [data]n
*/

UINT MSG_de_DecData(VOID *pDataBuf,
	 					  UINT *uiDataType,
	 					  VOID *pDataStruct)
{
	CHAR *pcDataBuf = NULL;
	UINT  uiDataBufPoint = 0;

	DBGASSERT(NULL != pDataStruct);	
	DBGASSERT(NULL != pDataBuf);

	pcDataBuf = (CHAR *)pDataBuf;

	/* 解开封数据类型 */
	uiDataBufPoint += MSG_datadec_Uint(pcDataBuf, uiDataType);

	/* 解开封数据 */
	uiDataBufPoint += MSG_datadec_DataStruct(*uiDataType,
 											 pcDataBuf + uiDataBufPoint,
 											 pDataStruct);

	return uiDataBufPoint;
}

/*
UINT MSG_de_DecMsgHead(UCHAR ucMsgType, 
						    UINT uiDataType,
  						    VOID *pRecvData,
  						    UINT  uiSendDataLen,
  						    VOID *pDataBuf)
  						    */
UINT MSG_de_DecMsgHead(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo, VOID *pRecvDataBuf)
{
	UINT   uiBufPoint       = 0;
	CHAR  *pcPBuf			= NULL;

	DBGASSERT(NULL != pstMsgRecvInfo);
	DBGASSERT(NULL != pRecvDataBuf);

	pcPBuf = (CHAR *)pRecvDataBuf;
	
	uiBufPoint += MSG_datadec_UChar(pRecvDataBuf, &(pstMsgRecvInfo->ucMsgType));
	uiBufPoint += MSG_datadec_Uint(pcPBuf + uiBufPoint, &(pstMsgRecvInfo->uiMsgLen));

	pstMsgRecvInfo->pRecvDataBuf = pcPBuf + uiBufPoint;

/*
	uiBufPoint += MSG_de_DecData(pcPBuf + uiBufPoint,
 								 &(pstMsgRecvInfo->uiDataType),
 								 pstMsgRecvInfo->pRecvData);

	pstMsgRecvInfo->uiRecvDataLen = uiBufPoint;
*/

	return uiBufPoint;
}


NS_MSG_ST *NS_MSG_MsgBuf2MsgList (IN UCHAR *pucMsgBuf, IN UINT uiLen) 
{
    UINT uiOff        = 0;
    USHORT usMType    = 0;
    USHORT usSType    = 0;
    UINT   uiTotalLen = 0;
    UINT   uiSeq      = 0;
    UINT   uiDataType = 0;
    UINT   uiDataLen  = 0;
    VOID   *pDataFlow = 0;
    NS_MSG_ST *pstMsg = NULL;
    NS_MSG_DATA_ST *pstMsgData = NULL;
    UINT   uiDataP = 0;

    DBGASSERT(NULL != pucMsgBuf);

    if (uiLen < NS_MSG_HEADER_LEN) {
        ERR_PRINTF("buffer is too short.");
        return NULL;
    }

    /* parse msg header */
    uiOff += NS_MSG_DATA_DECODE_USHORT(pucMsgBuf + uiOff, &usMType);
    uiOff += NS_MSG_DATA_DECODE_USHORT(pucMsgBuf + uiOff, &usSType);
    uiOff += NS_MSG_DATA_DECODE_UINT(pucMsgBuf + uiOff, &uiTotalLen);
    uiOff += NS_MSG_DATA_DECODE_UINT(pucMsgBuf + uiOff, &uiSeq);

    if (uiTotalLen < NS_MSG_HEADER_LEN) {
        ERR_PRINTF("recv msg is too short.");
        return NULL;
    }

    pstMsg = MSG_Create(usMType, usSType);
    if (NULL == pstMsg) {
        ERR_PRINTF("msg create failed.");
        return NULL;
    }

    /* this three do in MSG_Create function.
       pstMsg->usMainType = usMainType;
       pstMsg->usSubType  = usSubType;
       DCL_Init(&pstMsg->stMsgHead);
       */
    pstMsg->uiTotalLen = uiTotalLen;
    pstMsg->uiSeq      = uiSeq;
    MSG_PRINTF("MainType:%d.", usMType);
    MSG_PRINTF("SubType:%d.", usSType);
    MSG_PRINTF("uiTotalLen:%d.", uiTotalLen);
    MSG_PRINTF("uiSeq:0x%x.", uiSeq);

    uiDataP = uiTotalLen - NS_MSG_HEADER_LEN;
    if (uiDataP < (sizeof(UINT) + sizeof(UINT))) {
        ERR_PRINTF("msg data is invalid");
        goto err;
    }

    /* buf flow to list */
    //mtype stype==>Desc & dec function
    //loop start
    while (uiDataP > 0) {
        uiOff += NS_MSG_DATA_DECODE_UINT(pucMsgBuf + uiOff, &uiDataType);
        uiOff += NS_MSG_DATA_DECODE_UINT(pucMsgBuf + uiOff, &uiDataLen);
        MSG_PRINTF("uiDataType:%d.", uiDataType);
        MSG_PRINTF("uiDataLen:%d.", uiDataLen);
        if (uiDataLen <= 0)
        {
            MSG_PRINTF("msg data is invalid.");
            goto err;
        }
        pDataFlow = malloc(uiDataLen);
        if (pDataFlow == NULL) {
            ERR_PRINTF("malloc failed.");
            goto err;
        }
        memset(pDataFlow, 0, uiDataLen);
        uiOff += NS_MSG_DATA_DECODE_DATAFLOW(pDataFlow, uiDataLen, pucMsgBuf + uiOff, uiDataLen);
        pstMsgData = malloc(sizeof(NS_MSG_DATA_ST));
        if (pstMsgData == NULL) {
            ERR_PRINTF("malloc failed.");
            goto err;
        }
        memset(pstMsgData, 0, sizeof(NS_MSG_DATA_ST));
        pstMsgData->uiDataType = uiDataType;
        pstMsgData->uiDataLen  = uiDataLen;
        pstMsgData->pDataFlow  = pDataFlow;
        DCL_AddTail(&(pstMsg->stMsgHead), &(pstMsgData->stNode));

        if (uiOff >= uiTotalLen) {
            break;
        }

        uiDataP -= uiDataLen;
    }
    //loop end

    return pstMsg;
 err:
    MSG_Destroy(pstMsg);
    pstMsg = NULL;

    return NULL;
}

INT Msg_handler(IN INT iConnFd, IN const VOID * pRecvData)
{
    //find link by iConnfd
    
    //decode buffer ==> msg list

    //process msg by type.
    return 0;
}
