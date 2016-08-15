#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>

//#include "../include/msg.h"
/*
[   T    |   L    |    V                                                                         |
					 MT-------------C----------------N-----------------T----------------L-----------V
| 总类型 | 总长度 | 消息数据类型 | 有效数据条数 |   数据类型名列表   | 数据类型表(字节) | 长度表(UINT) | 数据区 | 
  1Byte      UINT       UINT          UINT-n     STRING[n]('\0'结尾)      UCHAR[n]         ULONG[n]     [data]n
*/

UINT MSG_ec_EncData(UINT uiDataType,
						  VOID *pDataStruct,
						  VOID *pDataBuf)
{
	CHAR *pcDataBuf = NULL;
	UINT  uiDataBufPoint = 0;

	DBGASSERT(NULL != pDataStruct);	
	DBGASSERT(NULL != pDataBuf);

	pcDataBuf = (CHAR *)pDataBuf;

	/* 封数据类型 */
	uiDataBufPoint += MSG_dataenc_Uint(uiDataType, pcDataBuf);

	/* 封数据 */
	uiDataBufPoint += MSG_datadenc_DataStruct(uiDataType, 
 										  pDataStruct, 
 										  (CHAR *)pDataBuf + uiDataBufPoint);

	return uiDataBufPoint;
}

UINT MSG_ec_EncMsg(UCHAR ucMsgType, 
					UINT uiDataType,
					VOID *pSendData,
					UINT  uiSendDataLen,
					VOID *pDataBuf)
{
	CHAR  *pcDataBuf    	= NULL;
	//UCHAR *pucMsgType 		= NULL;
	UINT  *puiMsgTotelLen 	= 0;
	//CHAR  *pucDataStart 	= NULL;

	DBGASSERT(NULL != pSendData);
	DBGASSERT(NULL != pDataBuf);

	pcDataBuf = (CHAR *)pDataBuf;
	//pucMsgType = (UCHAR *)pcDataBuf;
	puiMsgTotelLen = (UINT *)(pcDataBuf + sizeof(UCHAR));
	//pucDataStart   = pcDataBuf + sizeof(UINT);

	//*pucMsgType = ucMsgType;
	(*puiMsgTotelLen) += (sizeof(UCHAR) + sizeof(UINT));

	//memcpy(pucDataStart, pSendData, uiSendDataLen);
	//*puiMsgTotelLen += uiSendDataLen;
	(*puiMsgTotelLen) += MSG_ec_EncData(uiDataType, 
										pSendData, 
										pDataBuf + *puiMsgTotelLen);


	return *puiMsgTotelLen;
}

/*
 * |main type|sub type|total len|seq     |T.L.V|T.L.V.|T.L.V|
 * |   16bit | 16bi   | 32bit   |32bit   |32bit.32bit.xbit|
 * total len = main type + sub type + total len + date len
 * L = T + L + V
 *
 * */



STATIC UINT msg_CreateSeq(void)
{
    STATIC UINT uiSeq = 0x10001000;
    return uiSeq++;
}

/*
 * create a msg
 *   this struct will fill all the msg want to send in a this frame.
 * @usMainType : main type of the frame
 * @usSubType  : sub  type of the frame
 * */
NS_MSG_ST * MSG_Create (IN USHORT usMainType,
                        IN USHORT usSubType)
{
    NS_MSG_ST *pstMsg = NULL;

    pstMsg = (NS_MSG_ST *)malloc(sizeof(NS_MSG_ST));
    if (NULL == pstMsg) {
        return NULL;
    }
    memset(pstMsg, 0, sizeof(NS_MSG_ST));
    pstMsg->usMainType = usMainType;
    pstMsg->usSubType  = usSubType;
    pstMsg->uiTotalLen = NS_MSG_HEADER_LEN;
    pstMsg->uiSeq      = msg_CreateSeq();
    DCL_Init(&pstMsg->stMsgHead);

    return pstMsg;
}

VOID MSG_Destroy (NS_MSG_ST *pstMsg)
{
    NS_MSG_DATA_ST *pstMsgData = NULL;
    NS_MSG_DATA_ST *pstMsgDataNext = NULL;

    /* free all data */
    DCL_FOREACH_ENTRY_SAFE(&(pstMsg->stMsgHead), pstMsgData, pstMsgDataNext, stNode) {
        MSG_PRINTF("======");
        if (NULL != pstMsgData->pDataFlow) {
            MSG_PRINTF("======");
            free(pstMsgData->pDataFlow);
            pstMsgData->pDataFlow = NULL;
        }
        free(pstMsgData);
        pstMsgData = NULL;
    }

    if (NULL != pstMsg) {
        free(pstMsg);
        pstMsg = NULL;
    }
}

