#ifndef __MSG_QUE_H__
#define __MSG_QUE_H__

#define MSG_QUE_KEY 		(1234)
#define MSG_QUE_MSG_LEN_MAX (4096)

#define MSG_QUE_PRINT_BUF_LEN (MSG_QUE_MSG_LEN_MAX - sizeof(DBG_PRINT_BUFFER_S))

/* 进程间，Debug数据 结构 */
typedef struct tag_MSGQUEDATA
{
 	LONG lMsgQueType;
 	//UINT uiMsgQueLength;
 	DBG_PRINT_BUFFER_S stDbgPrintBuf;		/* 记录buffer的位置属性 */
 	CHAR ucMSgQueBuf[MSG_QUE_PRINT_BUF_LEN];	/* 真正的buffer */
}MSG_QUE_PRINT_S;

INT MsgQue_GetMsgFd(VOID);

ULONG DBG_print_OnRemoteTerminal(IN const DBG_PRINT_BUFFER_S *pstDbgPrintBuf);
VOID  DBG_process_FriendDeTransForm(IN MSG_QUE_PRINT_S *pstMsgQueData);

ULONG MsgQue_Send(MSG_QUE_PRINT_S *pstMsgQueData);
ULONG MsgQue_Recv(VOID);

ULONG MsgQue_Init(VOID);
ULONG MsgQue_Fini(VOID);

#endif //__MSG_QUE_H__
