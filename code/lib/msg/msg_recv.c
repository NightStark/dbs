#ifndef __MSG_RECV_C__
#define __MSG_RECV_C__

//THIS IS SERVER RECEIVE functions file.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <asm/errno.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_epoll.h>
#include <ns_lilist.h>
#include <ns_thread.h>
#include <ns_net.h>
#include <ns_sm.h>
#include <ns_server.h>
#include <ns_msg_server_link.h>

INT MSG_normal_recv(INT iConnFd, VOID *pRecvBuf, INT iBufLen)
{
	INT iMsgLen = 0;
    iMsgLen = recv(iConnFd, 
			   	   pRecvBuf, 
			   	   iBufLen, 0);
	return iMsgLen;
}

INT MSG_SSL_recv(SSL *pstSSL, VOID *pRecvBuf, INT iBufLen)
{
	INT iMsgLen = 0;

	iMsgLen = SSL_read(pstSSL, pRecvBuf, iBufLen);

	return iMsgLen;
}

INT SERVER_MSG_SSL_recv(MSG_SRV_LINK_ST *pstSrvLink, VOID *pRecvBuf, INT iBufLen)
{
	DBGASSERT(NULL != pstSrvLink);

	if (pstSrvLink->pstSSL != NULL) {
        MSG_PRINTF("SSL recv");
		return MSG_SSL_recv(pstSrvLink->pstSSL, pRecvBuf, iBufLen);
	} else {
        MSG_PRINTF("normal recv");
		return MSG_normal_recv(pstSrvLink->iSockFd, pRecvBuf, iBufLen);
	}
}

STATIC ULONG MSG_RECV_HandleMngMsg(IN INT iConnFd, IN NS_MSG_ST *pstMsg)
{
    UINT ulRet = 0;
    SRV_SM_ST *pstSrvSM = NULL;
    SRV_SM_EVT_EN enSrvSMEvt;
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    DBGASSERT(NULL != pstMsg);  
    
    pstSrvLink = MSG_server_link_FindBySockFd(iConnFd);
    if (NULL == pstSrvLink) {
        ERR_PRINTF("Can not find link by socked:%d", iConnFd);
    }

    pstSrvSM = pstSrvLink->pstSrvSm;

    switch (pstMsg->usSubType) {
        case MSG_MNG_JOIN_REQ:
            enSrvSMEvt = SRV_SM_EVT_RECV_JOIN;
            break;
        case MSG_MNG_CONFIM:
            enSrvSMEvt = SRV_SM_EVT_RECV_CONFIRM;
            break;
        default:
            ERR_PRINTF("Invalid Sub msg type!");
    }

    ulRet =  SRV_SM_EVT_HANDLE(pstSrvSM, enSrvSMEvt, pstMsg);

    return ulRet;
}

STATIC ULONG MSG_RECV_HandleMsg(IN INT iConnFd, IN UCHAR *pauRecvBuf, IN INT iMsgLen)
{
    UINT ulRet = ERROR_FAILE;
    NS_MSG_ST *pstMsg = NULL;	

    pstMsg = NS_MSG_MsgBuf2MsgList(pauRecvBuf, iMsgLen);
    
    //解析数据类型
    switch(pstMsg->usMainType)
    {
        case MSG_MT_MNG:
            ulRet = MSG_RECV_HandleMngMsg(iConnFd, pstMsg);
            break;
        case MSG_MT_CTL:

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

ULONG MSG_RECV_RecvData(IN INT iConnFd, IN INT events, IN VOID *arg)
{

    //EPOLL_EVENTS_S  pstEpollEvents = NULL;
    ULONG 				ulRet = 0;
    INT                 iMsgLen        = 0;
	UCHAR 				aulRecvBuf[NSDB_MSG_LEN_MAX];
	//MSG_MSG_RECV_INFO_S stMsgRecvInfo = {};
	MSG_CLIENT_INFO_S   stMsgClientInfo = {};	
	THREAD_EPOLL_EVENTS_S *pstThrdEpEv = NULL;

	pstThrdEpEv = (THREAD_EPOLL_EVENTS_S *)arg;

	mem_set0(aulRecvBuf, sizeof(aulRecvBuf));

	MSG_PRINTF("Thread ID = %d", pstThrdEpEv->iThreadId);

	/*
    iMsgLen = recv(iConnFd, 
			   	    aulRecvBuf, 
			   	    NSDB_MSG_LEN_MAX, 0);
					*/
	MSG_SRV_LINK_ST *pstSrvLink = MSG_server_link_FindBySockFd(iConnFd);
    MSG_PRINTF("Link (fd:%d)", pstSrvLink->iSockFd);
	iMsgLen = SERVER_MSG_SSL_recv(pstSrvLink, 
			aulRecvBuf, 
			NSDB_MSG_LEN_MAX);
	
    CHAR TNAME[56] = {};
	sprintf(TNAME, "S_RECV_%d.dat", iMsgLen);
	DBG_PRINT_LOG(TNAME,(CHAR *)stMsgClientInfo.stCNetIFData.szIFAddr, iMsgLen);

	if (ETIMEDOUT == errno)
	{
		perror("Recv!");
        ERR_PRINTF("link is break ! ETIMEDOUT!");
        return ERROR_FAILE;
	}
			   	    
    if (iMsgLen > 0){
		MSG_PRINTF("msg len %d", iMsgLen);
		
        ulRet = MSG_RECV_HandleMsg(iConnFd, aulRecvBuf, iMsgLen);
	    //stMsgRecvInfo.uiLinkFD = (UINT)iConnFd;
	    /* 解析消息长度和类型 */
		//ulRet = MSG_de_DecMsgHead(&stMsgRecvInfo, aulRecvBuf);
		
		//FMS_msg_Proc(&stMsgRecvInfo);
	}else if (0 == iMsgLen){
        ERR_PRINTF("Client Has ShutDown!");
        return ERROR_LINK_CLIENT_DOWN;
    }else{
        ERR_PRINTF("Recv ERROR!");
        return ERROR_FAILE;
    }
    
    return ulRet;
}

#endif //__MSG_RECV_C__
