#ifndef	__SOCKET_C__
#define __SOCKET_C__

#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>
#include <asm/errno.h>
#include <sys/signalfd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_base.h>
#include <ns_bitmap.h>
#include <ns_ab.h>
#include <ns_avl.h>
#include <ns_web_server.h>
#include <ns_thread.h>

#include <ns_http.h>
#include <http_parser.h>
#include <ns_websetting.h>
#include <ns_web_action.h>
#include <ns_web.h>

/* 创建socket 并监听，返回listen 的 fd */
STATIC INT web_server_CreateSocket(VOID)
{
	INT iRet = 0;
	INT ReUseAddr = BOOL_TRUE;	ULONG ulRet;
	int listenfd;
	struct sockaddr_in servaddr;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == listenfd)
	{
		ERR_PRINTF("socket create failed!");
		return -1;
	}

    // set nonblocking
    iRet = fcntl(listenfd, F_SETFL, O_NONBLOCK);
    if(iRet < 0)
    {
        ERR_PRINTF("fcntl nonblocking failed!");
    }

	iRet = setsockopt(listenfd, 
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
	/* INADDR_ANY (0.0.0.0)任意地址 */
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(WEB_SERV_PORT);
	
	ulRet = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
	if(-1 == ulRet)
	{
		ERR_PRINTF("socket bind failed!");
		return -1;
	}
	
	ulRet = listen(listenfd, WEB_SER_LISTEN_MAX);
	if(-1 == ulRet)
	{
		ERR_PRINTF("socket listen failed!");
		return -1;
	}

	return listenfd;
}

/*****************************************************************************
 Prototype    : _web_server_HttpReq_recv
 Description  : 接收 http连接请 求发来的数据
 Input        : INT connfd  accept socket fd
 Output       : None
 Return Value : VOID
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG _web_server_HttpReq_recv(IN INT connfd)
{
	ULONG ulRet;
	CHAR  buf[MAXLINE];
	ssize_t ulHttpHeadSize = 0;
	WEB_HTTP_REQMSGINFO_S stHttpReqMsgInfo;

	mem_set0(&stHttpReqMsgInfo, sizeof(WEB_HTTP_REQMSGINFO_S));

	MSG_PRINTF("Connect Fd = [%d]", connfd);

#ifdef DEBUG_ON
	THREAD_INFO_S *pstThrd = NULL;
	pstThrd = Thread_server_GetCurrent();
	DBGASSERT(NULL != pstThrd);
#endif
	SHOW_PRINTF("Thread[%s] is handled !\n"
	            "           Tid:%-3d ptid:0%0X TType:%-3d", 
				DBG_THRD_NAME_GET(pstThrd->iThreadID),
				pstThrd->iThreadID, 
				(UINT)pstThrd->pThreadID, 
				pstThrd->eThreadType);
		
	/* 接收报文 */
	ulHttpHeadSize = recv(connfd, buf, MAXLINE, 0);//read(connfd, buf, MAXLINE);
	if (-1 == ulHttpHeadSize)
	{
		ERR_PRINTF("Reqest Accept Read Failed(%s)!", strerror(errno));
		return ERROR_FAILE;
	}
	
	DBG_PRINT_LOG("reqHead.dat", buf, ulHttpHeadSize);
	
	MSG_PRINTF("Read Http Head Success!");

	#if 0 /* http parser 库，感觉很牛逼，但不知道到底怎么用，有空瞧瞧 */
	size_t nparsed;
	http_parser_settings settings;
	settings.on_url = ns_http_data_cb_url;
	settings.on_header_field = ns_http_data_cb_field;
	/* ... */

	http_parser *parser = malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	parser->data = (VOID *)connfd;


	nparsed = http_parser_execute(parser, &settings, buf, ulHttpHeadSize);

	if (parser->upgrade) {
	  ERR_PRINTF(" handle new protocol ");
	} else if (nparsed != ulHttpHeadSize) {
	  ERR_PRINTF(" Handle error. Usually just close the connection. ");
	}
	#endif
	
	ulRet = WEB_http_HandleHttpReq(connfd, buf,ulHttpHeadSize);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Handle Http Request Failed!");
	}

	close(connfd);

	return ERROR_SUCCESS;
}

ULONG Web_server_accept_http_init_cb(VOID *args)
{
	MSG_PRINTF("init thread success!");

	return ERROR_SUCCESS;
}

/* do receive http request data */
ULONG web_server_HttpReq_recv(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents = NULL;

	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;
	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode = _web_server_HttpReq_recv(pstThrdEpEvents->iEventFd);
	}

	return ulErrCode;
}

