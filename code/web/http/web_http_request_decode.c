#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ns_base.h>
#include <ns_web_server.h>
#include <ns_http.h>

/* 服务器网页文件放置根目录 */
STATIC CHAR g_szHttpWebRootDir[WEB_HTTP_REQ_ROOTDIR_BUF_LEN_MAX] = "./web/dir";
STATIC CHAR g_szHttpWebErr404Path[] = "/error/404.html";
STATIC CHAR g_szHttpWebIndexPath[]  = "/index.html";

WEB_HTTP_FILETYPE_S g_aucHttpReqFileTypeList[] = {
	{0,"html", "text/html"},
	{1,"php",  "text/html"},
	{2,"css",  "text/css"},
	{3,"js",   "application/x-javascript"},
	{4,"jpg",  "image/png"},
	{5,"jpeg", "image/png"},
	{6,"gif",  "image/gif"},
	{7,"ico",  "image/x-icon"},
	{8,"others", "text/html"}
};

WEB_HTTP_FILETYPE_S *Web_http_GetHttpFileType(CHAR *pucRegFileType)
{
	INT  iRet = 0;
	UINT uiIndex;
	UINT uiFileTypeCnt;
	
	DBGASSERT(NULL != pucRegFileType);

	uiFileTypeCnt = ARRAY_SIZE(g_aucHttpReqFileTypeList);

	for(uiIndex = 0; uiIndex < uiFileTypeCnt; uiIndex++)
	{
		iRet = str_safe_cmp((const CHAR *)pucRegFileType,
							 (const CHAR *)g_aucHttpReqFileTypeList[uiIndex].pucReqFileType,
							 strlen(pucRegFileType));
		if (0 == iRet)
		{
			break;
		}
	}

	if (uiIndex < uiFileTypeCnt)
	{
		return &(g_aucHttpReqFileTypeList[uiIndex]);
	}
	else
	{
		ERR_PRINTF("Do not support request file type! Failed");
		return NULL;
	}
	
}

/* 解析请求的文件后缀名 */
CHAR * Web_http_DecReqFileType(CHAR *pucReqFile)
{
	UINT    uiReqFileLen;

	uiReqFileLen = strlen(pucReqFile);
	while( '.' != (*(pucReqFile + uiReqFileLen)))
	{
		if (uiReqFileLen == 1) /* 本地路径为 ./xxx/xxxxx/sss.html */
		{
			break;
		}
		uiReqFileLen--;
	}

	if (1 == uiReqFileLen)
	{
		return NULL;
	}
	
	MSG_PRINTF("Req File Type [%s]" ,pucReqFile + uiReqFileLen + 1);
	return (pucReqFile + uiReqFileLen + 1);
}

STATIC CHAR const *g_WebHttpReqMethodInfoList[]={
	[HTTP_REQ_METHOD_GET] 	  = "GET",
	[HTTP_REQ_METHOD_POST]    = "POST",
	[HTTP_REQ_METHOD_HEAD]    = "HEAD",
	[HTTP_REQ_METHOD_PUT] 	  = "PUT",
	[HTTP_REQ_METHOD_DELETE]  = "DELETE",
	[HTTP_REQ_METHOD_TRACE]	  = "TRACE",
	[HTTP_REQ_METHOD_CONNECT] = "CONNECT",
	[HTTP_REQ_METHOD_OPTIONS] = "OPTIONS",
};

STATIC CHAR const *g_WebHttpReqVersionInfoList[]={
	[WEB_HTTP_REQVERSION_HTTP_1_1] 	  = "HTTP/1.1",
};

STATIC CHAR const *g_WebHttpReqHeaderInfoList[]={
	[WEB_HTTP_REQHEADER_ACCEPT] 	  	 = "Accept",
	[WEB_HTTP_REQHEADER_ACCEPT_LANGUAGE] = "Accept-Language",
	[WEB_HTTP_REQHEADER_USER_AGENT] 	 = "User-Agent",
	[WEB_HTTP_REQHEADER_ACCEPT_ENCODING] = "Accept-Encoding",
	[WEB_HTTP_REQHEADER_CONTENT_TYPE] 	 = "Content-Type",
	[WEB_HTTP_REQHEADER_HOST] 			 = "Host",
	[WEB_HTTP_REQHEADER_DNT] 			 = "DNT",
	[WEB_HTTP_REQHEADER_CONNECTION] 	 = "Connection",
	[WEB_HTTP_REQHEADER_CONTENT_LENGTH]  = "Content-Length",
	[WEB_HTTP_REQHEADER_CACHE_CONTROL]   = "Cache-Control",
	[WEB_HTTP_REQHEADER_CACHE_REFERER]	 = "Referer",
	[WEB_HTTP_REQHEADER_UA_CPU]	         = "UA-CPU",
	[WEB_HTTP_REQHEADER_X_REQUESTED_WITH]= "x-requested-with",
};

