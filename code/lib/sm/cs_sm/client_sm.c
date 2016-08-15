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

#include "client_sm.h"
#include <ns_base.h>
#include <ns_msg.h>
#include <ns_sm.h>
#include <ns_sm_client.h>
#include <ns_msg_client_link.h>


INT CLT_SM_EVT__TEST(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSMEvt, IN VOID * args)
{
    MSG_PRINTF("************%d**************\n", enCltSMEvt);
    
    return 0;
}

STATIC ULONG _CLT_GetIfInfo(IN UCHAR *pcMac, IN INT iBufLen)
{
    NET_IP_INFO_S *pstIfInfo = NULL;

    DBGASSERT(NULL != pcMac);
    DBGASSERT(6    != iBufLen);

    pstIfInfo = NET_GetIFInfo();
    if (pstIfInfo == NULL) {
        ERR_PRINTF("get local ip info failed");
        return ERROR_FAILE;
    }
    INT i = 0;
    MSG_PRINTF("cnt[%d]", pstIfInfo->iCnt);
    for (i = 0; i < pstIfInfo->iCnt; i++) {
        MSG_PRINTF("ifname:[%s]", pstIfInfo->pstNetIFData[i].szIFName);
        MSG_PRINTF("mac   :[%s]", pstIfInfo->pstNetIFData[i].szIFMACAddr);
        MSG_PRINTF("ifaddr:[%s]", pstIfInfo->pstNetIFData[i].szIFAddr);
        MSG_PRINTF("ifaddr:[0x%x]", pstIfInfo->pstNetIFData[i].uiIFAddr);
        if (strncmp(pstIfInfo->pstNetIFData[i].szIFName, "eth0", 4) == 0) {
            break;
        } else if (strncmp(pstIfInfo->pstNetIFData[i].szIFName, "eno", 3) == 0) {
            break;
        }
    }

    memcpy(pcMac, pstIfInfo->pstNetIFData[i].ucIFMACAddr, iBufLen);

    NET_DestroyIFInfo(pstIfInfo);
    pstIfInfo = NULL;

    return ERROR_SUCCESS;
}

INT CLT_SM_EVT_FUNC_IDLE_hd_CONNED(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSMEvt, IN VOID * args)
{
    NS_MSG_ST *pstMsg = NULL;	
    MSG_MNG_JOIN_REQ_ST stJoinReq;
    MSG_CLT_LINK_ST *pstMsgCltLink = NULL;
	
    DBGASSERT(NULL != pstCltSM);
    MSG_PRINTF("************%d**************", enCltSMEvt);

    pstMsgCltLink = (MSG_CLT_LINK_ST *)pstCltSM->pLinkInfo;
    MSG_PRINTF("connect sock id=%d", pstMsgCltLink->iSockFd);

    /* creat a msg */
    pstMsg = MSG_Create(MSG_MT_MNG, MSG_MNG_JOIN_REQ);
    if (NULL == pstMsg) {
        ERR_PRINTF("MSG Create failed.");
        return -1;
    }
    memset(&stJoinReq, 0, sizeof(MSG_MNG_JOIN_REQ));
    stJoinReq.uiClientID = 0xA5A5;
    if (ERROR_SUCCESS != _CLT_GetIfInfo(stJoinReq.ucClientMac, sizeof(stJoinReq.ucClientMac))) {
        ERR_PRINTF("Get IF info failed.");
        return -1;
    }
    MSG_PRINTF("Join Req ClientMac = "NS_MAC_FORMAT,
            NS_PRINT_MAC(stJoinReq.ucClientMac));

    MSG_AddData(pstMsg, MSG_MNG_JOIN_REQ, &stJoinReq, sizeof(stJoinReq));
   
    UCHAR ucMsgBuf[1024] = {0};
    UINT  uiLen = sizeof(ucMsgBuf);
    
    /* 吧消息链表转换长字符流，这个字符流可以被发送，或者加密什么的 */
    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    CLIENT_MSG_send(pstMsgCltLink, ucMsgBuf, uiLen);

    /* 消息发送完自后，消息链表就可以释放了 */
    MSG_Destroy(pstMsg);

    CLT_SM_STATS_CHANGE(pstCltSM, CLT_SM_STATS_WAIT_JOIN_RESP, NULL);

    return 0;
}

