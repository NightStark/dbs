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
#include <ns_task.h>

STATIC INT g_iSrvSMID = 0;

INT SRV_SM_EVT__TEST(SRV_SM_ST *pstSrvSM, SRV_SM_EVT_EN enSrvSMEvt, VOID * args)
{
    MSG_PRINTF("************%d**************\n", enSrvSMEvt);
    
    return 0;
}

STATIC ULONG g_ulSrvSession = 0;

STATIC INT SRV_SM_EVT_Idle_hd_JonReq(SRV_SM_ST *pstSrvSM, SRV_SM_EVT_EN enSrvSMEvt, VOID * args)
{
    NS_MSG_ST *pstMsg;
    MSG_MNG_JOIN_REQ_ST stJoinReq;
    ULONG ulRet = ERROR_FAILE;

    DBGASSERT(args != NULL);

    MSG_PRINTF("************%d**************\n", enSrvSMEvt);

    pstMsg = (NS_MSG_ST *)args;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stJoinReq, sizeof(stJoinReq));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Join Req id = 0x%x", stJoinReq.uiClientID);
    MSG_PRINTF("Join Req ClientMac = "NS_MAC_FORMAT,
            NS_PRINT_MAC(stJoinReq.ucClientMac));

    SRV_SM_STATS_CHANGE(pstSrvSM, SRV_SM_STATS_WAIT_CONFIRM);

    //Send Join Response.
    MSG_MNG_JOIN_RESP_ST stJoinResp;
    UCHAR ucMsgBuf[1024] = {0};
    UINT  uiLen = sizeof(ucMsgBuf);
    MSG_SRV_LINK_ST *pstSrvLink = NULL;
    INT iSockFd = -1;

    //iSockFd = MSG_server_GetSockFdBySm(pstSrvSM);
    pstSrvLink = (MSG_SRV_LINK_ST *)(pstSrvSM->pLink);
    DBGASSERT(NULL != pstSrvLink);
    iSockFd = pstSrvLink->iSockFd;
    if (iSockFd == -1) {
        ERR_PRINTF("invalid socket fd");
        return ERROR_FAILE;
    }

    /* save client mac addr */
    memcpy(pstSrvLink->stClientInfo.aucClientMac, 
           stJoinReq.ucClientMac,
           sizeof(pstSrvLink->stClientInfo.aucClientMac));

    MSG_PRINTF("clent sock fd:%d", iSockFd);

    pstMsg = MSG_Create(MSG_MT_MNG, MSG_MNG_JOIN_RESP);
    if (NULL == pstMsg) {
        ERR_PRINTF("create msg failed");
        ulRet = ERROR_FAILE;
    }

    memset(&stJoinResp, 0, sizeof(stJoinResp));
    stJoinResp.uiServerID  = 0xAA55;
    stJoinResp.ulSessionID = g_ulSrvSession++; //TODO:session mybe can sign different user.

    pstSrvLink->ulSessionID = stJoinResp.ulSessionID;

    MSG_AddData(pstMsg, MSG_MNG_JOIN_RESP, &stJoinResp, sizeof(stJoinResp));

    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    SERVER_MSG_send(pstSrvLink, ucMsgBuf, uiLen);

    MSG_Destroy(pstMsg);
    pstMsg = NULL;

    
    return ulRet;
}

STATIC ULONG _attach_client_TaskFunc(VOID *args /* ==> (NS_TASK_INFO *) */) {
    NS_TASK_INFO    *pstTask    = NULL;
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    DBGASSERT(args != NULL);

    pstTask = (NS_TASK_INFO *)args;
    pstSrvLink = (MSG_SRV_LINK_ST *)pstTask->ulArgs[0];

   return MSG_server_ctl_send_attach(pstSrvLink); 
}

STATIC INT SRV_SM_EVT_WaitConfirm_hd_Confirm(SRV_SM_ST *pstSrvSM, SRV_SM_EVT_EN enSrvSMEvt, VOID * args)
{
    NS_MSG_ST *pstMsg;
    MSG_MNG_CONFIRM_ST  stConfirm;
    ULONG ulRet = ERROR_FAILE;

    DBGASSERT(args != NULL);

    MSG_PRINTF("************%d**************\n", enSrvSMEvt);

    pstMsg = (NS_MSG_ST *)args;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stConfirm, sizeof(stConfirm));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Join Req id = 0x%x", stConfirm.uiConfirmID);

    SRV_SM_STATS_CHANGE(pstSrvSM, SRV_SM_STATS_RUN);

    //Send OK Response.
    MSG_MNG_OK_ST stOk;
    UCHAR ucMsgBuf[1024] = {0};
    UINT  uiLen = sizeof(ucMsgBuf);
    MSG_SRV_LINK_ST *pstSrvLink = NULL;
    INT iSockFd = -1;

    //iSockFd = MSG_server_GetSockFdBySm(pstSrvSM);
    pstSrvLink = (MSG_SRV_LINK_ST *)(pstSrvSM->pLink);
    DBGASSERT(NULL != pstSrvLink);
    iSockFd = pstSrvLink->iSockFd;
    if (iSockFd == -1) {
        ERR_PRINTF("invalid socket fd");
        return ERROR_FAILE;
    }

    MSG_PRINTF("clent sock fd:%d", iSockFd);

    pstMsg = MSG_Create(MSG_MT_MNG, MSG_MNG_OK);
    if (NULL == pstMsg) {
        ERR_PRINTF("create msg failed");
        ulRet = ERROR_FAILE;
    }

    memset(&stOk, 0, sizeof(stOk));
    stOk.uiOKID = 0xAA66;

    MSG_AddData(pstMsg, MSG_MNG_OK, &stOk, sizeof(stOk));

    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    SERVER_MSG_send(pstSrvLink, ucMsgBuf, uiLen);

    MSG_Destroy(pstMsg);
    pstMsg = NULL;


    //tell work thread to do some work.
    ulRet = Server_Task_Create(_attach_client_TaskFunc, pstSrvLink);
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("create tesk failed.");
    }
    
    return ulRet;
}

