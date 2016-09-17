#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__


#define MAXLINE 				(2048)
#define WEB_SERV_PORT 			(8080)

#define WEB_SER_LISTEN_MAX		(64)

#define RESP_BUF_MAX 		((1024) * (1024)) //1MB
#define RESP_HEAD_BUF_MAX 	(512)
#define RESP_FAILED_BUF_MAX	(1024)

#define HTTP_REQ_TYPE_GET	(0)
#define HTTP_REQ_TYPE_POST	(1)

#define HTTP_REQ_FILENAMELEN_MAX (64)
#define HTTP_REQ_FILELEN_MAX (256)


typedef struct tag_httpSubmitmsg_st
{
	UCHAR pcSMName[32];
	UCHAR pcSMValue[256];
}SubMsg_S;

/* 记录用户请求的文件的信息 */
typedef struct tag_httpHeadmsg_st
{
	CHAR cHttpType[5];
	CHAR cHttpTypeSet;	// 0 : GET     1 : POST 
	CHAR cHttpReqURL[256];
	CHAR cHttpReqFile[HTTP_REQ_FILELEN_MAX];
	CHAR cHttpReqFileName[HTTP_REQ_FILENAMELEN_MAX];
	CHAR cHttpReqFileType[32];
	CHAR cHttpReqFileTrueType[32];
	SubMsg_S stSMsg[32];
	ULONG ulNumOfSMsg;
}HttpHeadMsg_S;


typedef struct tag_WebTttpFileType
{
	UINT   uiFTindex;
	CHAR *pucReqFileType;
	CHAR *pucFileType;
}WEB_HTTP_FILETYPE_S;


/*
GET /socket.html?id=1&name=99&age=88&name=8899 HTTP/1.1
Accept: text/html, application/xhtml+xml, 
Accept-Language: zh-CN
User-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64; Trident/7.0; rv:11.0) like Gecko
Accept-Encoding: gzip, deflate
Host: 192.168.41.186:8888
DNT: 1
Connection: Keep-Alive
*/
/* 解析http请求 */
#define WEB_HTTP_REQ_URI_LEN_MAX			(256)
#define WEB_HTTP_REQ_MSGHEAD_HOST_LEN_MAX	(128)
#define WEB_HTTP_REQ_ROOTDIR_BUF_LEN_MAX	(64)
#define WEB_HTTP_REQ_FILEPATH_BUF_LEN_MAX	(256)
#define WEB_HTTP_REQ_FILETYPE_BUF_LEN_MAX	(64)
#define WEB_HTTP_REQ_RESPFILE_BUF_LEN_MAX	(1024 * 4)
#define WEB_HTTP_REQ_RESP_BUF_LEN_MAX	    (WEB_HTTP_REQ_RESPFILE_BUF_LEN_MAX + 1024)
#define WEB_HTTP_REQ_SUBN_BUF_LEN_MAX	    (64)
#define WEB_HTTP_REQ_SUBV_BUF_LEN_MAX	    (256)


/*
	请求方法（所有方法全为大写）有多种，各个方法的解释如下：
	GET     请求获取Request-URI所标识的资源
	POST    在Request-URI所标识的资源后附加新的数据
	HEAD    请求获取由Request-URI所标识的资源的响应消息报头
	PUT     请求服务器存储一个资源，并用Request-URI作为其标识
	DELETE  请求服务器删除Request-URI所标识的资源
	TRACE   请求服务器回送收到的请求信息，主要用于测试或诊断
	CONNECT 保留将来使用
	OPTIONS 请求查询服务器的性能，或者查询与资源相关的选项和需求
*/
typedef enum tag_HttpReqMethod
{
	HTTP_REQ_METHOD_NONE = 0,
	
	HTTP_REQ_METHOD_GET,
	HTTP_REQ_METHOD_POST,
	HTTP_REQ_METHOD_HEAD,
	HTTP_REQ_METHOD_PUT,
	HTTP_REQ_METHOD_DELETE,
	HTTP_REQ_METHOD_TRACE,
	HTTP_REQ_METHOD_CONNECT,
	HTTP_REQ_METHOD_OPTIONS,
	
	HTTP_REQ_METHOD_MAX,
}WEB_HTTP_REQMETHOD_E;

typedef enum tag_HttpReqVersion
{
	WEB_HTTP_REQVERSION_NONE = 0,
	
	WEB_HTTP_REQVERSION_HTTP_1_1,
	
	WEB_HTTP_REQVERSION_MAX,
}WEB_HTTP_REQVERSION_E;

