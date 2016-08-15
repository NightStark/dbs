#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <asm/errno.h>
#include <arpa/inet.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_sm.h>
#include <ns_epoll.h>
#include <ns_timer.h>
#include <ns_ssl.h>
#include <ns_msg_client_init.h>
#include <ns_msg_client_link.h>
#include <ns_thread_quemsg.h>
#include <ns_thread.h>

#define CLIENT_LINK_KEEPALIVE_IDEL (1)
#define CLIENT_LINK_KEEPALIVE_INTVL (1)
#define CLIENT_LINK_KEEPALIVE_CNT (2)

ULONG msg_client_EpCallBack(IN UINT events, IN VOID *arg);
ULONG msg_client_SSL_conn_EpCallBack(IN UINT events, IN VOID *arg);

#define MSG_CLIENT_EPOLL_NUM_MAX (4)

STATIC ULONG msg_client_ConnInit(IN THREAD_INFO_S *pstThreadInfo);

VOID msg_clientserver_QueMsgRecv(IN INT iThread,IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{		
	MSG_PRINTF("Revc msg = [%s] from Thread(%d)", (CHAR *)pstThrdQueMsg->pQueMsgData,
													iThread);
		

	//MSG_PRINTF("sleep......");
	//sleep(1);
	//MSG_PRINTF("wakeup......");
	
    THREAD_QUEMSG_DATA_S stThrdQueMsg;
    char *pstData = NULL;
    mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
    
	pstData = mem_alloc(100);
	DBGASSERT(NULL != pstData);
    sprintf(pstData, "This is Resp Test Msg--of [%s]", (CHAR *)pstThrdQueMsg->pQueMsgData);
    MSG_PRINTF("Send Resp Test Msg --of [%s]", (CHAR *)pstThrdQueMsg->pQueMsgData);
    stThrdQueMsg.uiQueMsgDataLen = 25;
    stThrdQueMsg.uiQueMsgType =  1;
    stThrdQueMsg.pQueMsgData = pstData;
    THREAD_server_QueMsg_Send(iThread,&(stThrdQueMsg));
	
	free(pstThrdQueMsg->pQueMsgData);
	
    return;
}

VOID msg_client_QueMsgRecv(IN INT iThreadId,IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	THREAD_QUEMSG_DATA_S stThrdQueMsg;

	if (pstThrdQueMsg->uiQueMsgType == THRD_QUEMSG_TYPE_WEB_EVENT_ASK)
	{
		MSG_PRINTF("Revc msg = [%lu] from Thread(%d)", 
					(ULONG)pstThrdQueMsg->pQueMsgData,
					iThreadId);
		mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
		stThrdQueMsg.uiQueMsgDataLen  = 4;
		stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_WEB_EVENT_RESP;
		stThrdQueMsg.pQueMsgData  = (VOID *)(ULONG)256;
		THREAD_server_QueMsg_Send(iThreadId,&(stThrdQueMsg));
	}
	else
	{
		MSG_PRINTF("Revc msg = [%s] from Thread(%d)", 
					(CHAR *)pstThrdQueMsg->pQueMsgData,
					iThreadId);
		MSG_PRINTF("Thread(THREAD_TYPE_LINK) to Thread (%d)", iThreadId);
				  
		mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
		stThrdQueMsg.uiQueMsgDataLen  = 1;
		stThrdQueMsg.uiQueMsgType = THRD_QUEMSG_TYPE_WEB_EVENT_RESP;
		stThrdQueMsg.pQueMsgData  = (VOID *)(ULONG)890098;
		THREAD_server_QueMsg_Send(iThreadId,&(stThrdQueMsg));
		
		free(pstThrdQueMsg->pQueMsgData);
	}
		


	
	
    return;
}

STATIC ULONG msg_client_ConnSuccess2(THREAD_EPOLL_EVENTS_S *pstThrdEpEvt, SSL *pstSSL)
{
	ULONG ulErrCode;

	DBGASSERT(NULL != pstThrdEpEvt);
	DBGASSERT(NULL != pstSSL);

    /* 连接成功后，这里只注册了接收时间，TODO::发送的呢？，还是另外一个函数呢？*/
	ulErrCode = THREAD_epoll_EventMod(pstThrdEpEvt->iEventFd,
						  			  EPOLLIN|EPOLLET|EPOLLERR,
						  			  pstThrdEpEvt->iThreadId,
						  			  msg_client_EpCallBack,
						  			  pstThrdEpEvt->pPriData);
	if (ERROR_SUCCESS != ulErrCode)
	{
		ERR_PRINTF("THREAD epoll EventMod Failed!");
		return ulErrCode;
	}


    //TODO: create fsm and ...
    CLT_SM_ST *pstCltSm = NULL;
    pstCltSm = CLT_sm_CreateAndStart();
    if (pstCltSm == NULL) {
		ERR_PRINTF("Create sm failed!");
		return ulErrCode;
    }

    MSG_CLT_LINK_ST *pstMsgCltLink = NULL;
    pstMsgCltLink = MSG_client_link_Create(pstThrdEpEvt->iEventFd, pstThrdEpEvt->iThreadId);
    if (NULL == pstMsgCltLink) {
        ERR_PRINTF("malloc failed");
        goto err;
    }
	pstMsgCltLink->pstSSL = pstSSL;
    pstMsgCltLink->pstCltSm = pstCltSm;
    pstCltSm->pLinkInfo = (VOID *)pstMsgCltLink;

    CLT_SM_STATS_CHANGE(pstCltSm, CLT_SM_STATS_IDEL, NULL);

    return ERROR_SUCCESS;

err:
    if (pstCltSm) {
        CLT_sm_Stop(pstCltSm);
        pstCltSm = NULL;
    }
    return ERROR_FAILE;
	return ERROR_SUCCESS;
}

ULONG msg_client_ConnSuccess(THREAD_EPOLL_EVENTS_S *pstThrdEpEvt)
{
	INT   iRet               = 0;
	SSL  *pstSSL             = NULL;
	INT   iConnFd            = 0;
	INT	  iSetKeepAlive      = 0;
	INT	  iKeepAliveIdel     = 0;
	INT	  iKeepAliveInterval = 0;
	INT	  iKeepAliveCount    = 0;
	ULONG ulErrCode    	     = ERROR_SUCCESS;
	THREAD_INFO_S *pstThrd   = NULL;

	DBGASSERT(NULL != pstThrdEpEvt);

	iConnFd = pstThrdEpEvt->iEventFd;
	
    /* 连接成功，删除定时器 */
	ulErrCode = TIMER_DeleteForThread(pstThrdEpEvt->iThreadId, 
									  pstThrdEpEvt->iTimerId);
	if (ERROR_SUCCESS != ulErrCode)
	{
		ERR_PRINTF("THREAD Timer Delete Failed!");		
		return ulErrCode;
	}
	pstThrdEpEvt->iTimerId = -1;
	
	/* 设置 链接保活探测 */
	iSetKeepAlive = 1;
	iRet = setsockopt(iConnFd, 
					  SOL_SOCKET, 
					  SO_KEEPALIVE, 
					  (VOID *)&iSetKeepAlive,
					  (INT)sizeof(iSetKeepAlive));
	if (0 > iRet)
	{
		ERR_PRINTF("SO_KEEPALIVE set Failed!");
        return ERROR_FAILE;
	}

	/*
		TCP_KEEPIDLE 	指定多长时间后发起探测
		TCP_KEEPINTVL 	指定每次探测相隔多长时间
		TCP_KEEPCNT		指定探测次数
		超过探测次数后，epoll上触发EPOLLIN时间，
			recv 返回 0 或 <0， errno == ETIMEDOUT
	*/
	iKeepAliveIdel 		= CLIENT_LINK_KEEPALIVE_IDEL;
	iKeepAliveInterval 	= CLIENT_LINK_KEEPALIVE_INTVL;
	iKeepAliveCount 	= CLIENT_LINK_KEEPALIVE_CNT;
	iRet = setsockopt(iConnFd, 
					  SOL_TCP, 
					  TCP_KEEPIDLE, 
					  (VOID *)&iKeepAliveIdel,
					  (INT)sizeof(iKeepAliveIdel));
	iRet |= setsockopt(iConnFd, 
					  SOL_TCP, 
					  TCP_KEEPINTVL, 
					  (VOID *)&iKeepAliveInterval,
					  (INT)sizeof(iKeepAliveIdel));
	iRet |= setsockopt(iConnFd, 
					  SOL_TCP, 
					  TCP_KEEPCNT, 
					  (VOID *)&iKeepAliveCount,
					  (INT)sizeof(iKeepAliveCount));
	if (0 > iRet)
	{
		ERR_PRINTF("SO_KEEPALIVE arg set Failed!");
        return ERROR_FAILE;
	}
	
	/* connect with SSL */
	pstSSL = CLIENT_SSL_link_create(iConnFd);
	if (NULL == pstSSL) {
		ERR_PRINTF("client SSL link create Failed!");
		return ERROR_FAILE;
	}

	if (CLIENT_SSL_link_connect(pstSSL) ==  ERROR_LINK_SSL_WANT_READ) {
		/* 因为socket为非阻塞，不一定握手完成  */
		ulErrCode = THREAD_epoll_EventMod(pstThrdEpEvt->iEventFd,
				EPOLLIN|EPOLLET|EPOLLERR,
				pstThrdEpEvt->iThreadId,
				msg_client_SSL_conn_EpCallBack,
				pstSSL);
		if (ERROR_SUCCESS != ulErrCode)
		{
			ERR_PRINTF("THREAD epoll EventMod Failed!");
			return ulErrCode;
		}
	}

    return ERROR_SUCCESS;

	/* 待升级为 不该起线程，因为操作socket与链接线程由资源共享问题 
	ulErrCode = CLIENT_work_ThreadStart(pstThrdEpEvt->iEventFd);
	if (ERROR_SUCCESS != ulErrCode)
	{
		return ulErrCode;
	}
    */

    //test
    pstThrd = NULL;
    do {
        pstThrd = Thread_server_GetNext(pstThrd);
    }while (pstThrd != NULL && 
            pstThrd->eThreadType != THREAD_TYPE_WORK);
            
    DBGASSERT(NULL != pstThrd);

    if (-1 == pstThrd->stThrdQueMsgInfo.iThrdQueMsgEventFd)
    {
		ERR_PRINTF("iThrdQueMsgEventFd = -1!");
        return ERROR_SUCCESS;
    }
    
    MSG_PRINTF("Thread(%d) to Thread (%d)",
               pstThrdEpEvt->iThreadId, pstThrd->iThreadID);
               
    MSG_PRINTF("event fd = %d", pstThrd->stThrdQueMsgInfo.iThrdQueMsgEventFd);

    int i = 0;
    INT iSrcThrdId;
    THREAD_QUEMSG_DATA_S stThrdQueMsg;
    THREAD_QUEMSG_DATA_S stRecvThrdQueMsg;
    char *pstData = NULL;
    mem_set0(&stThrdQueMsg, sizeof(THREAD_QUEMSG_DATA_S));
    for(;i < 4;i ++)
    {
		pstData = mem_alloc(100);
		DBGASSERT(NULL != pstData);
        sprintf(pstData, "This is Test Msg--%d", i);
        MSG_PRINTF("Send Test Msg -- %d ", i);
        stThrdQueMsg.uiQueMsgDataLen = 25;
        stThrdQueMsg.uiQueMsgType =  1;
        stThrdQueMsg.pQueMsgData = pstData;
        //THREAD_server_QueMsg_Send(pstThrd->iThreadID,&(stThrdQueMsg));
        THREAD_server_QueMsg_SendWithResp(pstThrd->iThreadID,
        								  &(stThrdQueMsg),
        								  &iSrcThrdId,
        								  &stRecvThrdQueMsg);
		MSG_PRINTF("===Revc Resp msg = [%s] from Thread(%d)", 
					(CHAR *)stRecvThrdQueMsg.pQueMsgData,
					iSrcThrdId);

		
		free(stRecvThrdQueMsg.pQueMsgData);
		stRecvThrdQueMsg.pQueMsgData = NULL;
        								  
    }    

    /////////////*
	/*
    char ttsetBuf[64];
    for (i = 0; i < 100; i++)
    {
    	sprintf(ttsetBuf , "MSG index = (%d)\n", i);
    	printf(".");
		send(iConnFd, ttsetBuf, strlen(ttsetBuf), 0);
		usleep(1000);
    }
	printf("\n");
	*/
	//INT iTimerId = 0;
	ULONG ulPara[4] = {0};
	ulPara[0] = (ULONG)iConnFd;
	TIMER_CreateForThread(pstThrdEpEvt->iThreadId,
  								     1,
  								     TimerCB_SentIPinfo,
  								     ulPara);

	/*
    for (i = 0; i < 100; i++){
		SentIPInfo(iConnFd);
		usleep(1000);
	}
	*/


	return ERROR_SUCCESS;
}

VOID msg_client_LinkIsBreak(IN INT iClientfd, IN INT iThreadId)
{
    THREAD_INFO_S *pstThrdInfo = NULL;
    
    pstThrdInfo = Thread_server_GetByThreadId(iThreadId);
    DBGASSERT(NULL != pstThrdInfo);
    
    Thread_server_EpollDel(iClientfd,
                           pstThrdInfo->pstThrdEp);

    (VOID)msg_client_ConnInit(pstThrdInfo);
    
    return;
}

ULONG msg_client_Recv(IN INT iClientfd , IN INT iThreadId)
{
	INT   iRecvMsgLen = 0;
	UCHAR  aucDataBuf[NSDB_MSG_LEN_MAX];
	MSG_CLT_LINK_ST *pstCltLink = NULL;

	pstCltLink = MSG_client_link_Get();

	//iRecvMsgLen = recv(iClientfd, aucDataBuf,sizeof(aucDataBuf),0);
	iRecvMsgLen = CLIENT_MSG_recv(pstCltLink, aucDataBuf,sizeof(aucDataBuf));

	MSG_PRINTF("recv [errno = %d]", errno);
	
	if (ETIMEDOUT == errno)
	{
		perror("Recv!");
        ERR_PRINTF("link is break ! ETIMEDOUT!");

        msg_client_LinkIsBreak(iClientfd, iThreadId);
        
        return ERROR_SUCCESS;
	}
	
    if (iRecvMsgLen > 0)
    {
        //(VOID)Client_Recv((INT)iRecvMsgLen, (VOID *)aucDataBuf);
        (VOID)MSG_Client_RECV_HandleMsg(iClientfd, aucDataBuf, iRecvMsgLen);
	}
	else if (0 == iRecvMsgLen)
	{
        ERR_PRINTF("Server Has ShutDown!");
        ERR_PRINTF("errNo = %d", errno); 
        msg_client_LinkIsBreak(iClientfd, iThreadId);
    }
    else
    {
        ERR_PRINTF("-- Recv ERROR! --");
        
    }

	DBG_PRINT_LOG("ClientRecvMSG.bat", (CHAR *)aucDataBuf, iRecvMsgLen);

	return ERROR_SUCCESS;

}

ULONG msg_client_SSL_conn_EpCallBack(IN UINT events, IN VOID *arg)
{
	INT   iRet  = 0;
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;
	SSL *pstSSL;

	DBGASSERT(NULL !=  arg);
	
	MSG_PRINTF("ssl conn EP CallBack!");
	
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	
	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		pstSSL = (SSL *)(pstThrdEpEvents->pPriData);
		DBGASSERT(NULL != pstSSL);
		iRet = CLIENT_SSL_link_connect(pstSSL);
		if (0 == iRet) {
			MSG_PRINTF("client SSL link create SUCCESS!");
			ulErrCode = msg_client_ConnSuccess2(pstThrdEpEvents, pstSSL);
		} else {
			if (iRet ==  ERROR_LINK_SSL_WANT_READ) {
			} else {
				MSG_PRINTF("client SSL link create Failed!");
				ulErrCode = ERROR_FAILE;
			}
		}
	}

	return ulErrCode;
}

