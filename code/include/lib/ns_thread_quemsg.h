#ifndef __THREAD_QUEMSG_H__
#define __THREAD_QUEMSG_H__

#define QUEMSG_MSG_LEN_MAX (256)

#define QUEMSG_INFO_EMPTY  	   (0)
#define QUEMSG_INFO_FULL	   (1)

typedef ULONG (* EPOLL_WAIT_SIG_DO)(VOID *);
typedef ULONG (* EPOLL_WATI_WAKEUP_DO)(VOID *);


typedef VOID * (*QUEMSG_DATA_ALLOC)(VOID);
typedef VOID (*QUEMSG_DATA_DELETE)(VOID *);

//typedef ULONG (*QUEMSG_DATA_IN)(VOID *, VOID *); //(*data, &sturct )
//typedef ULONG (*QUEMSG_DATA_OUT)(VOID *, VOID *);//(&sturct ,*data)


typedef enum THREAD_QUEMSG_TYPE
{
	THREAD_QUEMSG_TYPE_ASK_SRV_RESP_CODE,                
}THREAD_QUEMSG_TYPE_ENUM;

typedef struct tag_quemsgdata
{
	UINT  uiQueMsgType;
	UINT  uiQueMsgDataLen;  /* 这个并不一定是字节个数，有可能是元素成员个数，等... */
	VOID *pQueMsgData;  /* 使用者申请，接收者释放,蛋疼 */
}THREAD_QUEMSG_DATA_S;

#define THREAD_QUEMSG_TYPE_P (0x80000000)
#define THREAD_QUEMSG_TYPE_B (0x60000000)

/* 消息以指针传送 */
#define THRD_QUEMSG_TYPE_SET_P(type) ((type) | THREAD_QUEMSG_TYPE_P)
/* 消息以非指针传送 */
#define THRD_QUEMSG_TYPE_SET_B(type) ((type) | THREAD_QUEMSG_TYPE_B)

//#define THRD_QUEMSG_TYPE_WEB_EVENT_ASK  	THRD_QUEMSG_TYPE_SET_B(1)
//#define THRD_QUEMSG_TYPE_WEB_EVENT_RESP  	THRD_QUEMSG_TYPE_SET_B(2)

typedef enum tag_ThreadQueMsgTypeWebEvent
{
	THRD_QUEMSG_TYPE_WEB_EVENT_NONE = 0,

	THRD_QUEMSG_TYPE_WEB_EVENT_ASK,					/* 1 */
	THRD_QUEMSG_TYPE_WEB_EVENT_RESP,

	THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_ASK,     	
	THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_SAVE,		
	THRD_QUEMSG_TYPE_WEB_EVENT_SETTING_RESP, 		
	
	THRD_QUEMSG_TYPE_WEB_EVENT_CONTROL_ASK_CONN_SERVER, 		
	THRD_QUEMSG_TYPE_WEB_EVENT_CONTROL_ASK_DISCONN_SERVER, 		
	THRD_QUEMSG_TYPE_WEB_EVENT_CONTROL_RESP,					

	THRD_QUEMSG_TYPE_KILL,
	THRD_QUEMSG_TYPE_WEB_EVENT_MAX,
}THRD_QUEMSG_TYPE_WEB_EVENT_E;

typedef enum TAG_THRADQUSMSGTYPE
{
	THRAD_QUSMSG_TYPE_NONE = THRD_QUEMSG_TYPE_WEB_EVENT_MAX,

	THRAD_QUSMSG_TYPE_DBUG_PRINT,

	THRAD_QUSMSG_TYPE_MAX,
}THRAD_QUSMSG_TYPE_E;

#define WEB_EVENT_TYPE_MASK (0x0000FFFF);

typedef enum tag_WebEventType
{
	/* WET <=> WEB EVENT TYPE */
/* 0 */		WET_SET_ASK = 0,
/* 1 */		WET_SET_SAVE, 
/* 2 */		WET_SET_RESP,

/* 3 */		WET_CTRL_CONN_SER,
/* 4 */		WET_CTRL_DISCONN_SER,

			WET_CTRL_MAX,
}WEB_EVENT_TYPE_E;

typedef enum tag_ThreadQueMsgClientLink
{
	THREAD_QUEMSG_CLIENT_LINK_TYPE_NONE = 0,

	TQ_C_LK_TYPE_ADD_DATA,
	
	THREAD_QUEMSG_CLIENT_LINK_TYPE_MAX,
	
}THREAD_QUEMSG_CLIENT_LINK_TYPE;
/* INT:Thread id, msg, */
typedef VOID (*THREAD_SERVER_QUEMSG_RECVCB)(IN INT ,IN const THREAD_QUEMSG_DATA_S *);

typedef enum THREADQUEMSGON
{
    THREAD_QUEMSG_ON,
    THREAD_QUEMSG_OFF,
}THREAD_QUEMSGON_E;

typedef struct tag_thread_QueMsg
{
	BOOL_T bThrdQueMsgIsOn;							/* 是否开启了消息列队功能 */
    INT iThrdQueMsgEventFd;
    THREAD_SERVER_QUEMSG_RECVCB pfThrdQueMsgRecvCb; /* 接收消息的处理回调 */

    UINT uiThreadQuemsgLen; 						/* 列队长度，还有消息个数 */
    DCL_HEAD_S stThreadQuemsgHead;                  /* 消息队列链表 */
    pthread_mutex_t ThreadQuemsg_mutex;				/* 线程消息列队锁 */
    INT iThrdWaitQueMsgSeqNum;						/* 当前在等待的消息序列号 */
    
	pthread_mutex_t ThreadQuemsgWait_mutex;  		/* 线程消息列队[等待]锁 */
	pthread_cond_t  ThreadQuemsWaticond;  			/* 线程消息列队[唤醒条件]锁 */

	INT iCurThrdQueMsgSeqNum;						/* 线程当前正在处理的消息序列号 */
    
}THREAD_QUEMSG_INFO_S;

ULONG THREAD_quemsg_Init(VOID);
VOID  THREAD_quemsg_Fint(VOID);
INT   Thread_quemsg_GenerateSeqNum(VOID);
ULONG THREAD_quemsg_Write(IN INT iQMSNum,
						  IN BOOL_T bNeedResp,
						  IN THREAD_QUEMSG_INFO_S *pstThrdQueMsg,
						  INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsgData);
ULONG THREAD_quemsg_Read(IN THREAD_QUEMSG_INFO_S *pstThrdQueMsg,						 
						 OUT INT *piSrcThrdId,
						 INOUT INT *piQMSNum, 
						 OUT BOOL_T *pbNeedResp,
						 INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsgData);						 
VOID  THREAD_quemsg_Free(IN INT iQueMsgSeqNum);
VOID  THREAD_quemsg_DestroyAll(IN DCL_HEAD_S *pstThreadQuemsgHead);

#endif //__THREAD_QUEMSG_H__