typedef enum tag_HttpReqHeader
{
	WEB_HTTP_REQHEADER_NONE = 0,

	WEB_HTTP_REQHEADER_ACCEPT,
	WEB_HTTP_REQHEADER_ACCEPT_LANGUAGE,
	WEB_HTTP_REQHEADER_USER_AGENT,
	WEB_HTTP_REQHEADER_ACCEPT_ENCODING,
	WEB_HTTP_REQHEADER_CONTENT_TYPE,
	WEB_HTTP_REQHEADER_CONTENT_LEN,
	WEB_HTTP_REQHEADER_HOST,
	WEB_HTTP_REQHEADER_DNT,
	WEB_HTTP_REQHEADER_CONNECTION,
	WEB_HTTP_REQHEADER_CONTENT_LENGTH,
	WEB_HTTP_REQHEADER_CACHE_CONTROL,
	WEB_HTTP_REQHEADER_CACHE_REFERER,
	WEB_HTTP_REQHEADER_UA_CPU,
	WEB_HTTP_REQHEADER_X_REQUESTED_WITH,
	
	WEB_HTTP_REQHEADER_MAX,
}WEB_HTTP_REQHEADER_E;

/*客户端接受哪些类型的信息 */
typedef enum tag_HttpReqAccept
{
	WEB_HTTP_REQACCEPT_NONE = 0,
	
	WEB_HTTP_REQACCEPT_TEXT_HTML, 
	WEB_HTTP_REQACCEPT_APPLICATION_XHTML_XML,
	
	WEB_HTTP_REQACCEPT_MAX,
}WEB_HTTP_REQACCEPT_E;

/* User Agent 还包括浏览器版本号，系统类型等，现在只标示浏览器类型 */
typedef enum tag_HttpReqUserAgent
{
	WEB_HTTP_REQUSERAGENT_NONE = 0,
	 
	WEB_HTTP_REQUSERAGENT_MOZILLA,
	
	WEB_HTTP_REQUSERAGENT_MAX,
}WEB_HTTP_REQUSERAGENT_E;

/* 客户可接受的内容编码 */
typedef enum tag_HttpReqAcceptEncoding
{
	WEB_HTTP_REQACCEPTENCODING_NONE = 0,
	
	WEB_HTTP_REQACCEPTENCODING_GZIP, 
	WEB_HTTP_REQACCEPTENCODING_DEFLATE,
	
	WEB_HTTP_REQACCEPTENCODING_MAX,
}WEB_HTTP_REQACCEPTENCODING_E;

/*
	DNT现在接受三个赋值：
		1代表用户不想被第三方网站追踪，
		0代表接受追踪，
		null代表用户不置可否。
*/
typedef enum tag_HttpReqDNT
{
	WEB_HTTP_REQDNT_ACCEPT = 0,
	WEB_HTTP_REQDNT_REFUSE, 
	WEB_HTTP_REQDNT_NOCARE,
}WEB_HTTP_REQDNT_E;


/* 请求行 */
typedef struct tag_HttpReqLine
{
	UINT  uiReqMethod;								/* 方法 */
	CHAR szReqURI[WEB_HTTP_REQ_URI_LEN_MAX + 1];   
				/* 请求的URI [http://192.168.41.186:8888/abc/def.html] */
	CHAR szFilePathBuf[WEB_HTTP_REQ_FILEPATH_BUF_LEN_MAX];
				/* 请求文件的本地路径 [./rootdir/abc/def.html] */
	CHAR szReqFileType[WEB_HTTP_REQ_FILETYPE_BUF_LEN_MAX];
				/* 请求文件的系统类型  [text/html] */
	UINT  uiReqEevetID;  /* [http://192.168.41.186:8888/abc] ==> [abc]  */
						 /* 如果此ID有效(>0), 则上面的 路径和type无效，
						    如果此ID无效(=0)，则上面两项有效 */
	UINT  uiHTTPVersion;							/* 协议的版本 */
}WEB_HTTP_REQLINE_S;

/* 请求报头 */
typedef struct tag_HttpReqMsgHead
{
	UINT  uiAccept; /* ui的可以都放到一个数组里面啊 */ 
	UINT  uiAcceptLanguage;
	UINT  uiUserAgent;
	UINT  uiAcceptEncodingl;
	CHAR pucHost[WEB_HTTP_REQ_MSGHEAD_HOST_LEN_MAX];
	CHAR ucDNT; /* Do Not Track */
	UINT  uiConnection; /* 暂时未处理 */
	UINT  uiContentLen;
}WEB_HTTP_REQMSGHEAD_S;

#define WEB_HTTP_REQ_SUBMSG_CNT_MAX		(32)

/* 提交的数据 */
typedef struct tag_HttpReqSubmitData
{
	UCHAR pcSMName[WEB_HTTP_REQ_SUBN_BUF_LEN_MAX];
	UCHAR pcSMValue[WEB_HTTP_REQ_SUBV_BUF_LEN_MAX];
}WEB_HTTP_REQSUBMITDATA_S;

/* http请求 */
typedef struct tag_HttpReqMsgInfo
{
	WEB_HTTP_REQLINE_S 		 stHttpReqLine;
	WEB_HTTP_REQMSGHEAD_S 	 stHttpReqMsgHead;

	/* POST方式携带的数据 */
	UCHAR ucSMCnt;	/* 数据个数 */
	WEB_HTTP_REQSUBMITDATA_S pstHttpReqSubmitData[WEB_HTTP_REQ_SUBMSG_CNT_MAX];
}WEB_HTTP_REQMSGINFO_S;

#endif //__WEB_SERVER_H__