/*
 * connect 时间处理
 * */
ULONG msg_client_EpCallBack(IN UINT events, IN VOID *arg)
{
	INT   iRet  = 0;
	INT   error = 0;
	INT   len   = sizeof(error);
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents;

	DBGASSERT(NULL !=  arg);
	
	MSG_PRINTF("msg_client_EpCallBack!");
	
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	if ((events & EPOLLOUT) && (pstThrdEpEvents->uiEvents & EPOLLOUT))
	{
		iRet = getsockopt(pstThrdEpEvents->iEventFd, 
							SOL_SOCKET, 
							SO_ERROR, 
							&error, 
							(socklen_t *)&len);
		if (0 != iRet || 0 != error)
		{
			MSG_PRINTF("EPOLL get EPOLLOUT but connect UnSuccess!");
			return ERROR_SUCCESS;
		}
		
		MSG_PRINTF("Server Connect Success!");

		ulErrCode = msg_client_ConnSuccess(pstThrdEpEvents);
	}
	
	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode |= msg_client_Recv(pstThrdEpEvents->iEventFd, 
		                             pstThrdEpEvents->iThreadId);
	}

	
	if ((events & EPOLLHUP) && (pstThrdEpEvents->uiEvents & EPOLLHUP))
	{
		ERR_PRINTF("Server Connect is Break!");
		return ERROR_FAILE;
		
	}
	
	return ulErrCode;
	
}

