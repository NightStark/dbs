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

typedef struct tag_msg_desc_info
{
    DCL_NODE_S stNode; 
    
    NS_MSG_DATA_DESC_ST stMsgDataDesc;
}MSG_DESC_INFO_ST;

/* 链表，用来存放消息描述，便于封装操作  */
DCL_HEAD_S g_msg_desc_list_head;

INT MSG_Desc_Init(VOID)
{
    DCL_Init(&g_msg_desc_list_head);
    return 0;
}

VOID MSG_Desc_Fini(VOID)
{
    DCL_Fint(&g_msg_desc_list_head);
    return;
}

NS_MSG_DATA_DESC_ST * MSG_Desc_Register(IN UINT uiDType, 
                                        IN UINT uiDLen,
                                        pf_encode_TLV pfEnTLV,
                                        pf_decode_TLV pfDeTVL)
{
    MSG_DESC_INFO_ST *pstDDInfo   = NULL;
    NS_MSG_DATA_DESC_ST *pstMsgDD = NULL; 

    pstMsgDD = MSG_Desc_Get(uiDType);
    if (pstMsgDD != NULL) {
        ERR_PRINTF("msg desc type[0x%X] is exsited.", uiDType);
        return NULL;
    }

    pstDDInfo = malloc(sizeof(MSG_DESC_INFO_ST));
    if (pstDDInfo == NULL) {
        ERR_PRINTF("malloc failed");
        return NULL;
    }
    memset(pstDDInfo, 0, sizeof(MSG_DESC_INFO_ST));

    pstMsgDD = &(pstDDInfo->stMsgDataDesc);
    pstMsgDD->uiDataType  = uiDType;
    pstMsgDD->uiDataLen   = uiDLen;
    pstMsgDD->pfEncodeTLV = pfEnTLV;
    pstMsgDD->pfDecodeTVL = pfDeTVL;

    DCL_AddTail(&g_msg_desc_list_head, &(pstDDInfo->stNode));

    return pstMsgDD;
}

NS_MSG_DATA_DESC_ST * MSG_Desc_Get(IN UINT uiDType)
{
    MSG_DESC_INFO_ST *pstDDInfo   = NULL;

    DCL_FOREACH_ENTRY(&g_msg_desc_list_head, pstDDInfo, stNode) {
        if (pstDDInfo->stMsgDataDesc.uiDataType == uiDType) {
            return &(pstDDInfo->stMsgDataDesc);
        }
    }

    return NULL;
}

VOID MSG_Desc_UnRegisterAll(VOID)
{
    MSG_DESC_INFO_ST *pstDDInfo = NULL;
    MSG_DESC_INFO_ST *pstNext   = NULL;

    DCL_FOREACH_ENTRY_SAFE(&g_msg_desc_list_head, pstDDInfo, pstNext, stNode) {
        DCL_Del(&(pstDDInfo->stNode));
        free(pstDDInfo);
    }

    return;
}