STATIC INT CLT_SM_EVT_FUNC_WaitJResp_hd_JResp(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSMEvt, IN VOID * args)
{
    NS_MSG_ST *pstMsg;
    MSG_MNG_JOIN_RESP_ST stJoinResp;
    ULONG ulRet = ERROR_FAILE;

    DBGASSERT(args != NULL);

    MSG_PRINTF("************%d**************\n", enCltSMEvt);

    pstMsg = (NS_MSG_ST *)args;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stJoinResp, sizeof(stJoinResp));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Join Resp server id = 0x%x", stJoinResp.uiServerID);

    CLT_SM_STATS_CHANGE(pstCltSM, CLT_SM_STATS_WAIT_OK, NULL);

    //Send Confirm
    MSG_MNG_CONFIRM_ST stConfirm;
    UCHAR ucMsgBuf[1024] = {0};
    UINT  uiLen = sizeof(ucMsgBuf);
    MSG_CLT_LINK_ST *pstCltLink = NULL;
    INT iSockFd = -1;

    //iSockFd = MSG_server_GetSockFdBySm(pstSrvSM);
    pstCltLink = (MSG_CLT_LINK_ST *)(pstCltSM->pLinkInfo);
    DBGASSERT(NULL != pstCltLink);
    iSockFd = pstCltLink->iSockFd;
    if (iSockFd == -1) {
        ERR_PRINTF("invalid socket fd");
        return ERROR_FAILE;
    }

    pstCltLink->ulSessionID = stJoinResp.ulSessionID;

    MSG_PRINTF("server sock fd:%d, sessionID:%d", 
                iSockFd,
                pstCltLink->ulSessionID);

    pstMsg = MSG_Create(MSG_MT_MNG, MSG_MNG_CONFIM);
    if (NULL == pstMsg) {
        ERR_PRINTF("create msg failed");
        ulRet = ERROR_FAILE;
    }

    memset(&stConfirm, 0, sizeof(stConfirm));
    stConfirm.uiConfirmID = 0xAA56;

    MSG_AddData(pstMsg, MSG_MNG_CONFIM, &stConfirm, sizeof(stConfirm));

    NS_MSG_MsgList2MsgBuf(pstMsg, ucMsgBuf, &uiLen);
    MSG_PRINTF("msg len = %d", uiLen);

    //send data
    CLIENT_MSG_send(pstCltLink, ucMsgBuf, uiLen);

    MSG_Destroy(pstMsg);
    pstMsg = NULL;
    
    return ulRet;
}

STATIC INT CLT_SM_EVT_FUNC_WaitOK_hd_OK(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSMEvt, IN VOID * args)
{
    NS_MSG_ST *pstMsg;
    MSG_MNG_OK_ST stOK;
    ULONG ulRet = ERROR_FAILE;

    DBGASSERT(args != NULL);

    MSG_PRINTF("************%d**************\n", enCltSMEvt);

    pstMsg = (NS_MSG_ST *)args;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stOK, sizeof(stOK));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Join Resp OK id = 0x%x", stOK.uiOKID);

    CLT_SM_STATS_CHANGE(pstCltSM, CLT_SM_STATS_RUN, NULL);

    MSG_PRINTF("Clint fsm is Run on!");
    
    return ulRet;
}

STATIC CLT_SM_EVT_MAP_ST g_astCltSMEvtMapList[CLT_SM_STATS_MAX][CLT_SM_EVT_MAX] = 
{
    [CLT_SM_STATS_INIT] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST, },
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST, },
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST, },
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST, },
    },
    [CLT_SM_STATS_IDEL] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT_FUNC_IDLE_hd_CONNED},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
    [CLT_SM_STATS_WAIT_JOIN_RESP] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT_FUNC_WaitJResp_hd_JResp},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
    [CLT_SM_STATS_WAIT_CONFIRM] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
    [CLT_SM_STATS_WAIT_OK] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT_FUNC_WaitOK_hd_OK},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
    [CLT_SM_STATS_RUN] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
    [CLT_SM_STATS_STOP] = {
        [CLT_SM_EVT_CONNED]          = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_JOIN_RESP]  = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_OK]         = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
        [CLT_SM_EVT_RECV_CONN_BREAK] = {SRV_SM_EVT_CONNED, CLT_SM_EVT__TEST},
    },
};

