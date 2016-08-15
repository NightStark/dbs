#ifndef __CLIENT_FSM_H_
#define __CLIENT_FSM_H_

ULONG Client_fsm_msg_Proc(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo);

ULONG Client_fsm_msg_proc_AskServiceResp(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo);

ULONG Client_fsm_msg_proc_ClientCreateTableResp(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo);

#endif //__CLIENT_FSM_H_