/* //暂时不支持他们
STATIC CHAR const *g_WebHttpReqAcceptInfoList[]={
	[WEB_HTTP_REQACCEPT_TEXT_HTML] 	  		   = "text/html",
	[WEB_HTTP_REQACCEPT_APPLICATION_XHTML_XML] = "application/xhtml+xml",
};

STATIC CHAR const *g_WebHttpReqUserAgentInfoList[]={
	[WEB_HTTP_REQUSERAGENT_MOZILLA] 	  	   = "Mozilla",
};

STATIC CHAR const *g_WebHttpReqAcceptEncodingInfoList[]={
	[WEB_HTTP_REQACCEPTENCODING_GZIP] 	  	   = "gzip",
	[WEB_HTTP_REQACCEPTENCODING_DEFLATE] 	   = "deflate",
};
*/

/* 可修改为宏函数 */
STATIC inline UINT web_http_DecInfoList(IN CHAR const *pucInfoList[], 
	 									IN CHAR *pucCkBuf,
	 									IN UINT uiINone,
	 									IN UINT uiIMax)
{
	INT  iRet = 0;
	UINT uiIndex = 0;
	for (uiIndex = uiINone + 1; uiIndex < uiIMax; uiIndex++)
	{
		iRet = str_safe_cmp(pucInfoList[uiIndex],
	   						pucCkBuf,
	   						strlen(pucInfoList[uiIndex]) + 1);
	   	if (0 == iRet)
	   	{
	   		MSG_PRINTF("xx = [%d][%s]", 
				   		uiIndex, 
				   		pucInfoList[uiIndex]);
			return uiIndex;
	   	}
	}
	
	return uiIndex;
}

/* 在'?'处截断 */
ULONG WEB_http_FilePathCutOffWithQ(IN CHAR *pucReqURI)
{
	UINT   uiI;
	BOOL_T bFindedQ = BOOL_FALSE; /* 出现第二问号就认为请求错误 */

	/* 查找问号的地方，并截断 */
	for (uiI = 0; *(pucReqURI + uiI) != '\0'; uiI++)
	{
		if ('?' == *(pucReqURI + uiI))
		{
			if (BOOL_TRUE == bFindedQ)
			{
				ERR_PRINTF("This have 2 \'?\' in request path!");
				return ERROR_FAILE;
			}
			*(pucReqURI + uiI) = 0; /* 在问好处截断 */
			bFindedQ = BOOL_TRUE;
		}
	}

	return ERROR_SUCCESS;
}

/* 解析本地请求文件路径 */
ULONG WEB_http_DecFilePath(IN CHAR *pucReqURI, 
 						   INOUT CHAR *pucReqFilePath,
 						   IN UINT uiReqFilePathBufLen)
{
	if (uiReqFilePathBufLen < WEB_HTTP_REQ_ROOTDIR_BUF_LEN_MAX)
	{
		MSG_PRINTF("Req File Path Buf is too small!");
		return ERROR_FAILE;
	}

	if (strlen(pucReqURI) + sizeof(g_szHttpWebRootDir) > uiReqFilePathBufLen)
	{
		MSG_PRINTF("Req File Path Buf is too small!");
		return ERROR_FAILE;
	}

	/* 拼装完整路径 */
	sprintf(pucReqFilePath, "%s%s", g_szHttpWebRootDir, pucReqURI);

	MSG_PRINTF(pucReqFilePath);

	return ERROR_SUCCESS;
}