INT CLT_SM_EVT_HANDLE(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSmEvt, IN VOID * args)
{
    CLT_SM_EVT_PF pfCltSMEvt = NULL;

    DBGASSERT(NULL != pstCltSM);

    if (enCltSmEvt < 0 || enCltSmEvt >= CLT_SM_EVT_MAX) {
        ERR_PRINTF("invalid event.");
        return -1;
    }

    pfCltSMEvt = g_astCltSMEvtMapList[pstCltSM->enCltSMStats][enCltSmEvt].pfCltSMEvt;
    if (NULL == pfCltSMEvt) 
    {
        ERR_PRINTF("no function to handle the event.");
        return -1;
    }

    //INT CLT_SM_EVT__TEST(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSMEvt, IN VOID * args)
    return pfCltSMEvt(pstCltSM, enCltSmEvt, args);
}

/* change status */
INT CLT_SM_STATS_CHANGE(CLT_SM_ST *pstCltSM, CLT_SM_STATS_EN enCLtSMStatNow, VOID *args)
{
    INT iRet = 0;
    //CLT_SM_EVT_EN enEvt;

    MSG_PRINTF("client sm trans from %d to %d", pstCltSM->enCltSMStats, enCLtSMStatNow);
    pstCltSM->enCltSMStats = enCLtSMStatNow;

    //do state chang trans...

    /* 进入idle则发起socket连接成功事件，准备开始握手 */
    if (enCLtSMStatNow == CLT_SM_STATS_IDEL) {
        iRet = CLT_SM_EVT_HANDLE(pstCltSM, CLT_SM_EVT_CONNED, args);
    }

    return iRet;
}

CLT_SM_ST * CLT_sm_CreateAndStart(VOID)
{
    CLT_SM_ST *pstCltSM = NULL;

    pstCltSM = malloc(sizeof(CLT_SM_ST));
    if (NULL == pstCltSM) {
        ERR_PRINTF("malloc failed.");
        return NULL;
    }
    memset(pstCltSM, 0, sizeof(CLT_SM_ST));

    CLT_SM_STATS_CHANGE(pstCltSM, CLT_SM_STATS_INIT, NULL);

    MSG_PRINTF("fsm is create an start.");

    return pstCltSM;
}

INT CLT_sm_Stop(CLT_SM_ST *pstCltSm)
{
    DBGASSERT(NULL != pstCltSm);

    return 0;
}

VOID CLT_sm_Destroy(IN CLT_SM_ST *pstCltSm)
{
    DBGASSERT(NULL != pstCltSm);

    if (pstCltSm->enCltSMStats != CLT_SM_STATS_STOP) {
        CLT_sm_Stop(pstCltSm);
    }

    free(pstCltSm);
    pstCltSm = NULL;

    return;
}


INT CLT_sm_init(void)
{
    MSG_desc_reg();
            /*
    MSG_Desc_Init();

    MSG_Desc_Register(MSG_MNG_JOIN_REQ, 
            sizeof(MSG_MNG_JOIN_REQ_ST), 
            MSG_DATA_encode_TLV_JoinReq, 
            MSG_DATA_decode_TLV_JoinReq); 
    MSG_Desc_Register(MSG_MNG_JOIN_RESP, 
            sizeof(MSG_DATA_TEST_ST), 
            MSG_DATA_encode_TLV_Test, 
            MSG_DATA_decode_TLV_Test); 
    MSG_Desc_Register(MSG_MNG_CONFIM, 
            sizeof(MSG_DATA_TEST_ST), 
            MSG_DATA_encode_TLV_Test, 
            MSG_DATA_decode_TLV_Test); 
            */
    
    return 0;
}
