#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>
#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_lilist.h>
#include <ns_log.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_sm.h>

NS_MSG_DATA_ENCODE_TLV(MSG_MNG_JOIN_REQ)
{
    UINT uiOffset = 0;
    MSG_MNG_JOIN_REQ_ST *pstJoinReq = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstJoinReq = (MSG_MNG_JOIN_REQ_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow   + uiOffset, pstJoinReq->uiClientID);
    uiOffset += NS_MSG_DATA_ENCODE_DATAFLOW(pDataFlow + uiOffset, uiDataFlowLen - uiOffset,
                                            pstJoinReq->ucClientMac, sizeof(pstJoinReq->ucClientMac));

    return uiOffset;
}

NS_MSG_DATA_DECODE_TLV(MSG_MNG_JOIN_REQ)
{
    UINT uiOffset = 0;
    MSG_MNG_JOIN_REQ_ST *pstJoinReq = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstJoinReq = (MSG_MNG_JOIN_REQ_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstJoinReq->uiClientID);
    uiOffset += NS_MSG_DATA_DECODE_DATAFLOW(pstJoinReq->ucClientMac, sizeof(pstJoinReq->ucClientMac),
                                            pDataFlow + uiOffset, uiDataFlowLen - uiOffset);

    return uiOffset;
}

NS_MSG_DATA_ENCODE_TLV(MSG_MNG_JOIN_RESP)
{
    UINT uiOffset = 0;
    MSG_MNG_JOIN_RESP_ST *pstJoinResp = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstJoinResp= (MSG_MNG_JOIN_RESP_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow  + uiOffset, pstJoinResp->uiServerID);
    uiOffset += NS_MSG_DATA_ENCODE_ULONG(pDataFlow + uiOffset, pstJoinResp->ulSessionID);

    return uiOffset;
}

NS_MSG_DATA_DECODE_TLV(MSG_MNG_JOIN_RESP)
{
    UINT uiOffset = 0;
    MSG_MNG_JOIN_RESP_ST *pstJoinResp = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstJoinResp= (MSG_MNG_JOIN_RESP_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstJoinResp->uiServerID);
    uiOffset += NS_MSG_DATA_DECODE_ULONG(pDataFlow + uiOffset, &pstJoinResp->ulSessionID);

    return uiOffset;
}

NS_MSG_DATA_ENCODE_TLV(MSG_MNG_CONFIRM)
{
    UINT uiOffset = 0;
    MSG_MNG_CONFIRM_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_CONFIRM_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow   + uiOffset, pstStruct->uiConfirmID);

    return uiOffset;
}

NS_MSG_DATA_DECODE_TLV(MSG_MNG_CONFIRM)
{
    UINT uiOffset = 0;
    MSG_MNG_CONFIRM_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_CONFIRM_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiConfirmID);

    return uiOffset;
}

NS_MSG_DATA_ENCODE_TLV(MSG_MNG_OK)
{
    UINT uiOffset = 0;
    MSG_MNG_OK_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_OK_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, pstStruct->uiOKID);

    return uiOffset;
}

NS_MSG_DATA_DECODE_TLV(MSG_MNG_OK)
{
    UINT uiOffset = 0;
    MSG_MNG_OK_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_OK_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiOKID);

    return uiOffset;
}

NS_MSG_DATA_ENCODE_TLV(MSG_CTL_ATTACH)
{
    UINT uiOffset = 0;
    MSG_CTL_ATTACH_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_CTL_ATTACH_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, pstStruct->uiAttachMode);
    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, pstStruct->uiCmdVer);
    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, pstStruct->uiCmdID);

    return uiOffset;
}

NS_MSG_DATA_DECODE_TLV(MSG_CTL_ATTACH)
{
    UINT uiOffset = 0;
    MSG_CTL_ATTACH_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_CTL_ATTACH_ST*)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiAttachMode);
    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiCmdVer);
    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiCmdID);

    return uiOffset;
}


typedef struct tag_msg_type_desc
{
    INT type_type[128];
    INT iMsgType;
}MSG_TYPE_DESC_ST;

STATIC INT g_aiTypeDescList[] = 
{
#include <ns_msg_type_desc.h>
};

typedef struct tag_typeDescList
{
    INT *piTypeDescList;
    INT  aiTypeDescMap[_MSG_SUB_MAX_];
}TYPE_DESC_LIST_ST;

STATIC TYPE_DESC_LIST_ST g_stMsgTypeDescList;

