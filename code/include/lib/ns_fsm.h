#ifndef __FSM_H__
#define __FSM_H__

/* 状态机元素 */
typedef enum tag_FSMEle
{
	FSM_ELE_NONE = 0,
	FSM_ELE_NSDB_ASK_SERVICE_REQ,

	FSM_ELE_MAX,
}FSM_ELE_S;

ULONG FMS_msg_Proc(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo);
ULONG FSM_Init(VOID);

#endif //__FSM_H__