/* 解析请求行，就是第一行 */
STATIC ULONG web_htte_DecReqLine(IN CHAR *pucBufLine,
								 IN WEB_HTTP_REQLINE_S *pstReqLine)
{
	UINT  uiRet;
	CHAR szMethod[16]  = {0};
	CHAR szURI[WEB_HTTP_REQ_URI_LEN_MAX] = {0};
	CHAR szVersion[16] = {0};
	CHAR *pucReqFType  = NULL; /* temp point */
	WEB_HTTP_FILETYPE_S *pstReqFileTypeInfo = NULL;

	if (3 != sscanf(pucBufLine, "%s %s %s", szMethod, szURI, szVersion))
	{
		ERR_PRINTF("Request line is invalid!");
		return ERROR_FAILE;
	}
	MSG_PRINTF("Method:%s,URI:%s,Version:%s", szMethod, szURI, szVersion);

	/* 解析请求类型 */
	uiRet = web_http_DecInfoList(g_WebHttpReqMethodInfoList,
								 szMethod,
								 HTTP_REQ_METHOD_NONE,
								 HTTP_REQ_METHOD_MAX);
	if (uiRet >= WEB_HTTP_REQHEADER_MAX)
	{
		ERR_PRINTF("DecReq Method failed!");
		return ERROR_FAILE;
	}
	pstReqLine->uiReqMethod = uiRet;

	/* 解析请求路径，包括各种地址栏输入的信息 */
	str_safe_cpy(pstReqLine->szReqURI,szURI,sizeof(pstReqLine->szReqURI));
	MSG_PRINTF("Request URI = %s", pstReqLine->szReqURI);

	if (ERROR_SUCCESS != WEB_http_FilePathCutOffWithQ(pstReqLine->szReqURI))
	{
		ERR_PRINTF("File Path Cut Off Witt \"?\" failed!");
		return ERROR_FAILE;
	}

	
	/* 获取请求的实际文件类型 */
	pucReqFType = Web_http_DecReqFileType(pstReqLine->szReqURI);
	if (NULL != pucReqFType)
	{	/* 请求的为一个文件而已 */
		MSG_PRINTF("Request is only a File [%s]", pstReqLine->szReqURI);

		/* 解析本地请求文件路径 */
		if (ERROR_SUCCESS != WEB_http_DecFilePath(pstReqLine->szReqURI,
 											      pstReqLine->szFilePathBuf,
 											      sizeof(pstReqLine->szFilePathBuf)))
		{
			ERR_PRINTF("DecReq File Path failed!");
			return ERROR_FAILE;
		}
		
		pstReqFileTypeInfo = Web_http_GetHttpFileType(pucReqFType);
		str_safe_cpy(pstReqLine->szReqFileType,
					 pstReqFileTypeInfo->pucFileType,
					 sizeof(pstReqLine->szReqFileType));
					 
		MSG_PRINTF("Req File type Info = %s", pstReqLine->szReqFileType);
	}
	else
	{
		CHAR *pucReqEvtId = pstReqLine->szReqURI + 1;
		if (1 != is_all_num(pucReqEvtId) || strlen(pucReqEvtId) > 30)
		{
			ERR_PRINTF("Req Eevet ID is invalid!");
			return ERROR_FAILE;
		}
		pstReqLine->uiReqEevetID = atoi(pucReqEvtId);
		MSG_PRINTF("ASK A EVENT [s:%s] [int:%d]", pucReqEvtId,pstReqLine->uiReqEevetID);
		str_safe_cpy(pstReqLine->szReqFileType,
					 "text/html",
					 sizeof(pstReqLine->szReqFileType));
	}

	uiRet = web_http_DecInfoList(g_WebHttpReqVersionInfoList,
								 szVersion,
								 WEB_HTTP_REQVERSION_NONE,
								 WEB_HTTP_REQVERSION_MAX);
	if (WEB_HTTP_REQVERSION_NONE == uiRet)
	{
		ERR_PRINTF("DecReq Version failed!");
		return ERROR_FAILE;
	}	
	pstReqLine->uiHTTPVersion = uiRet;

	return ERROR_SUCCESS;
}

