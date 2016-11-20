#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>

#include <ns_net.h>
#include <ns_table.h>
#include <ns_sm.h>
#include <ns_msg.h>
#include <ns_msg_server_link.h>
#include <ns_server.h>
#include <version.h>

STATIC ULONG MSG_server_ctl_recv_attachReap (MSG_SRV_LINK_ST *pstSrvLink, VOID *pMsg)
{
    INT   iSockFd       = -1;
    ULONG ulRet         = ERROR_FAILE;
    NS_MSG_ST *pstMsg   = NULL;
    MSG_CTL_ATTACH_RESP_ST stCtlAttachResp;
    UCHAR ucMsgBuf[1024]  = {0};
    NS_MSG_ST *pstRespMsg = NULL;

    DBGASSERT(NULL != pMsg);

    pstMsg = (NS_MSG_ST *)pMsg;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stCtlAttachResp, sizeof(stCtlAttachResp));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Attach resp status = %d", stCtlAttachResp.uiAttachStatus);

    return ERROR_SUCCESS;
}

ULONG MSG_RECV_HandleCtlMsg(IN INT iConnFd, IN VOID *pMsg)
{

    UINT ulRet                  = 0;
    SRV_SM_ST *pstSrvSM         = NULL;
    NS_MSG_ST *pstMsg           = (NS_MSG_ST*)pMsg;
    SRV_SM_STATS_EN enSt        = 0;
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    DBGASSERT(NULL != pstMsg);  
    
    pstSrvLink = MSG_server_link_FindBySockFd(iConnFd);
    if (NULL == pstSrvLink) {
        ERR_PRINTF("Can not find link by socked:%d", iConnFd);
        return ERROR_FAILE;
    }

    pstSrvSM = pstSrvLink->pstSrvSm;
    enSt = SRV_SM_STATUS_GET(pstSrvSM);
    if (enSt != SRV_SM_STATS_RUN) {
        ERR_PRINTF("Srv sm is not RUN, sockfd:%d, now status:%d", iConnFd, enSt);
        return ERROR_FAILE;
    }

    switch (pstMsg->usSubType) {
        case MSG_CTL_ATTACH_RESP:
            ERR_PRINTF("get attach response.");
            MSG_server_ctl_recv_attachReap(pstSrvLink, pMsg);
            break;
        default:
            ERR_PRINTF("Invalid Sub ctl msg type!");
    }

    return ulRet;
}

ULONG MSG_server_ctl_send_attach(MSG_SRV_LINK_ST *pstSrvLink)
{
    ULONG           ulRet       = ERROR_FAILE;
    NS_MSG_ST       *pstMsg     = NULL;

    DBGASSERT(pstSrvLink != NULL);

    //Send OK Response.
    MSG_CTL_ATTACH_ST stCtlAttch;
    UCHAR ucMsgBuf[1024] = {0};
    UINT  uiLen = sizeof(ucMsgBuf);
    INT iSockFd = -1;

    //iSockFd = MSG_server_GetSockFdBySm(pstSrvSM);
    iSockFd = pstSrvLink->iSockFd;
    if (iSockFd == -1) {
        ERR_PRINTF("invalid socket fd");
        return ERROR_FAILE;
    }

    MSG_PRINTF("clent sock fd:%d", iSockFd);

    pstMsg = MSG_Create(MSG_MT_CTL, MSG_CTL_ATTACH);
    if (NULL == pstMsg) {
        ERR_PRINTF("create msg failed");
        ulRet = ERROR_FAILE;
    }

    memset(&stCtlAttch, 0, sizeof(stCtlAttch));
    stCtlAttch.uiAttachMode = 0;
    stCtlAttch.uiFwVer      = NS_CLIENT_VER + 1;
    stCtlAttch.uiCmdVer     = 0;
    stCtlAttch.uiCmdID      = 0;

    MSG_AddData(pstMsg, MSG_CTL_ATTACH, &stCtlAttch, sizeof(stCtlAttch));

    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    ulRet = SERVER_MSG_send(pstSrvLink, ucMsgBuf, uiLen);

    MSG_Destroy(pstMsg);
    pstMsg = NULL;

    return ulRet;
}
