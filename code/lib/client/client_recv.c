#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <asm/errno.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_client.h>
#include <ns_table_type.h>
#include <ns_nsdb.h>
#include <ns_client_fsm.h>
#include <ns_sm.h>
#include <ns_server.h>
#include <ns_msg_client_link.h>

STATIC ULONG MSG_Client_RECV_HandleMngMsg(IN INT iConnFd, IN NS_MSG_ST *pstMsg)
{
    UINT ulRet = 0;
    CLT_SM_ST *pstCltSM = NULL;
    CLT_SM_EVT_EN enCltSMEvt;
    MSG_CLT_LINK_ST *pstMsgCltLink = NULL;

    DBGASSERT(NULL != pstMsg);  
    
    pstMsgCltLink = MSG_client_link_Get();
    if (NULL == pstMsgCltLink) {
        ERR_PRINTF("Can not find link of socked:%d", iConnFd);
    }

    pstCltSM = pstMsgCltLink->pstCltSm;

    switch (pstMsg->usSubType) {
        case MSG_MNG_JOIN_RESP:
            enCltSMEvt = CLT_SM_EVT_RECV_JOIN_RESP;
            break;
        case MSG_MNG_OK:
            enCltSMEvt = CLT_SM_EVT_RECV_OK;
            break;
        default:
            ERR_PRINTF("Invalid Sub msg type!");
    }

    ulRet =  CLT_SM_EVT_HANDLE(pstCltSM, enCltSMEvt, pstMsg);

    return ulRet;
}

STATIC ULONG MSG_Client_RECV_HandleCtlMsg(IN INT iConnFd, IN NS_MSG_ST *pstMsg)
{
    UINT ulRet = 0;
    CLT_SM_ST *pstCltSM = NULL;
    MSG_CLT_LINK_ST *pstMsgCltLink = NULL;
    CLT_SM_STATS_EN enSt;

    DBGASSERT(NULL != pstMsg);  
    
    pstMsgCltLink = MSG_client_link_Get();
    if (NULL == pstMsgCltLink) {
        ERR_PRINTF("Can not find link of socked:%d", iConnFd);
        return ERROR_FAILE;
    }

    pstCltSM = pstMsgCltLink->pstCltSm;

    enSt = CLT_SM_STATUS_GET(pstCltSM); 
    if (enSt != CLT_SM_STATS_RUN) {
        ERR_PRINTF("CLT sm is not RUN, sockfd:%d, now status:%d", iConnFd, enSt);
        return ERROR_FAILE;
    }

    DBGASSERT(iConnFd == pstMsgCltLink->iSockFd);

    switch (pstMsg->usSubType) {
        case MSG_CTL_ATTACH:
            MSG_PRINTF("get ATTACH.");
            MSG_clinet_ctl_recv_attach(pstMsgCltLink, pstMsg);
            break;
        default:
            ERR_PRINTF("Invalid Sub msg type!");
    }

    return ulRet;
}

//TODO:和server合并？
ULONG MSG_Client_RECV_HandleMsg(IN INT iConnFd, IN UCHAR *pauRecvBuf, IN INT iMsgLen)
{

    UINT ulRet = ERROR_FAILE;
    NS_MSG_ST *pstMsg = NULL;	

    pstMsg = NS_MSG_MsgBuf2MsgList(pauRecvBuf, iMsgLen);
    
    //解析数据类型
    switch(pstMsg->usMainType)
    {
        case MSG_MT_MNG:
            ulRet = MSG_Client_RECV_HandleMngMsg(iConnFd, pstMsg);
            break;
        case MSG_MT_CTL:
            ulRet = MSG_Client_RECV_HandleCtlMsg(iConnFd, pstMsg);
            break;
        case MSG_MT_DAT:

            break;
        default :
        ;
    }

    MSG_Destroy(pstMsg);
    pstMsg = NULL;

    return ulRet;
}

INT CLIENT_MSG_recv(MSG_CLT_LINK_ST *pstCltLink, VOID *pRecvBuf, INT iBufLen)
{
	DBGASSERT(NULL != pstCltLink);
	if (pstCltLink->pstSSL) {
		return MSG_SSL_recv(pstCltLink->pstSSL, pRecvBuf, iBufLen);
	} else {
		return MSG_normal_recv(pstCltLink->iSockFd, pRecvBuf, iBufLen);
	}
}



/*
ULONG Client_Recv(IN INT iDataBufLen , IN VOID *pDataBuf)
{
	ULONG ulRet       = 0;	
	UINT  uiRecvMsgLen = 0;

	MSG_MSG_RECV_INFO_S stMsgRecvInfo = {};

	ulRet = MSG_de_DecMsgHead(&stMsgRecvInfo, pDataBuf);

	MSG_PRINTF("client msg recv len : %d", uiRecvMsgLen);
	if  ( uiRecvMsgLen != stMsgRecvInfo.uiMsgLen )
	{
		ERR_PRINTF("Recv msg :%d is Failed!", stMsgRecvInfo.ucMsgType);
	    return ERROR_SUCCESS;
	}
	MSG_PRINTF("Recv msg :%d is Success!", stMsgRecvInfo.ucMsgType);

	ulRet = Client_fsm_msg_Proc(&stMsgRecvInfo);

	return ulRet;
}
*/