/* add a msg to msglist */
//TODO: may be can use a ring to hold all NS_MSG_DATA_ST fo reduce alloc times.
STATIC ULONG _MSG_AddData (IN NS_MSG_ST *pstNsMsg, IN NS_MSG_DATA_DESC_ST *pstMsgDesc)
{
    UINT uiRet = 0;
    VOID *pData = NULL;
    NS_MSG_DATA_ST *pstMsgData = NULL; 

    if (NULL == pstNsMsg) {
        ERR_PRINTF("why give a null point.");
        return ERROR_FAILE;
    }

    if (NULL == pstMsgDesc) {
        ERR_PRINTF("why give a null point.");
        return ERROR_FAILE;
    }

    pstMsgData = (NS_MSG_DATA_ST *)malloc(sizeof(NS_MSG_DATA_ST));
    if (NULL == pstMsgData) {
        ERR_PRINTF("malloc failed.");
        return ERROR_FAILE;
    }
    memset(pstMsgData, 0, sizeof(NS_MSG_DATA_ST));

    /* no need to trans one msg for evey time ,make sure one struct for one msg*/
    //if (NULL == pstMsgDesc->pDataFlow) {
        UINT uiDataFlowLen = pstMsgDesc->uiDataLen + 128; //why 128:  by my self
        pstMsgDesc->pDataFlow = malloc(uiDataFlowLen);
        if (NULL == pstMsgDesc->pDataFlow) {
            ERR_PRINTF("malloc failed.");
            goto err_msd; 
        }
        uiRet = pstMsgDesc->pfEncodeTLV(pstMsgDesc->pDataStruct, pstMsgDesc->pDataFlow, uiDataFlowLen); 
        if (uiRet <= 0) {
            ERR_PRINTF("encode msg for msg %d failed.", pstMsgDesc->uiDataType);
            goto err_msd; 
        }
    //}

    pstMsgData->uiDataLen  = uiRet; 
    pstMsgData->uiDataType = pstMsgDesc->uiDataType;
    pData = malloc(uiRet);
    if (pData == NULL) {
        goto err_msd;
    }
    memset(pData, 0, uiRet);
    memcpy(pData, pstMsgDesc->pDataFlow, uiRet);
    pstMsgData->pDataFlow = pData;

    DCL_AddTail(&(pstNsMsg->stMsgHead), &(pstMsgData->stNode));

    pstNsMsg->uiTotalLen += NS_MSG_DATA_TYPE_LEN + NS_MSG_DATA_LEN_LEN + uiRet;

    //TODO: muti malloc is not good!!
    free(pstMsgDesc->pDataFlow);
    pstMsgDesc->pDataFlow = NULL;

    return ERROR_SUCCESS;

err_msd:
    if (NULL != pstMsgData) {
        free(pstMsgData);
        pstMsgData = NULL;
    }

    if (NULL != pData) {
        free(pData);
        pData = NULL;
    }

    return ERROR_FAILE;
}

ULONG MSG_AddData (IN NS_MSG_ST *pstMsg,
                   IN UINT uiMsgType, 
                   INOUT VOID *pDataBuf, 
                   IN UINT uiBufLen)
{
    NS_MSG_DATA_DESC_ST *pstMsgDD = NULL;

    DBGASSERT(NULL != pDataBuf);
    DBGASSERT(0    != uiBufLen);

    pstMsgDD = MSG_Desc_Get(uiMsgType);
    if (pstMsgDD == NULL) {
        ERR_PRINTF("invalid msg type[%d]", uiMsgType);
        return ERROR_FAILE;
    }

    if (uiBufLen < pstMsgDD->uiDataLen) {
        ERR_PRINTF("buffer is too small, need:[%d], but:[%d]", pstMsgDD->uiDataLen, uiBufLen);
        return ERROR_FAILE;
    }

    pstMsgDD->pDataStruct = pDataBuf;

    _MSG_AddData(pstMsg, pstMsgDD);

	return ERROR_SUCCESS;
}