INT MSG_client_CreatSocket(VOID)
{	
    INT   iRet      = 0;
    INT   iNbio     = 1;
	INT   iClientfd = 0;
    ULONG ulAsync   = 1;
	
	iClientfd = socket(AF_INET, SOCK_STREAM, 0);
	if(MSG_SOCKET_INVALID_FD == iClientfd)
	{
		ERR_PRINTF("Client socet create failed!");
		return MSG_SOCKET_INVALID_FD;
	}

	iRet = ioctl(iClientfd, FIONBIO, &iNbio);
	if(0 > iRet)
	{
		ERR_PRINTF("set FIONBIO failed!");
		return MSG_SOCKET_INVALID_FD;
	}

	iRet = ioctl(iClientfd, FIOASYNC, &ulAsync);
	if(0 > iRet)
	{
		ERR_PRINTF("set FIOASYNC failed!");
		return MSG_SOCKET_INVALID_FD;
	}
	return iClientfd;
}

ULONG 
MSG_client_CreateLink(IN INT iClientfd, IN INT *piConnFd)
{
    /* UINT iRet;
	struct sockaddr_in stClientAddr; */
	struct sockaddr_in stServerAddr;

	/*
	bzero(&stClientAddr, sizeof(struct sockaddr_in));
	stClientAddr.sin_family = AF_INET;
	stClientAddr.sin_addr.s_addr = inet_addr(NSDB_MSG_CLIENT_ADDR);
	stClientAddr.sin_port = 0;
	iRet = bind(iClientfd, (struct sockaddr *)&stClientAddr, sizeof(struct sockaddr_in));
	if(-1 == iRet)
	{
		ERR_PRINTF("socket bind failed!");
		return ERROR_FAILE;
	}
	*/
	
	bzero(&stServerAddr, sizeof(struct sockaddr_in));
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_addr.s_addr = inet_addr(NSDB_MSG_SERVER_ADDR);
	stServerAddr.sin_port = htons(NSDB_MSG_LISTEN_PORT);
    *piConnFd = connect(iClientfd, 
	   					  (struct sockaddr *)&stServerAddr, 
	   					  sizeof(struct sockaddr));
	if(0 > (*piConnFd) && (EINPROGRESS != errno))
	{
		ERR_PRINTF("connetc failed! iConnFd = %d errno = %d", 
				   (*piConnFd), errno);
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}

/*
	connect 还是需要加入EPOLL
	在超时后要摘出来，close,重新创建
*/

ULONG msg_client_LinkTimerOut(INT iTimerId, VOID *arg)
{
	INT   iClientSockfd = 0;
	INT   iThreadId     = 0;
	THREAD_INFO_S *pstThreadInfo = NULL;

	//pstTimerData = (TIMER_DATA_S *)arg;
	iThreadId = ((ULONG *)arg)[0];
	iClientSockfd = ((ULONG *)arg)[1];
	pstThreadInfo = Thread_server_GetByThreadId(iThreadId);
	DBGASSERT(NULL != pstThreadInfo);
	
	MSG_PRINTF("msg client Link Timer IS Out !");

	TIMER_DeleteForThread(iThreadId, iTimerId);

	MSG_PRINTF("old socket fd = [%d]", iClientSockfd);
	THREAD_epoll_EventDel(iClientSockfd, pstThreadInfo->pstThrdEp);
	
	MSG_PRINTF("connect Server again !");
	iClientSockfd = msg_client_ConnInit(pstThreadInfo);
	if(MSG_SOCKET_INVALID_FD == iClientSockfd)
	{
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}

INT msg_client_TryToConnect(VOID)
{
	ULONG ulRet         = 0;	
    INT   iConnFd       = 0;
	INT   iClientSockfd = 0;
	
	iClientSockfd = MSG_client_CreatSocket();
	if(MSG_SOCKET_INVALID_FD == iClientSockfd)
	{
		return MSG_SOCKET_INVALID_FD;
	}
	ulRet = MSG_client_CreateLink(iClientSockfd, &iConnFd);
	if(ERROR_SUCCESS!= ulRet)
	{
	    close(iClientSockfd);
		return MSG_SOCKET_INVALID_FD;
	}
	return iClientSockfd;
}

STATIC ULONG msg_client_ConnInit(IN THREAD_INFO_S *pstThreadInfo)
{
	ULONG ulRet 		= 0;	
	INT   iClientSockfd = 0;
	
	iClientSockfd = msg_client_TryToConnect();
	if(0 > iClientSockfd)
	{
		ERR_PRINTF("TryToConnect Failed");
		goto error_TryToConnect;
	}
	MSG_PRINTF("Client connect create success! But it does not mean can Use!");

	ulRet = Thread_server_EpollAdd(pstThreadInfo->iThreadID,
								   iClientSockfd, 
								   EPOLLIN|EPOLLOUT|EPOLLET|EPOLLHUP|EPOLLERR,
								   msg_client_EpCallBack);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Thread_server_EpollAdd Failed");
		goto error_epoll_add;
	}

    /* 如果超时时间之内没有连接成功，重新connect */
    //TODO:USE A thread do with evendfd and timerfd.!!
	ulRet = THREAD_server_TimerCreateForEvt(pstThreadInfo->iThreadID, 
										    iClientSockfd,
										    1,
										    msg_client_LinkTimerOut);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("THREAD epoll Timer Create For Event Failed");
		goto error_timer_create;
	}
	
	return ERROR_SUCCESS;
	
error_timer_create:
    Thread_server_EpollDel(iClientSockfd, 
                           pstThreadInfo->pstThrdEp);
error_epoll_add:
    close(iClientSockfd);
error_TryToConnect:

    return ERROR_FAILE;
}


