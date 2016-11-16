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

NS_MSG_DATA_ENCODE_TLV(MSG_MNG_JOIN_RESP)
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

NS_MSG_DATA_DECODE_TLV(MSG_MNG_JOIN_RESP)
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

STATIC UINT MSG_DATA_encode_TLV_JoinResp(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
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

STATIC UINT MSG_DATA_decode_TLV_JoinResp(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)
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

STATIC UINT MSG_DATA_encode_TLV_Confirm(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
{
    UINT uiOffset = 0;
    MSG_MNG_CONFIRM_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_CONFIRM_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow   + uiOffset, pstStruct->uiConfirmID);

    return uiOffset;
}

STATIC UINT MSG_DATA_decode_TLV_Confirm(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)
{
    UINT uiOffset = 0;
    MSG_MNG_CONFIRM_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_CONFIRM_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiConfirmID);

    return uiOffset;
}

STATIC UINT MSG_DATA_encode_TLV_OK(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
{
    UINT uiOffset = 0;
    MSG_MNG_OK_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_OK_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow + uiOffset, pstStruct->uiOKID);

    return uiOffset;
}

STATIC UINT MSG_DATA_decode_TLV_OK(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)
{
    UINT uiOffset = 0;
    MSG_MNG_OK_ST *pstStruct = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstStruct= (MSG_MNG_OK_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstStruct->uiOKID);

    return uiOffset;
}

STATIC UINT MSG_DATA_encode_CTL_ATTACH(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
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

STATIC UINT MSG_DATA_decode_CTL_ATTACH(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)
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

ULONG MSG_desc_reg(VOID)
{
    MSG_Desc_Init();

/*
    MSG_Desc_Register(MSG_MNG_JOIN_REQ, 
            sizeof(MSG_MNG_JOIN_REQ_ST), 
            MSG_DATA_encode_TLV_JoinReq, 
            MSG_DATA_decode_TLV_JoinReq); 
            */
    MSG_Desc_Register(MSG_MNG_JOIN_RESP, 
            sizeof(MSG_MNG_JOIN_RESP_ST), 
            MSG_DATA_encode_TLV_JoinResp, 
            MSG_DATA_decode_TLV_JoinResp); 
    MSG_Desc_Register(MSG_MNG_CONFIM, 
            sizeof(MSG_MNG_CONFIRM_ST), 
            MSG_DATA_encode_TLV_Confirm, 
            MSG_DATA_decode_TLV_Confirm); 
    MSG_Desc_Register(MSG_MNG_OK, 
            sizeof(MSG_MNG_OK_ST), 
            MSG_DATA_encode_TLV_OK, 
            MSG_DATA_decode_TLV_OK); 
    MSG_Desc_Register(MSG_CTL_ATTACH, 
            sizeof(MSG_CTL_ATTACH_ST), 
            MSG_DATA_encode_CTL_ATTACH, 
            MSG_DATA_decode_CTL_ATTACH); 

            NS_MSG_DESC_REG(MSG_MNG_JOIN_RESP);

    return ERROR_SUCCESS;
}
