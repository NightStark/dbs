#ifndef __MSG_SERVER_C__
#define __MSG_SERVER_C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <asm/errno.h>
#include <arpa/inet.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_epoll.h>
#include <ns_thread.h>
#include <ns_sm.h>
#include <ns_msg.h>
#include <ns_server.h>
#include <ns_ssl.h>

#define SERVER_LINK_KEEPALIVE_IDEL  (50)
#define SERVER_LINK_KEEPALIVE_INTVL (1)
#define SERVER_LINK_KEEPALIVE_CNT   (5)

EPOLL_EVENTS_S *g_pstEpollEventsServer = NULL;
INT 				g_epollFdServer = 0;
	 
VOID msg_server_QueMsgRecv(IN INT iThreadId, IN const THREAD_QUEMSG_DATA_S *pstT)
{
    MSG_PRINTF("**** Recv Que MSg ****");
}


/* 数据到来回调 */
ULONG Thread_server_RecvData(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents = NULL;
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;
	THREAD_INFO_S *pstThrd = NULL;
    
	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode = MSG_RECV_RecvData(pstThrdEpEvents->iEventFd, events, arg);
	}

    /*  Client has Close The Socket link , We Del the listen in epoll
        to do not work for the client 
    */
    if (ERROR_LINK_CLIENT_DOWN == ulErrCode)
    {
        pstThrd = Thread_server_GetByThreadId(pstThrdEpEvents->iThreadId);
        DBGASSERT(NULL != pstThrd);
        
        /* 释放link */
        MSG_server_link_Down(pstThrdEpEvents->iEventFd);

        Thread_server_EpollDel(pstThrdEpEvents->iEventFd,
                               pstThrd->pstThrdEp);
    }

    /* Link shoud not affect the thread work! */
	return ERROR_SUCCESS;
}

ULONG MSG_server_CreateLink(IN INT iSockFd)
{
	INT 		iRet				= 0;
	ULONG 		ulRet 				= 0;
	INT 		iConnFd				= -1;
	INT			iSetKeepAlive 		= 0;
	INT			iKeepAliveIdel 		= 0;
	INT			iKeepAliveInterval 	= 0;
	INT			iKeepAliveCount 	= 0;
	socklen_t 	cliaddr_len;
	struct sockaddr_in cliaddr;
    SSL             *pstSSL     = NULL;
	THREAD_INFO_S   *pstThrdSrv = NULL;
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    cliaddr_len = sizeof(struct sockaddr_in);
    memset(&cliaddr, 0, sizeof(cliaddr));
    iConnFd = accept(iSockFd, (struct sockaddr *)&cliaddr, &cliaddr_len);
    if(-1 == iConnFd)
    {
        ERR_PRINTF("Link create failed!");
        return ERROR_FAILE;
    }

	MSG_PRINTF("Link create SUCCESS!");
	//ERR_PRINTF("Accept Client IP = [%s]",inet_ntoa(cliaddr.sin_addr.s_addr));

	/* 设置 链接保活探测 */
	iSetKeepAlive = 1;
	iRet = setsockopt(iConnFd, 
					  SOL_SOCKET, 
					  SO_KEEPALIVE, 
					  (VOID *)&iSetKeepAlive,
					  (INT)sizeof(iSetKeepAlive));
	if (0 > iRet)
	{
		MSG_PRINTF("SO_KEEPALIVE set Failed!");
		goto err;
	}

	/*
		TCP_KEEPIDLE 	指定多长时间后发起探测
		TCP_KEEPINTVL 	指定每次探测相隔多长时间
		TCP_KEEPCNT		指定探测次数
		超过探测次数后，epoll上触发EPOLLIN时间，
			recv 返回 0 或 <0， errno == ETIMEDOUT
	*/
	iKeepAliveIdel 		= SERVER_LINK_KEEPALIVE_IDEL;
	iKeepAliveInterval 	= SERVER_LINK_KEEPALIVE_INTVL;
	iKeepAliveCount 	= SERVER_LINK_KEEPALIVE_CNT;
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
		goto err;
	}
					  
	/* 创建线程，专门服务于某一客户端的数据处理(update to muti client) 
	iThreadId = Thread_server_CreateWithEpQMsg(NULL,0);
	if (0 > iThreadId)
	{
		return ERROR_FAILE;
	}*/
    pstThrdSrv = Thread_server_GetNext(NULL);
	while (NULL != pstThrdSrv &&
    THREAD_TYPE_WORK_SERVER != pstThrdSrv->eThreadType)
	       
	{
        pstThrdSrv = Thread_server_GetNext(pstThrdSrv);
	}

	if (NULL == pstThrdSrv)
	{
		ERR_PRINTF("THREAD WORK SERVER is not exit!!");
		goto err;
	}

	/* create SSL LINK */
	pstSSL = SERVER_SSL_link_create(iConnFd);
	if (NULL == pstSSL) {
		ERR_PRINTF("SSL link create failed.");
		goto err;
	}

	MSG_PRINTF("[fd:%d]SSL link create SUCCESS.", iConnFd);
    /*
     * Create and start a sm for this link
     * */
    pstSrvLink = MSG_server_link_Up(iConnFd, pstThrdSrv->iThreadID);
    if (NULL == pstSrvLink) {
        ERR_PRINTF("create link failed.");
		goto err;
    }

	pstSrvLink->pstSSL = pstSSL;
    //pstSrvLink->stClientInfo.unClientAddr = cliaddr;
    memcpy(&(pstSrvLink->stClientInfo.unClientAddr), &cliaddr, sizeof(pstSrvLink->stClientInfo.unClientAddr));

	ulRet = Thread_server_EpollAdd(pstThrdSrv->iThreadID, 
	 					   	       iConnFd, 
	 					           EPOLLIN|EPOLLET,
						           Thread_server_RecvData);

	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Thread_server_EpollAdd FAILED!!");
	}
	else
	{
		MSG_PRINTF("Link (fd:%d) work at Thread(id:%d)!!", iConnFd,pstThrdSrv->iThreadID);
	}

	return ulRet;