ULONG msg_client_ConnThreadInit(VOID *arg)
{
	ULONG ulRet 		= 0;	
	THREAD_INFO_S *pstThreadInfo;

	DBGASSERT(NULL != arg);

	pstThreadInfo = (THREAD_INFO_S *)arg;

	MSG_PRINTF("---- MSG server Thread Init start ----");

	ulRet = msg_client_ConnInit(pstThreadInfo);
	
	MSG_PRINTF("---- MSG server Thread Init End ----");

	return ulRet;
	
}

STATIC int g_iThreadIdLink = 0;
STATIC int g_iThreadIdWork = 0;

ULONG DataBase_client_init(VOID)
{
    CLT_sm_init();
    
	/* 链接线程，负责和Server进程的链接和数据交互 */
	/*  */
    g_iThreadIdLink = Thread_server_CreateWithEpQMsg(msg_client_ConnThreadInit,
            NULL,
            THREAD_TYPE_LINK,
            msg_client_QueMsgRecv);
    if (-1 == g_iThreadIdLink)
    {
        ERR_PRINTF("Create THREAD WORK CLIENT Failed");
        return ERROR_FAILE;
    }
    DBG_THRD_NAME_REG(g_iThreadIdLink, "LinkServer");

	/* 工作线程，负责组要的工作 */
    g_iThreadIdWork = Thread_server_CreateWithEpQMsg(NULL,
    												 NULL,
                                                     THREAD_TYPE_WORK,
                                                     msg_clientserver_QueMsgRecv);
    if (-1 == g_iThreadIdLink)
    {
        ERR_PRINTF("Create THREAD WORK CLIENT Failed");
        return ERROR_FAILE;
    }
    DBG_THRD_NAME_REG(g_iThreadIdWork, "LinkWork");

	return ERROR_SUCCESS;
}

VOID DataBase_client_Fini(VOID)
{
	Thread_server_KillByThrdID(g_iThreadIdLink);
	Thread_server_KillByThrdID(g_iThreadIdWork);
	
	return;
}