STATIC SRV_SM_EVT_MAP_ST g_astSrvSMEvtMapList[SRV_SM_STATS_MAX][SRV_SM_EVT_MAX] = 
{
    [SRV_SM_STATS_INIT] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
    [SRV_SM_STATS_IDEL] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT_Idle_hd_JonReq,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
    [SRV_SM_STATS_WAIT_JOIN] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
    [SRV_SM_STATS_WAIT_CONFIRM] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT_WaitConfirm_hd_Confirm,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
    [SRV_SM_STATS_RUN] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
    [SRV_SM_STATS_STOP] = {
        [SRV_SM_EVT_CONNED] = {
            SRV_SM_EVT_CONNED,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_JOIN] = {
            SRV_SM_EVT_RECV_JOIN,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONFIRM] = {
            SRV_SM_EVT_RECV_CONFIRM,
            SRV_SM_EVT__TEST,
        },
        [SRV_SM_EVT_RECV_CONN_BREAK] = {
            SRV_SM_EVT_RECV_CONN_BREAK,
            SRV_SM_EVT__TEST,
        },
    },
};

INT SRV_SM_STATS_CHANGE(SRV_SM_ST *pstSrvSM, SRV_SM_STATS_EN enSrvSMStatNow)
{
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    pstSrvLink = (MSG_SRV_LINK_ST *)(pstSrvSM->pLink);
    DBGASSERT(NULL != pstSrvLink);

    MSG_PRINTF("[LinkID:%d]server sm trans from %d to %d\n", pstSrvLink->uiLinkID, pstSrvSM->enSrvSMStats, enSrvSMStatNow);

    pstSrvSM->enSrvSMStats = enSrvSMStatNow;

    

    //do state chang trans...
    //

    return 0;
}

INT SRV_SM_EVT_HANDLE(IN SRV_SM_ST *pstSrvSM, IN SRV_SM_EVT_EN enSrvSMEvt, IN VOID * args)
{
    SRV_SM_EVT_PF pfSrvSMEvt = NULL;

    DBGASSERT(NULL == pstSrvSM);

    if (enSrvSMEvt < 0 || enSrvSMEvt >= SRV_SM_EVT_MAX)
    {
        ERR_PRINTF("invalid event.");
        return -1;
    }

    pfSrvSMEvt = g_astSrvSMEvtMapList[pstSrvSM->enSrvSMStats][enSrvSMEvt].pfSrvSMEvt;
    if (NULL == pfSrvSMEvt) 
    {
        ERR_PRINTF("no function to handle the event.");
        return -1;
    }
    // INT SRV_SM_EVT__TEST(SRV_SM_ST *pstSrvSM, SRV_SM_EVT_EN enSrvSMEvt, VOID * args)

    return pfSrvSMEvt(pstSrvSM, enSrvSMEvt, args);
}

SRV_SM_ST *SRV_sm_CreateAndStart(VOID)
{
    SRV_SM_ST *stSrvSM = NULL;

    stSrvSM = malloc(sizeof(SRV_SM_ST));
    if (NULL == stSrvSM)
    {
        ERR_PRINTF("malloc failed.");
        return NULL;
    }
    memset(stSrvSM, 0, sizeof(SRV_SM_ST));
    
    stSrvSM->iSMID = g_iSrvSMID++;

    MSG_PRINTF("fsm is create an start.");

    return stSrvSM;
}

INT SRV_sm_Stop(SRV_SM_ST *pstSrvSm)
{
   DBGASSERT(NULL != pstSrvSm); 

   SRV_SM_STATS_CHANGE(pstSrvSm, SRV_SM_STATS_STOP);

   return 0;
}

VOID SRV_sm_Destroy(SRV_SM_ST *pstSrvSm)
{
   DBGASSERT(NULL != pstSrvSm); 
   
   if (pstSrvSm->enSrvSMStats != SRV_SM_STATS_STOP) 
   {
       SRV_sm_Stop(pstSrvSm);
   }

    free(pstSrvSm);
    pstSrvSm = NULL;
}

VOID SRV_sm_Init(VOID)
{
    MSG_desc_reg();

    return;
}
