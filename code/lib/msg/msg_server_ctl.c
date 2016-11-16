#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>

#include <ns_net.h>
#include <ns_table.h>
#include <ns_sm.h>
#include <ns_msg_server_link.h>
#include <ns_msg.h>
#include <ns_server.h>

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