STATIC ULONG web_http_DecMsgHead(IN CHAR *pucBufLine,
								 IN WEB_HTTP_REQMSGHEAD_S *pstReqMsgHead)
{
	UINT uiRet;
	CHAR *pucBuf;
	CHAR *pucValueBuf;
	/*Xxxx: xxx*/
	/* 	函数返回一个指针，
		它指向字符串str2中任意字符在字符串str1 
		首次出现的位置，如果不存在返回NULL */
	pucBuf = strpbrk(pucBufLine, ": ");
	if (NULL == pucBuf)
	{
		ERR_PRINTF("Invalid Line!");
		return ERROR_FAILE;
	}

	*pucBuf = 0x0;
	pucValueBuf = pucBuf+2;

	MSG_PRINTF("Header after brk [%s]", pucBufLine);

	//uiRet = web_http_DecMsgHeaderType(pucBufLine);
	uiRet = web_http_DecInfoList(g_WebHttpReqHeaderInfoList,
								 pucBufLine,
								 WEB_HTTP_REQHEADER_NONE,
								 WEB_HTTP_REQHEADER_MAX);

	if (uiRet >= WEB_HTTP_REQHEADER_MAX)
	{
		ERR_PRINTF("Dec Msg Header Type failed, not do anything for [%s]!", pucBufLine);
		return ERROR_SUCCESS;
	}

	switch (uiRet)
	{
		case WEB_HTTP_REQHEADER_ACCEPT:
		{
			/*
			uiRet = web_http_DecInfoList(g_WebHttpReqAcceptInfoList,
								 pucBufLine,
								 WEB_HTTP_REQACCEPT_NONE,
								 WEB_HTTP_REQACCEPT_MAX);
			if (WEB_HTTP_REQACCEPT_NONE == uiRet)
			{
				ERR_PRINTF("DecReq Accept failed!");
				return ERROR_FAILE;
			}	
			*/
			pstReqMsgHead->uiAccept = 0;
			break;
		}
		case WEB_HTTP_REQHEADER_ACCEPT_LANGUAGE:
		{
			pstReqMsgHead->uiAcceptLanguage = 0;
			break;
		}
		case WEB_HTTP_REQHEADER_USER_AGENT:
		{
			pstReqMsgHead->uiUserAgent = 0;;
			break;
		}
		case WEB_HTTP_REQHEADER_ACCEPT_ENCODING:
		{
			pstReqMsgHead->uiAcceptEncodingl = 0;
			break;
		}
		case WEB_HTTP_REQHEADER_HOST:
		{
			str_safe_cpy(pstReqMsgHead->pucHost,
 						 pucValueBuf,
 						 strlen(pstReqMsgHead->pucHost));
			break;
		}
		case WEB_HTTP_REQHEADER_DNT:
		{
			pstReqMsgHead->ucDNT = 0;
			break;
		}
		case WEB_HTTP_REQHEADER_CONNECTION:
		{
			pstReqMsgHead->uiConnection = 0;
			break;
		}
		default:
		{
			WARN_PRINTF("I have not support this [%s]", 
			     g_WebHttpReqHeaderInfoList[uiRet]);
		}
	}

	return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : web_http_DecLine
 Description  : 逐行解析请求报文
 Input        : IN UCHAR *pucBufLine                         
                IN UINT uiLineNum                            
                IN WEB_HTTP_REQMSGINFO_S *pstHttpReqMsgInfo  
 Output       : None
 Return Value : STATIC
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG web_http_DecLine(IN CHAR *pucBufLine, 
   							  IN UINT   uiLineNum,
   							  IN WEB_HTTP_REQMSGINFO_S *pstHttpReqMsgInfo)
{
	ULONG ulRet;
	if (0 == uiLineNum)
	{
		ulRet = web_htte_DecReqLine(pucBufLine,&(pstHttpReqMsgInfo->stHttpReqLine));
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Dec Req Line FAILED!");
			return ERROR_FAILE;
		}
	}
	else
	{
		ulRet = web_http_DecMsgHead(pucBufLine,&(pstHttpReqMsgInfo->stHttpReqMsgHead));
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("Dec Msg Head FAILED!");
			return ERROR_FAILE;
		}	
	}

	return ERROR_SUCCESS;
}


