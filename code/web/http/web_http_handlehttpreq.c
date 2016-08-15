#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <ns_base.h>
#include <ns_web_server.h>
#include <ns_http.h>

/*****************************************************************************
 Prototype    : web_http_SendBigFile
 Description  : 分拨发送 大文件（>4k）
 Input        : IN INT   connfd   		请求者fd    
                IN CHAR *pucFilePath    请求文件的路径
                IN CHAR *buf            借用调用者提供的buffer
                IN ULONG ulBufSize      调用者提供buffer的大小
                IN ULONG ulFileSize     请求文件的大小
 Output       : None
 Return Value : STATIC
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/1
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC LONG web_http_SendBigFile(IN INT   connfd,
								 IN CHAR *pucFilePath, 
								 IN CHAR *buf, 
								 IN ULONG ulBufSize, 
								 IN ULONG ulFileSize)
{
	INT  fd;
	INT  len;
	UINT iRet;
	LONG lSendDataLen = 0;
	
	/* open file */
	fd = open(pucFilePath, O_RDONLY);
	if(-1 == fd)
	{
		ERR_PRINTF("Open request file :%s failed!", pucFilePath);
		return ERROR_FAILE;
	}

	while(1)
	{
		len = read(fd, buf, ulBufSize);
		if (iRet < 0)
		{
			ERR_PRINTF(strerror(errno));
			return -1;
		}
		
		iRet = send(connfd, buf, len, 0);
		if (-1 == iRet)
		{
			ERR_PRINTF(strerror(errno));
			return -1;
		}
		
		lSendDataLen += len;
		if (lSendDataLen > ulFileSize)
		{
			return -1;
		}
		
		if(len != ulBufSize)
		{
			MSG_PRINTF("Read Big File End!");
			break;
		}	
	}
	
	close(fd);

	MSG_PRINTF("Success To Send Big File size = [%lu] Bytes", lSendDataLen);

	return lSendDataLen;
	
}

/*****************************************************************************
 Prototype    : WEB_http_HandleHttpReq
 Description  : 处理http请求
 Input        : IN INT    connfd          
                IN CHAR *pucBuf           
                IN ULONG  ulHttpHeadSize  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG WEB_http_HandleHttpReq(IN INT   connfd, 
 							 IN CHAR *pucBuf, 
 							 IN ULONG ulHttpHeadSize)
{
	UINT   iRet;
	ULONG  ulRet;
	INT    status		    = 200;
	BOOL_T bBigFile 		= BOOL_FALSE;
    LONG   lHeadLen         = 0;
	ULONG  uiReapBufLen	    = 0;
	UINT   uiReapFileBufLen = 0;
	WEB_HTTP_REQMSGINFO_S stHttpReqMsgInfo;
	CHAR   szRespBuf[WEB_HTTP_REQ_RESP_BUF_LEN_MAX] 		= {0};
	CHAR   szRespFileBuf[WEB_HTTP_REQ_RESPFILE_BUF_LEN_MAX] = {0};

	memset(&stHttpReqMsgInfo, 0, sizeof(WEB_HTTP_REQMSGINFO_S));
	/* 获取要请求文件的信息 */
	ulRet = WEB_http_DecHttpReqMsg(pucBuf, ulHttpHeadSize, &stHttpReqMsgInfo);
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("get http reg msg failed!");
		//BUG:需要回复404页面
	}

	/* ReqEevetID为0说明请求的文件 */
	if (0 == stHttpReqMsgInfo.stHttpReqLine.uiReqEevetID)
	{
		uiReapFileBufLen = WEB_HTTP_REQ_RESP_BUF_LEN_MAX;
		/* 读取请求的文件 */		
		ulRet = WEB_http_GetPurenessFileData(stHttpReqMsgInfo.stHttpReqLine.szFilePathBuf,
											 szRespFileBuf,
											 (&uiReapFileBufLen));
		if (ERROR_DATA_IS_TOO_LONG == ulRet)
		{
			MSG_PRINTF("Req File Reap Buf is too small! File size = [%d] Bytes", uiReapFileBufLen);
			//分开慢慢send;
			bBigFile = BOOL_TRUE;
 		}
 		else if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Get Pureness File Data Failed!");
			status = 404;
		}
	}
	/* 请求事件 */
	else
	{
		ulRet = WEB_http_HandleEvent(&stHttpReqMsgInfo, 
									 szRespFileBuf,
									 (&uiReapFileBufLen));
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Get Pureness File Data Failed!");
			status = 404;
		}

	}

	/* 设置报文头 */
	lHeadLen = WEB_http_SetHeadInfo( szRespBuf , 
									 WEB_HTTP_REQ_RESP_BUF_LEN_MAX,
			 					     status, 
			 					     uiReapFileBufLen, 
			 					     stHttpReqMsgInfo.stHttpReqLine.szReqFileType);
	if (lHeadLen < 0)
	{
		ERR_PRINTF("Web Http Set Head Info Failed!");
	}
	
	MSG_PRINTF("SetHeadInfo SUCCESS!");

	/* 拼装回应报文 */
	if (strlen(szRespFileBuf) > WEB_HTTP_REQ_RESP_BUF_LEN_MAX)
	{
		ERR_PRINTF("Some Thing is too long for us!");
		return ERROR_FAILE;
	}
 	strcat(szRespBuf, szRespFileBuf);

 	uiReapBufLen = strlen(szRespBuf);

	DBG_PRINT_LOG("pcRespBUF.dat", szRespBuf, uiReapBufLen);

	MSG_PRINTF("HTTP Fd = [%d]", connfd);
	/*
		如果连续两次点击链接按钮，，再点断开按钮，fd就他妈无效了
		这个问题应该是linkserver线程连接服务器，引起的，因为断开就没有问题
		*/
	
	/* 返回用户请求的数据 */
	iRet = send(connfd, szRespBuf, uiReapBufLen,0);
	if (-1 == iRet)
	{
		ERR_PRINTF("Send Http Resp Data Failed! Because:%s", strerror(errno));
		return ERROR_FAILE;
	}

	if (bBigFile == BOOL_TRUE)
	{
		iRet = web_http_SendBigFile(connfd,
 								    stHttpReqMsgInfo.stHttpReqLine.szFilePathBuf,
 								    szRespFileBuf,
 								    WEB_HTTP_REQ_RESPFILE_BUF_LEN_MAX,
 								    uiReapFileBufLen);
 		if (iRet < 0)
 		{
			ERR_PRINTF("Send Big File Failed!");
			return ERROR_FAILE;
 		}
	}

	return ERROR_SUCCESS;
}