err:
	if (pstSSL != NULL) {
		SERVER_SSL_link_destroy(pstSSL);
		pstSSL = NULL;
	}
	if (iConnFd > 0) {
		close(iConnFd);
		iConnFd = -1;
	}

    return ERROR_FAILE;
}

INT MSG_CreateSocket(VOID)
{
	INT iRet = 0;
	INT fd   = -1;
	INT ReUseAddr = BOOL_TRUE;
	struct sockaddr_in servaddr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == fd)
	{
		ERR_PRINTF("socket create failed!");
	}

    // set nonblocking
    iRet = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(iRet < 0)
    {
        ERR_PRINTF("fcntl nonblocking failed!");
    }


	iRet = setsockopt(fd, 
	  					SOL_SOCKET, 
	  					SO_REUSEADDR, 
	  					&ReUseAddr, 
	  					sizeof(ReUseAddr));    
	if (0 > iRet)
	{
        ERR_PRINTF("setsockopt SO_REUSERADDR failed!");
	}
	
	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(NSDB_MSG_LISTEN_ADDR);
	servaddr.sin_port = htons(NSDB_MSG_LISTEN_PORT);
	
	iRet = bind(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
	if(-1 == iRet)
	{
		ERR_PRINTF("socket bind failed! errno = %d", errno);
		perror("MSG_CreateSocket bind");
		return -1;
	}
	
	iRet = listen(fd, NSDB_MSG_LISTEN_WAIT_MAX);
	if(-1 == iRet)
	{
		ERR_PRINTF("socket listen failed!");		
		return -1;
	}

	return fd;
}

ULONG MSG_server_MainThreadCallBack(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents = NULL;
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode = MSG_server_CreateLink(pstThrdEpEvents->iEventFd);
	}

	return ulErrCode;
}


ULONG MSG_server_Thread_Init(VOID *arg)
{
    INT iSockFd;
    ULONG ulRet;
	THREAD_INFO_S *pstThrdSrv = NULL;

	DBGASSERT(NULL != arg);

	pstThrdSrv = (THREAD_INFO_S *)arg;

	MSG_PRINTF("MSG server Thread Init start...");
    
    iSockFd = MSG_CreateSocket();
    if (0 > iSockFd)
    {
        ERR_PRINTF("MSG_CreateSocket Failed!");
        return ERROR_FAILE;
    }
    
    ulRet = Thread_server_EpollAdd(pstThrdSrv->iThreadID,
                                    iSockFd,
                                    EPOLLIN|EPOLLET,
                                    MSG_server_MainThreadCallBack);
    if (ERROR_SUCCESS != ulRet)
    {
        ERR_PRINTF("Thread_server_EpollAdd Failed!");
        return ERROR_FAILE;
    }
    
	MSG_PRINTF("MSG server Thread Init end...");
	
    return ERROR_SUCCESS;
}

