#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
#include <ns_config.h>
#include <ns_task.h>

ULONG MSG_clinet_ctl_recv_attach (MSG_CLT_LINK_ST *pstCltLink, VOID *pMsg)
{
    INT   iSockFd       = -1;
    UINT  uiClientFwVer = 0;
    ULONG ulRet         = ERROR_FAILE;
    UINT  uiLen         = 0;
    NS_MSG_ST *pstMsg   = NULL;
    MSG_CTL_ATTACH_ST      stCtlAttach;
    MSG_CTL_ATTACH_RESP_ST stCtlAttachResp;
    UCHAR ucMsgBuf[1024]  = {0};
    NS_MSG_ST *pstRespMsg = NULL;

    DBGASSERT(NULL != pMsg);

    pstMsg = (NS_MSG_ST *)pMsg;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stCtlAttach, sizeof(stCtlAttach));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    uiClientFwVer = cfg_get_client_fw_ver();
    MSG_PRINTF("Attach Mode = %d", stCtlAttach.uiAttachMode);
    MSG_PRINTF("Attach FW version  = %d", stCtlAttach.uiFwVer);
    MSG_PRINTF("now FW version  = %d", uiClientFwVer);
    if (uiClientFwVer != stCtlAttach.uiFwVer) {
        MSG_PRINTF("FW version  attach failed need upgrade to new");
    }

    iSockFd = pstCltLink->iSockFd;
    if (iSockFd == -1) {
        ERR_PRINTF("invalid socket fd");
        return ERROR_FAILE;
    }
    pstRespMsg = MSG_Create(MSG_MT_CTL, MSG_CTL_ATTACH_RESP);
    if (NULL == pstRespMsg) {
        ERR_PRINTF("create msg failed");
        ulRet = ERROR_FAILE;
    }

    memset(&stCtlAttachResp, 0, sizeof(stCtlAttachResp));
    stCtlAttachResp.uiAttachStatus = MSG_ATTACH_RESP_STATUS_NEED_UPGRAED;

    MSG_AddData(pstRespMsg, MSG_CTL_ATTACH_RESP, &stCtlAttachResp, sizeof(stCtlAttachResp));

    uiLen = sizeof(ucMsgBuf);
    NS_MSG_MsgList2MsgBuf(pstRespMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    ulRet = CLIENT_MSG_send(pstCltLink, ucMsgBuf, uiLen);

    MSG_Destroy(pstRespMsg);
    pstRespMsg = NULL;


    return ERROR_SUCCESS;
}

STATIC ULONG _donwload_and_upgraed_TaskFunc(VOID *args /* ==> (NS_TASK_INFO *) */) {
    NS_TASK_INFO       *pstTask       = NULL;
    MSG_SRV_LINK_ST    *pstSrvLink    = NULL;
    MSG_CTL_UPGRADE_ST *pstCtlUpgrade = NULL;
    CHAR *pcBuf = NULL;

    DBGASSERT(args != NULL);

    pstTask = (NS_TASK_INFO *)args;
    pcBuf = (MSG_SRV_LINK_ST *)pstTask->ulArgs[0];
    //pstCtlUpgrade = (MSG_SRV_LINK_ST *)pstTask->ulArgs[1];

    ERR_PRINTF("url=%s", pcBuf);

   //return MSG_server_ctl_send_attach(pstSrvLink); 
   return ERROR_SUCCESS;
}

ULONG MSG_clinet_ctl_recv_Upgrade(MSG_CLT_LINK_ST *pstCltLink, VOID *pMsg)
{
    INT   iSockFd       = -1;
    UINT  uiClientFwVer = 0;
    ULONG ulRet         = ERROR_FAILE;
    UINT  uiLen         = 0;
    NS_MSG_ST *pstMsg   = NULL;
    MSG_CTL_UPGRADE_ST stCtlUpgrade;
    MSG_CTL_ATTACH_RESP_ST stCtlAttachResp;
    UCHAR ucMsgBuf[1024]  = {0};
    NS_MSG_ST *pstRespMsg = NULL;
    ULONG ulArgs[4];
    CHAR *pcBuf = NULL;

    DBGASSERT(NULL != pMsg);

    pstMsg = (NS_MSG_ST *)pMsg;

    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stCtlUpgrade, sizeof(stCtlUpgrade));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    if (stCtlUpgrade.uiCmd == MSG_CTL_UPGRADE_CMD_DO_UPGTADE) {
        MSG_PRINTF("do upgrade, FW URL:%s", stCtlUpgrade.ucFwUrl);
    }

    MSG_PRINTF("------------------------");
    /* start a donwload task */
    ulArgs[0] = (ULONG)pstCltLink;
    pcBuf = malloc(strlen((CHAR *)stCtlUpgrade.ucFwUrl) + 1);
    if (NULL  == pcBuf) {
        ERR_PRINTF("oom.");
        return ERROR_FAILE;
    }
    MSG_PRINTF("------------------------");

    snprintf(pcBuf, strlen((CHAR *)stCtlUpgrade.ucFwUrl) + 1, "%s", stCtlUpgrade.ucFwUrl);
    //ulArgs[1] = (ULONG)pcBuf;
    MSG_PRINTF("------------------------");

    ulRet = Server_Task_Create(_donwload_and_upgraed_TaskFunc, pcBuf);
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("create tesk failed.");
    }
    MSG_PRINTF("------------------------");

    return ERROR_FAILE;
}