/*****************************************************************************
 Prototype    : web_server_AcceptHttp
 Description  : 接收处理，http连接请求，
 				现在的处理是为每一的请求建立一个线程，可升级为队列优先级机制
 Input        : IN INT iListenFd  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG web_server_AcceptHttp(IN INT iListenFd)
{
	//INT iRet  = 0;
	INT ulRet = 0;
	INT connfd;
	socklen_t cliaddr_len;
	struct sockaddr_in  cliaddr;
	THREAD_INFO_S *pstThrdInfo = NULL;

	MSG_PRINTF("ACCEPT(%d)....", iListenFd);
	cliaddr_len = sizeof(struct sockaddr_in);

	/* accept http request */
	memset(&cliaddr, 0, sizeof(struct sockaddr_in));
	connfd = accept(iListenFd, (struct sockaddr *)&cliaddr, &cliaddr_len);
	if(-1 == connfd)
	{
		ERR_PRINTF("accept failed! [errno] = %d", errno);
		return ERROR_FAILE;
	}

#if 0
	/* 每个进程在当前环境智能创建380个，待升级为调度方案 */
	iRet = Thread_server_CreatWithMain(THREAD_TYPE_WEB_SERVER_WROK,
  									   web_server_ReqAccept,
  									   (VOID *)(ULONG)(connfd));
	if(iRet < 0)
	{
		ERR_PRINTF("thread create failed!");
	}
	DBG_THRD_NAME_REG(iRet, "WServerWork-%d", iRet);
#endif



	/* recv data will do in another thread */
	pstThrdInfo = Thread_server_GetByThreadType(THREAD_TYPE_WEB_SERVER_WROK_RECV);
	if (NULL == pstThrdInfo) {
		ERR_PRINTF("get thread by type failed!");
		return ERROR_FAILE;
	}

	//TODO: need set noblock?

	ulRet = Thread_server_EpollAdd(pstThrdInfo->iThreadID,
								   connfd,
								   EPOLLIN|EPOLLET,
								   web_server_HttpReq_recv);

	return ulRet;
		
}

/*****************************************************************************
 Prototype    : web_server_MainThreadCallBack
 Description  : WEB主线程,接收到http连接请求后的回调函数
 Input        : IN UINT events  触发事件
                IN VOID *arg    事件描述集合
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG web_server_MainThreadCallBack(IN UINT events, IN VOID *arg)
{
	ULONG ulErrCode = ERROR_SUCCESS;
	THREAD_EPOLL_EVENTS_S *pstThrdEpEvents = NULL;
	pstThrdEpEvents = (THREAD_EPOLL_EVENTS_S *)arg;

	if ((events & EPOLLIN) && (pstThrdEpEvents->uiEvents & EPOLLIN))
	{
		ulErrCode = web_server_AcceptHttp(pstThrdEpEvents->iEventFd);
	}

	return ulErrCode;
}

/*****************************************************************************
 Prototype    : web_server_Thread_Init
 Description  : WEB主线程初始化
 				创建socket, 启动EPOLL监听
 Input        : VOID *arg  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG web_server_Thread_Init(VOID *arg)
{
    INT iSockFd;
    ULONG ulRet;
	THREAD_INFO_S *pstThrdSrv = NULL;

	DBGASSERT(NULL != arg);

	pstThrdSrv = (THREAD_INFO_S *)arg;

	MSG_PRINTF("WEB server Thread Init start...");
    
	iSockFd = web_server_CreateSocket();
	if (0 > iSockFd)
	{
		return 1;
	}
	
    ulRet = Thread_server_EpollAdd(pstThrdSrv->iThreadID,
                                   iSockFd,
                                   EPOLLIN|EPOLLET,
                                   web_server_MainThreadCallBack);
    if (ERROR_SUCCESS != ulRet)
    {
        ERR_PRINTF("Thread_server_EpollAdd Failed!");
        return ERROR_FAILE;
    }
    
	MSG_PRINTF("Socket is Listen!");
	MSG_PRINTF("WEB server Thread Init end...");
	
    return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : web_server_QueMsgRecv
 Description  : WEB主线程，线程间消息接收回调
 Input        : IN INT sss                            
                IN const THREAD_QUEMSG_DATA_S *pstss  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC VOID web_server_QueMsgRecv(IN INT sss,IN const THREAD_QUEMSG_DATA_S *pstss)
{


    return;
}

/*****************************************************************************
 Prototype    : Web_server_init
 Description  : WEB服务初始化
 Input        : VOID  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
INT Web_server_init(VOID)
{
	INT iThreadID;
	/* 创建WEB http 监听主线程 */
	iThreadID = Thread_server_CreateWithEpQMsg(web_server_Thread_Init, 	
											   NULL,
											   THREAD_TYPE_WEB_SERVER,
											   web_server_QueMsgRecv);
	if (iThreadID < 0)
	{
		ERR_PRINTF("Create Web Server Thread Failed!");
		return -1;
	}
	DBG_THRD_NAME_REG(iThreadID, "Web-Server");
	
	iThreadID = Thread_server_CreateWithEpQMsg(Web_server_accept_http_init_cb,
											   NULL,
											   THREAD_TYPE_WEB_SERVER_WROK_RECV,
											   web_server_QueMsgRecv);
	DBG_THRD_NAME_REG(iThreadID, "Web-Server-Work-recv.");

	return 0;
}

#endif //__SOCKET_C__