ULONG MSG_server_Init(VOID)
{
    INT iIndex;
    INT iThreadId;

    SRV_sm_Init();
    
	for(iIndex = 0; iIndex < 4; iIndex++)
    {
        iThreadId = Thread_server_CreateWithEpQMsg(NULL, 
        										   NULL,
	                                               THREAD_TYPE_WORK_SERVER,
	                                               msg_server_QueMsgRecv);
        if (-1 == iThreadId)
        {
            ERR_PRINTF("work-server Thread Create Failed!");
            return ERROR_FAILE;
        }
		DBG_THRD_NAME_REG(iThreadId, "Server-Work-%d", iIndex);
    }
    
    iThreadId = Thread_server_CreateWithEpQMsg(MSG_server_Thread_Init, 
    										   NULL,
	                                           THREAD_TYPE_MAIN_SERVER,
	                                           msg_server_QueMsgRecv);
    if (-1 == iThreadId)
    {
        ERR_PRINTF("MAIN Thread Create Failed!");
        return ERROR_FAILE;
    }

	DBG_THRD_NAME_REG(iThreadId, "Server-Main");

    while(1) sleep(100);
    return ERROR_SUCCESS;
}

#if 0 //废弃************************************************
ULONG _MSG_server_Init()
{
	INT iSockFd     = -1;
    INT iEpWaitFd   = -1;
    INT iIndex      = 0;
    INT iEWIndex    = 0;
	EPOLL_EVENTS_S *pstEpollEvents = NULL;
    EPOLL_EVENT_S      *pstEpEvtGet    = NULL;


	/* 创建进程全局EPOLL池 */
    g_pstEpollEventsServer = mem_alloc(sizeof(EPOLL_EVENTS_S) * MSG_EPOLL_MAX_EVENTS);
    g_epollFdServer = Epoll_Create(MSG_EPOLL_MAX_EVENTS);
	
	/* 创建Socket */
	iSockFd = MSG_CreateSocket();
	
	/* 设置EPOLL数据 */
	iIndex = msg_epoll_GetIdelNode(g_pstEpollEventsServer);
    Epoll_EventSet(iSockFd, 
						(VOID *)MSG_server_CreateLink, 
					    &g_pstEpollEventsServer[iIndex]);
	
	/* 把Socket FD加入EPOLL池，监听所有的connet事件 */
    Epoll_EventAdd( g_epollFdServer,
    				   	EPOLLIN|EPOLLET, 
    				   	&g_pstEpollEventsServer[iIndex]);
	
    /* 超时处理 */
    //... ...

    /* wait  */
    pstEpEvtGet  = mem_alloc(sizeof(EPOLL_EVENT_S)  * MSG_EPOLL_MAX_EVENTS);
	while(1)
	{
		iEpWaitFd = epoll_wait(g_epollFdServer, pstEpEvtGet, MSG_EPOLL_MAX_EVENTS, -1);
		if(iEpWaitFd < 0){
			ERR_PRINTF("epoll_wait Error!");
			break;
		}
		
		/* Listen 事件 和 Receive 事件都会在这 被CallBack  */
		for (iEWIndex = 0; iEWIndex < iEpWaitFd; iEWIndex++)
		{
			pstEpollEvents = (EPOLL_EVENTS_S *)pstEpEvtGet[iEWIndex].data.ptr;
			if ((pstEpEvtGet[iEWIndex].events & EPOLLIN) && (pstEpollEvents->events & EPOLLIN))
			{
				((EPOLL_CALL_BACK)pstEpollEvents->call_back)(pstEpollEvents->fd, 
	 														   pstEpEvtGet[iEWIndex].events, 
	 														   pstEpollEvents->arg); 
			}
		}
	}

    free(pstEpEvtGet);

    return ERROR_SUCCESS;
}
#endif

ULONG MSG_server_Fint()
{
	
	Epoll_Destroy(g_pstEpollEventsServer);
	
	return ERROR_SUCCESS;
}

#endif //__MSG_SERVER_C__
