#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_log.h>

typedef struct tag_MsgDataTest
{
    UINT  uiNum;
    CHAR ucName[32];  
    UINT  uiSize;
}MSG_DATA_TEST_ST;

STATIC UINT MSG_DATA_encode_TLV_Test(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
{
    UINT uiOffset = 0;
    MSG_DATA_TEST_ST *pstMDTest = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstMDTest = (MSG_DATA_TEST_ST *)pStruct;

    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow   + uiOffset, pstMDTest->uiNum);
    uiOffset += NS_MSG_DATA_ENCODE_STRING(pDataFlow + uiOffset, uiDataFlowLen - uiOffset, pstMDTest->ucName, strlen((CHAR *)pstMDTest->ucName));
    uiOffset += NS_MSG_DATA_ENCODE_UINT(pDataFlow   + uiOffset, pstMDTest->uiSize);

    return uiOffset;
}

STATIC UINT MSG_DATA_decode_TLV_Test(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)
{
    UINT uiOffset = 0;
    MSG_DATA_TEST_ST *pstMDTest = NULL;

    DBGASSERT(NULL != pStruct);
    DBGASSERT(NULL != pDataFlow);
    
    pstMDTest = (MSG_DATA_TEST_ST *)pStruct;

    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstMDTest->uiNum);
    uiOffset += NS_MSG_DATA_DECODE_STRING(pstMDTest->ucName, 
            sizeof(pstMDTest->ucName), 
            pDataFlow + uiOffset, 
            strlen((CHAR *)(pDataFlow + uiOffset))/* is not a good idea*/);
    uiOffset += NS_MSG_DATA_DECODE_UINT(pDataFlow + uiOffset, &pstMDTest->uiSize);

    return uiOffset;
}

INT msg_desc_test(void);

int msg_en_de_test2(void)
{
    NS_MSG_ST *pstMsg = NULL;	
    MSG_DATA_TEST_ST stDataTest;
	
    /* creat a msg */
    pstMsg = MSG_Create(1, 2);
    if (NULL == pstMsg) {
        ERR_PRINTF("MSG Create failed.");
    }
    memset(&stDataTest, 0, sizeof(MSG_DATA_TEST_ST));
    stDataTest.uiNum = 0x38;
    stDataTest.uiSize = 0xEA;
    snprintf(stDataTest.ucName, sizeof(stDataTest.ucName), "%s", "TestDataMsg");

    MSG_AddData(pstMsg, 0x12, &stDataTest, sizeof(stDataTest));
    MSG_AddData(pstMsg, 0x88, &stDataTest, sizeof(stDataTest));
    MSG_AddData(pstMsg, 0x33, &stDataTest, sizeof(stDataTest));
   
    UCHAR ucMsgBuf[1024] = {0};
    UINT uiLen = sizeof(ucMsgBuf);
    
    /* 吧消息链表转换长字符流，这个字符流可以被发送，或者加密什么的 */
    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    /* 消息发送完自后，消息链表就可以释放了 */
    MSG_Destroy(pstMsg);

    Dbg_Print_Log("Msg.dat", (char *)ucMsgBuf, uiLen);
    pstMsg = NULL;

	MSG_PRINTF("create buffer over.");

    /* 将接收的字符流转换为消息链表 */
    pstMsg = NS_MSG_MsgBuf2MsgList(ucMsgBuf, uiLen);

    MSG_GetData(pstMsg, 0x88, &stDataTest, sizeof(stDataTest));

	MSG_PRINTF("num:[0x%X].", stDataTest.uiNum);
	MSG_PRINTF("name:[%s].", stDataTest.ucName);
	MSG_PRINTF("size:[0x%X].", stDataTest.uiSize);


    MSG_Destroy(pstMsg);

	return 0;
}

int msg_en_de_test_main(void)
{
#if 0
    NS_MSG_ST *pstMsg = NULL;	
    NS_MSG_DATA_DESC_ST stDataDesc;
    MSG_DATA_TEST_ST stDataTest;
	
    /* creat a msg */
    pstMsg = MSG_Create(1, 2);
    if (NULL == pstMsg) {
        ERR_PRINTF("MSG Create failed.");
    }
    memset(&stDataDesc, 0, sizeof(NS_MSG_DATA_DESC_ST));
    memset(&stDataTest, 0, sizeof(MSG_DATA_TEST_ST));
    stDataTest.uiNum = 0x38;
    stDataTest.uiSize = 0xEA;
    snprintf(stDataTest.ucName, sizeof(stDataTest.ucName), "%s", "TestDataMsg");

    stDataDesc.uiDataType = 0x12;
    stDataDesc.uiDataLen  = sizeof(stDataTest);
    stDataDesc.pDataStruct = (VOID *)(&stDataTest);
    stDataDesc.pfEncodeTLV = MSG_DATA_encode_TLV_Test;

    _MSG_AddData(pstMsg, &stDataDesc);
    stDataDesc.uiDataType = 0x18;
    _MSG_AddData(pstMsg, &stDataDesc);
    stDataDesc.uiDataType = 0x33;
    _MSG_AddData(pstMsg, &stDataDesc);
   
    UCHAR ucMsgBuf[1024] = {0};
    UINT uiLen = sizeof(ucMsgBuf);
    
    /* 吧消息链表转换长字符流，这个字符流可以被发送，或者加密什么的 */
    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    /* 消息发送完自后，消息链表就可以释放了 */
    MSG_Destroy(pstMsg);

    Dbg_Print_Log("Msg.dat", (char *)ucMsgBuf, uiLen);
    pstMsg = NULL;

	MSG_PRINTF("create buffer over.");

    /* 将接收的字符流转换为消息链表 */
    pstMsg = NS_MSG_MsgBuf2MsgList(ucMsgBuf, uiLen);

    memset(&stDataDesc, 0, sizeof(NS_MSG_DATA_DESC_ST));
    memset(&stDataTest, 0, sizeof(MSG_DATA_TEST_ST));

    stDataDesc.uiDataType = 0x12;
    stDataDesc.uiDataLen  = sizeof(stDataTest);
    stDataDesc.pDataStruct = (VOID *)(&stDataTest);
    stDataDesc.pfEncodeTLV = MSG_DATA_encode_TLV_Test;
    stDataDesc.pfDecodeTVL = MSG_DATA_decode_TLV_Test;

    MSG_GetData(pstMsg, &stDataDesc);
	MSG_PRINTF("num:[0x%X].", stDataTest.uiNum);
	MSG_PRINTF("name:[%s].", stDataTest.ucName);
	MSG_PRINTF("size:[0x%X].", stDataTest.uiSize);


    MSG_Destroy(pstMsg);

#endif
    msg_desc_test();
    msg_en_de_test2();
    MSG_Desc_UnRegisterAll();

	return 0;
}

INT msg_desc_test(void)
{
    MSG_Desc_Init();

    MSG_Desc_Register(0x33, 
            sizeof(MSG_DATA_TEST_ST), 
            MSG_DATA_encode_TLV_Test, 
            MSG_DATA_decode_TLV_Test); 
    MSG_Desc_Register(0x12, 
            sizeof(MSG_DATA_TEST_ST), 
            MSG_DATA_encode_TLV_Test, 
            MSG_DATA_decode_TLV_Test); 
    MSG_Desc_Register(0x88, 
            sizeof(MSG_DATA_TEST_ST), 
            MSG_DATA_encode_TLV_Test, 
            MSG_DATA_decode_TLV_Test); 

    return 0;
}
