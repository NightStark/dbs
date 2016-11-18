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

ULONG MSG_desc_reg(VOID)
{
    MSG_Desc_Init();

    NS_MSG_DESC_REG(MSG_MNG_JOIN_REQ);
    NS_MSG_DESC_REG(MSG_MNG_JOIN_RESP);
    NS_MSG_DESC_REG(MSG_MNG_CONFIRM);
    NS_MSG_DESC_REG(MSG_MNG_OK);
    NS_MSG_DESC_REG(MSG_CTL_ATTACH);

    memset(&g_stMsgTypeDescList, 0, sizeof(TYPE_DESC_LIST_ST));

    int i = 0;
    for (i = 0; i < ARRAY_SIZE(g_aiTypeDescList); i++) {
        if (g_aiTypeDescList[i] == 0xdeadbaef) {
            i++;
            if (i < ARRAY_SIZE(g_aiTypeDescList)) {
                g_stMsgTypeDescList.aiTypeDescMap[g_aiTypeDescList[i]] = i; 
            }
        }
        printf("type=%d\n", g_aiTypeDescList[i]);
    }
    for (i = 0; i < ARRAY_SIZE(g_stMsgTypeDescList.aiTypeDescMap); i++) {
        printf("msg type map index =%d\n", g_stMsgTypeDescList.aiTypeDescMap[i]);
    }

    return ERROR_SUCCESS;
}
