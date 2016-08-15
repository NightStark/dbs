#ifndef __WEB_ACTION_H__
#define __WEB_ACTION_H__

#define WEB_ACTION_ID_STRING_LEN_MAX (16)

typedef enum   tag_WebActionId
{
	WEB_ACTION_ID_NONE = 0,
	
	WEB_ACTION_ID_SYSINTO,
	
	WEB_ACTION_ID_MAX,
}WEB_ACTION_ID_E;

typedef enum tag_WebActionValue
{
	WEB_ACTION_VALUE_NONE = 0,
	
	WEB_ACTION_VALUE_SYSINTO,
	
	WEB_ACTION_VALUE_MAX,
}WEB_ACTION_VALUE_E;


typedef struct tag_WebAction
{
	INT iActionId;
	INT iActionValue;
}WEB_ACTION_REQLIST_S;


typedef enum tag_WebThrdRespCode
{
	WEB_THRD_RESP_CODE_NONE = 0,
	
	WEB_THRD_RESP_CODE_ACTION_SUCCESS,
	
	WEB_THRD_RESP_CODE_MAX,
}WEB_THRD_RESP_CODE_E;

/* WEB线程向action线程发送请求的消息 */
typedef struct tag_WebActionReq
{
	UINT  uiWebEventType;	/* Http Event 类型 */
	VOID *pReqBuf;			/* 需要Action线程填充的Buffer */
	UINT  uiReqBufLen;		/* pReqBuf的长度 */
	VOID *pHttpReqMsgInfo;	/* http请求信息 */
}WEB_ACTION_REQ_S;
/* WEB线程向action线程发送请求回复的消息 */
typedef struct tag_WebActionResp
{
	UINT  uiWebEventRespType;/* Http Event 的Action回复类型 */
	VOID *pRespBuf;			 /* 和WEB_ACTION_REQ_S.pReqBuf应该是一个地址，Action线程返回给http处理线程 */
	UINT  uiRespBufLen;		 /* pRespBuf在Action线程实际填充的长度 */
}WEB_ACTION_RESP_S;

ULONG WEB_action_Init(VOID);
ULONG WEB_action_Handle(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg);

#endif //__WEB_ACTION_H__