/* get a data(TLV) from msglist */
ULONG _MSG_GetData (IN NS_MSG_ST *pstNsMsg, IN NS_MSG_DATA_DESC_ST *pstMsgDesc)
{
    NS_MSG_DATA_ST *pstMsgData = NULL;

    if (NULL == pstNsMsg) {
        ERR_PRINTF("why give a null point.");
        return ERROR_FAILE;
    }

    if (NULL == pstMsgDesc) {
        ERR_PRINTF("why give a null point.");
        return ERROR_FAILE;
    }

    DCL_FOREACH_ENTRY(&(pstNsMsg->stMsgHead), pstMsgData, stNode) {
        if (pstMsgData->uiDataType == pstMsgDesc->uiDataType) {
            //pstMsgDesc->pDataFlow;
            pstMsgDesc->uiDataLen = pstMsgData->uiDataLen;
            pstMsgDesc->pfDecodeTVL(pstMsgData->pDataFlow, pstMsgData->uiDataLen, pstMsgDesc->pDataStruct);
            //TODO:TBD: will del and free .?
            
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FAILE;
}

ULONG MSG_GetData (IN NS_MSG_ST *pstMsg,
                   IN UINT uiMsgType, 
                   INOUT VOID *pDataBuf, 
                   IN UINT uiBufLen)
{
    NS_MSG_DATA_DESC_ST *pstMsgDD = NULL;

    DBGASSERT(NULL != pDataBuf);
    DBGASSERT(0    != uiBufLen);

    pstMsgDD = MSG_Desc_Get(uiMsgType);
    if (pstMsgDD == NULL) {
        ERR_PRINTF("invalid msg type[%d]", uiMsgType);
        return ERROR_FAILE;
    }

    if (uiBufLen < pstMsgDD->uiDataLen) {
        ERR_PRINTF("buffer is too small, need:[%d], but:[%d]", pstMsgDD->uiDataLen, uiBufLen);
        return ERROR_FAILE;
    }

    pstMsgDD->pDataStruct = pDataBuf;

    _MSG_GetData(pstMsg, pstMsgDD);

	return ERROR_SUCCESS;
}

VOID MSG_ShowData (NS_MSG_ST *pstMsg)
{
    NS_MSG_DATA_ST *pstMsgData = NULL;
    NS_MSG_DATA_ST *pstMsgDataNext = NULL;

    /* free all data */
    DCL_FOREACH_ENTRY_SAFE(&(pstMsg->stMsgHead), pstMsgData, pstMsgDataNext, stNode) {
        if (NULL != pstMsgData->pDataFlow) {
            MSG_PRINTF("======");
        }
    }
}

/*
 *    
 *  msg list transform to a buffer stream
 * */
ULONG NS_MSG_MsgList2MsgBuf (IN NS_MSG_ST *pstNsMsg, IN UCHAR *pucMsgBuf, INOUT UINT *puiLen)
{
    UINT   uiRet    = 0;
    UINT   uiBufOff = 0;
    UINT   uiBufLen = 0; /* all data len */
    UINT   uiRoomLen = 0; /* length of a buffer to fill msg stram */
    UCHAR *pucMsgFlow = NULL;
    NS_MSG_DATA_ST *pstMsgData = NULL;

    uiRoomLen = *puiLen;

    uiBufLen = pstNsMsg->uiTotalLen;
    if (uiBufLen < NS_MSG_HEADER_LEN) {
        ERR_PRINTF("msg is too short.");
        return ERROR_FAILE;
    }

    if (uiBufLen > uiRoomLen) {
        ERR_PRINTF("buffer is too short.");
        return ERROR_FAILE;
    }

    //pucMsgFlow = (UCHAR *)malloc(uiBufLen);
    pucMsgFlow = pucMsgBuf; 
    if (NULL == pucMsgFlow) {
        ERR_PRINTF("null point");
        return ERROR_FAILE;
    }
    memset(pucMsgFlow, 0, uiBufLen);

    /* fill msg header  */
    uiBufOff += NS_MSG_DATA_ENCODE_USHORT(pucMsgFlow + uiBufOff, pstNsMsg->usMainType);
    uiBufOff += NS_MSG_DATA_ENCODE_USHORT(pucMsgFlow + uiBufOff, pstNsMsg->usSubType);
    uiBufOff += NS_MSG_DATA_ENCODE_UINT(pucMsgFlow   + uiBufOff, pstNsMsg->uiTotalLen);
    uiBufOff += NS_MSG_DATA_ENCODE_UINT(pucMsgFlow   + uiBufOff, pstNsMsg->uiSeq);

    DCL_FOREACH_ENTRY(&(pstNsMsg->stMsgHead), pstMsgData, stNode) {
        if ((uiRoomLen - uiBufOff) < (sizeof(UINT) + sizeof(UINT))) {
            ERR_PRINTF("room is not enougth.");
            goto err_ml2mb;
        }
        uiBufOff += NS_MSG_DATA_ENCODE_UINT(pucMsgFlow + uiBufOff, pstMsgData->uiDataType);
        uiBufOff += NS_MSG_DATA_ENCODE_UINT(pucMsgFlow + uiBufOff, pstMsgData->uiDataLen);
        uiRet = NS_MSG_DATA_ENCODE_DATAFLOW(pucMsgFlow + uiBufOff, 
                                                uiRoomLen - uiBufOff,
                                                pstMsgData->pDataFlow,
                                                pstMsgData->uiDataLen);
        if (uiRet < 0) {
            goto err_ml2mb;
        }
        uiBufOff += uiRet;
    }


//    *ppucMsgBuf = pucMsgFlow;
    *puiLen = uiBufOff;

    return ERROR_SUCCESS; 

err_ml2mb:
/*
    if (NULL != pucMsgFlow) {
        free(pucMsgFlow);
        pucMsgFlow = NULL;
    }
    */
    return ERROR_FAILE;
}