UINT MSG_DATA_encode_TLV_normal(IN VOID *pStruct, OUT VOID *pDataFlow, UINT uiDataFlowLen, INT iMsgType)
{
    CHAR *pST = NULL;
    INT  iDesc = NULL;
    UINT uiOffset = 0;
    UINT uiStOffset = 0;
    UINT uiArrayLen = 0;
    ANY_TYPE_EN enType = 0;

    pST = (CHAR *)pStruct;

    MSG_PRINTF("MsgType = %d", iMsgType);
    iDesc = g_stMsgTypeDescList.aiTypeDescMap[iMsgType];
    MSG_PRINTF("iDesc = %d", iDesc);
    /* make sure not overrun all type list */
    while (iDesc < ARRAY_SIZE(g_aiTypeDescList)) {
        iDesc ++;

        /* arrive next type, this type is over */
        if (g_aiTypeDescList[iDesc] == 0xdeadbaef) {
            MSG_PRINTF("desc over %d", iDesc);
            break;
        }

        enType = g_aiTypeDescList[iDesc] & 0xFFFF;
        uiArrayLen = (g_aiTypeDescList[iDesc] & 0xFFFF0000) >> 16;
        MSG_PRINTF("enType = %d", enType);
        MSG_PRINTF("uiArrayLen = %d", uiArrayLen);
        switch (enType)
        {
            case AT_CHAR:
            case AT_UCHAR:
                uiOffset += NS_MSG_DATA_ENCODE_DATAFLOW(pDataFlow + uiOffset, 
                        uiDataFlowLen - uiOffset, 
                        (UCHAR *)(pST + uiStOffset), 
                        uiArrayLen);
                break;
            case AT_SZ:
                ERR_PRINTF("not support type now.");
            case AT_SHORT:
            case AT_USHORT:
                uiOffset += NS_MSG_DATA_ENCODE_USHORT(pDataFlow + uiOffset, *((USHORT*)(pST + uiStOffset)));
                uiStOffset += sizeof(USHORT);
                break;
            case AT_INT:
            case AT_UINT:
                uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, *((UINT*)(pST + uiStOffset)));
                uiStOffset += sizeof(UINT);
                break;
            case AT_LONG:
            case AT_ULONG:
                uiOffset += NS_MSG_DATA_ENCODE_ULONG(pDataFlow + uiOffset, *((ULONG*)(pST + uiStOffset)));
                uiStOffset += sizeof(ULONG);
                break;
            case AT_LLONG:
            case AT_ULLONG:
            case AT_FLOAT:
            case AT_DOUBLE:
            case AT_BOOL_T:
            case AT_NONE:
                ERR_PRINTF("not support type now.");
                break;
            default:
                ERR_PRINTF("invalid type.");
                ;
        }
    }

    return uiOffset;
}

UINT MSG_DATA_decode_TLV_normal(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct, INT iMsgType)
{
    CHAR *pST = NULL;
    INT  iDesc = NULL;
    UINT uiOffset = 0;
    UINT uiStOffset = 0;
    UINT uiArrayLen = 0;
    ANY_TYPE_EN enType = 0;

    pST = (CHAR *)pStruct;

    iDesc = g_stMsgTypeDescList.aiTypeDescMap[iMsgType];
    /* make sure not overrun all type list */
    while (iDesc < ARRAY_SIZE(g_aiTypeDescList)) {
        iDesc ++;

        /* arrive next type, this type is over */
        if (g_aiTypeDescList[iDesc] == 0xdeadbaef) {
            break;
        }

        enType = g_aiTypeDescList[iDesc] & 0xFFFF;
        uiArrayLen = (g_aiTypeDescList[iDesc] & 0xFFFF0000) >> 16;
        switch (enType)
        {
            case AT_CHAR:
            case AT_UCHAR:
                uiOffset += NS_MSG_DATA_DECODE_DATAFLOW((UCHAR *)(pST + uiStOffset), 
                        uiArrayLen,
                        (UCHAR *)(pDataFlow + uiOffset), 
                        uiDataFlowLen - uiOffset);
                break;
            case AT_SZ:
                ERR_PRINTF("not support type now.");
            case AT_SHORT:
            case AT_USHORT:
                uiOffset += NS_MSG_DATA_DECODE_USHORT(pDataFlow + uiOffset, (USHORT*)(pST + uiStOffset));
                uiStOffset += sizeof(USHORT);
                break;
            case AT_INT:
            case AT_UINT:
                uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, (UINT *)(pST + uiStOffset));
                uiStOffset += sizeof(UINT);
                break;
            case AT_LONG:
            case AT_ULONG:
                uiOffset += NS_MSG_DATA_DECODE_ULONG(pDataFlow + uiOffset, (ULONG*)(pST + uiStOffset));
                uiStOffset += sizeof(ULONG);
                break;
            case AT_LLONG:
            case AT_ULLONG:
            case AT_FLOAT:
            case AT_DOUBLE:
            case AT_BOOL_T:
            case AT_NONE:
                ERR_PRINTF("not support type now.");
                break;
            default:
                ERR_PRINTF("invalid type.");
        }
    }

    return uiOffset;
}

ULONG MSG_desc_reg(VOID)
{
    int i = 0;
    MSG_Desc_Init();

    NS_MSG_DESC_REG(MSG_MNG_JOIN_REQ);
    NS_MSG_DESC_REG(MSG_MNG_JOIN_RESP);
    NS_MSG_DESC_REG(MSG_MNG_CONFIRM);
    NS_MSG_DESC_REG(MSG_MNG_OK);
    NS_MSG_DESC_REG(MSG_CTL_ATTACH);
    NS_MSG_DESC_REG(MSG_CTL_ATTACH_RESP);

    memset(&g_stMsgTypeDescList, 0, sizeof(TYPE_DESC_LIST_ST));

    /* map list type */
    for (i = 0; i < ARRAY_SIZE(g_aiTypeDescList); i++) {
        if (g_aiTypeDescList[i] == 0xdeadbaef) {
            i++;
            if (i < ARRAY_SIZE(g_aiTypeDescList)) {
                g_stMsgTypeDescList.aiTypeDescMap[g_aiTypeDescList[i]] = i; 
            }
        }
        MSG_PRINTF("type=%d\n", g_aiTypeDescList[i]);
    }

    for (i = 0; i < ARRAY_SIZE(g_stMsgTypeDescList.aiTypeDescMap); i++) {
        MSG_PRINTF("msg type map index =%d\n", g_stMsgTypeDescList.aiTypeDescMap[i]);
    }

    return ERROR_SUCCESS;
}