/* 
	把数据行解析的数组中 

	ID=123&ID2=456&ID3=789&submit=-2.194851E-4760.000000%E4%BA-0X1.007FP-1554
			| |
			\ /
			 V
		[ID, 123]
		[ID2,456]
		[ID3,789]
			.
			.
			.
	
*/
/*****************************************************************************
 Prototype    : WEB_http_ProcSubmitMsg
 Description  : 把数据行解析的数组中
 Input        : IN UINT SMsgBufLen      		提交数据长度               
                IN const UCHAR *pucSMsgBuf      提交的数据缓冲区地址
                IN WEB_HTTP_REQSUBMITDATA_S *ppstSMsg  解析出来的数组
 Output       : None
 Return Value : STATIC
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
STATIC ULONG WEB_http_ProcSubmitMsg(IN UINT SMsgBufLen,
							 	    IN const CHAR *pucSMsgBuf, 
						            OUT WEB_HTTP_REQSUBMITDATA_S *ppstSMsg)
{
	//ULONG ulP = 0;
	//ULONG ulNP = 0;
	//ULONG ulVP = 0;
	ULONG ulNumOfSMsg = 0;

	/*
	
	for(ulP = 0; (0 != *(pucSMsgBuf + ulP)) && (ulP < SMsgBufLen) ;)
	{
		for(ulNP = 0; 
			'=' != *(pucSMsgBuf + ulP) && 0 != *(pucSMsgBuf + ulP) && (ulP < SMsgBufLen); 
			ulNP++, ulP++)
		{
			if (ulNP > WEB_HTTP_REQ_SUBN_BUF_LEN_MAX)
			{
				ERR_PRINTF("sub mit name is too long");
				;
			}
			ppstSMsg[ulNumOfSMsg].pcSMName[ulNP] = *(pucSMsgBuf + ulP);
		}
		ulP++;
		
		for(ulVP = 0; 
			'&' != *(pucSMsgBuf + ulP) && 0 != *(pucSMsgBuf + ulP) && (ulP < SMsgBufLen); 
			ulVP++, ulP++)
		{
			if (ulVP > WEB_HTTP_REQ_SUBV_BUF_LEN_MAX)
			{
				ERR_PRINTF("sub mit value is too long");
				return ERROR_FAILE;
			}
			ppstSMsg[ulNumOfSMsg].pcSMValue[ulVP] = *(pucSMsgBuf + ulP);
		}
		ulP++;

		if(ulNumOfSMsg++ > WEB_HTTP_REQ_SUBMSG_CNT_MAX)
		{
			ERR_PRINTF("Submit Msg is too much!");
			ulNumOfSMsg = 0;
		}
	}
	*/
	
	return ulNumOfSMsg;
	
}

/* 获得头信息的最后一行( POST来的数据行，暂时是的 ) */
CHAR * WEB_http_getSMsgATEOFHH(IN const CHAR * pcBuf)
{
	const CHAR *pcNext = NULL;
	ULONG ulPc = 0;

	pcNext = pcBuf + 1;

	for(ulPc = 0; 0 != *(pcBuf + ulPc); ulPc++)
	{
		if(('\r' == *(pcBuf + ulPc)) && ('\n' == *(pcBuf + ulPc + 1)))
		{
			pcNext= pcBuf + ulPc + 2;
		}
	}

	return (CHAR *)pcNext;
}

/*****************************************************************************
 Prototype    : WEB_http_DecHttpReqMsg
 Description  : 解析http请求报文
 Input        : IN UCHAR *pucBuf                             
                IN ULONG ulBufLen                            
                IN WEB_HTTP_REQMSGINFO_S *pstHttpReqMsgInfo  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/8/31
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
ULONG WEB_http_DecHttpReqMsg(IN CHAR *pucBuf, 
                             IN ULONG ulBufLen, 
                             IN WEB_HTTP_REQMSGINFO_S *pstHttpReqMsgInfo)
{
	ULONG  ulRet 		 = ERROR_SUCCESS;
	UINT   uiBufP    	 = 0;
	UINT   uiLineNum 	 = 0;
	ULONG  ulCutOffP 	 = 0;
	CHAR  *pucBufPre 	 = NULL;
	ULONG  ulNumOfSubMsg = 0;
	pucBufPre = pucBuf;

	/* 逐行解析请求报文 */
	for(uiBufP = 0; uiBufP < ulBufLen; uiBufP++, pucBuf++, ulCutOffP++)
	{
		if (*(USHORT*)(pucBuf) == 0x0A0D)
		{
			if (*(UINT*)(pucBuf) == 0x0A0D0A0D)
			{
				MSG_PRINTF("Req head is end!");
				break;
			}
			*(USHORT*)pucBuf = 0x0;
			MSG_PRINTF(pucBufPre);
			ulRet = web_http_DecLine(pucBufPre,uiLineNum,pstHttpReqMsgInfo);
			if (ERROR_SUCCESS != ulRet)
			{
				ERR_PRINTF("WEB Http Decode Request Line Failed!");
				return ulRet;
			}
			
			pucBuf    += 2;
			ulCutOffP += 2;
			pucBufPre = pucBuf;
			uiLineNum++;
		}
	}

	pucBuf    += 4;
	ulCutOffP += 4; /* 跳过 0x0A0D0A0D  */
	
	/* 解析报文头中携带的提交信息 */
	switch (pstHttpReqMsgInfo->stHttpReqLine.uiReqMethod)
	{
		case HTTP_REQ_METHOD_GET:
		{
			//WEB_http_ProcSubmitMsg(IN const UCHAR * pucSMsgBuf,IN WEB_HTTP_REQSUBMITDATA_S * ppstSMsg)
			break;
		}
		case HTTP_REQ_METHOD_POST:
		{
			ulNumOfSubMsg = WEB_http_ProcSubmitMsg(ulBufLen - ulCutOffP,
												   pucBuf,
												   pstHttpReqMsgInfo->pstHttpReqSubmitData);
			pstHttpReqMsgInfo->ucSMCnt = ulNumOfSubMsg;
			UINT uiII = 0;
			for(uiII = 0; uiII < ulNumOfSubMsg; uiII++)
			{
				MSG_PRINTF("[%s] = [%s]", 
							pstHttpReqMsgInfo->pstHttpReqSubmitData[uiII].pcSMName,
							pstHttpReqMsgInfo->pstHttpReqSubmitData[uiII].pcSMValue);
			}
			break;
		}
		default :
		{
			ERR_PRINTF("Fuck how i come to here!");
		}
	}

	return ERROR_SUCCESS;
}        

