#ifndef __CLIENT_H__
#define __CLIENT_H__

typedef enum tag_ClientThrdQueMsgCode
{
	CLIENT_THRDQUE_MSG_CODE_NONE = 0,
	CLIENT_THRDQUE_MSG_CODE_ASKSRV_RESP,
}CLIENT_THRDQUE_MSG_CODE_E;
 
typedef enum tag_ClientThrdQueMsgTranChannel
{
	CLIENT_THRDQUE_MSG_TRSCHL_NONE = 0x8888,
	CLIENT_THRDQUE_MSG_TRSCHL_ASKSRV_RESP,
}CLIENT_THRDQUE_MSG_TransChl_E;

//ULONG Client_Recv(IN INT iDataBufLen , IN VOID *pDataBuf);
ULONG CLIRNT_msg_send(INT iSockFd, VOID *pBuf, SIZE_T ssSendBuflen);
ULONG CLIENT_AskServer(IN INT iClientfd);
ULONG CLIENT_CreateClientTable(IN INT iClientfd);

ULONG MSG_Client_RECV_HandleMsg(IN INT iConnFd, IN UCHAR *pauRecvBuf, IN INT iMsgLen);

#endif //__CLIENT_H__