/* 
	@pucFilePath : 要读取的文件的路径
	@pcRespBUF   : 存放读取内容的内存
	@puiReapBufLen : 入参为pcRespBUF可用长度，出参实际文件的长度
*/
ULONG WEB_http_GetPurenessFileData(IN CHAR    *pucFilePath, 
								   IN CHAR    *pcRespBUF,
								   INOUT UINT *puiReapBufLen)
{
	ULONG ulRet					= 0;
	ULONG ulReqFileSize 		= 0;
	UINT uiReTryCheckCnt        = 0;
	
	struct stat stReqFileStat;

	DBGASSERT(NULL != pucFilePath);
	DBGASSERT(NULL != pcRespBUF);
	DBGASSERT(NULL != puiReapBufLen);
	DBGASSERT(0    != *puiReapBufLen);

	mem_set0(&stReqFileStat, sizeof(struct stat));

	/* 获取文件大小 */
	while(1)
	{
		if (uiReTryCheckCnt++ > 4)
		{
			return ERROR_FAILE;
		}

		if (pucFilePath[0] == '\0') {
			MSG_PRINTF("[%s] is empty ! redirect to [%s%s]", pucFilePath, g_szHttpWebRootDir, g_szHttpWebIndexPath);
			sprintf(pucFilePath, "%s%s", g_szHttpWebRootDir, g_szHttpWebIndexPath);
		}

		ulRet = stat(pucFilePath, &stReqFileStat);

		/* 指定文件不存在或为目录，重定向到 error/404.html */
		/* BUG:但是结构体[pstHHMsg]里面的其他成员没有相应的改变 */
		if(-1 == ulRet || S_ISDIR(stReqFileStat.st_mode))
		{
			ERR_PRINTF("get [%s] stat failed! redirect to [%s%s]", pucFilePath, g_szHttpWebRootDir, g_szHttpWebErr404Path);
			sprintf(pucFilePath, "%s%s", g_szHttpWebRootDir, g_szHttpWebErr404Path);
		}
		else
		{
			break;
		}
	}
	
	ulReqFileSize = stReqFileStat.st_size;
	MSG_PRINTF("READ FILE [path : %s] [size %lu]", pucFilePath, ulReqFileSize);
	
	if (ulReqFileSize > *puiReapBufLen)
	{
		/* 将实际的文件的大小返回出去 */
		*puiReapBufLen = ulReqFileSize;
		return ERROR_DATA_IS_TOO_LONG;
	}

	/* 读取文件内容 */
	ulRet = WEB_http_ReadReqFile(pucFilePath,pcRespBUF,&ulReqFileSize);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Read req file failed!");
		return ERROR_FAILE;
	}
	
	/* 将实际读取的文件的大小返回出去 */
	*puiReapBufLen = ulReqFileSize;
				
	return ERROR_SUCCESS;
}


