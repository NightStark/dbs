/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : msg.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/7
  Last Modified :
  Description   : 有关消息的定义
  Function List :
  History       :
  1.Date        : 2013/11/7
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#ifndef __MSG_H__
#define __MSG_H__

#include <ns_msg_server_link.h>
#include <ns_msg_client_link.h>

#define NSDB_MSG_LISTEN_PORT 		9669
#define NSDB_MSG_LISTEN_ADDR 		"0.0.0.0" /* ==>ALL */
//#define NSDB_MSG_LISTEN_ADDR 		"192.168.93.131" /* ==>ALL */
#define NSDB_MSG_SERVER_ADDR 		"127.0.0.1"
#define NSDB_MSG_CLIENT_ADDR        "192.168.62.1"
#define NSDB_MSG_LISTEN_WAIT_MAX 	16
#define NSDB_MSG_LEN_MAX            (1024 * 8)


#define NSDB_HOST_NAME_LEN_MAX      (32)
	
#define MSG_SOCKET_INVALID_FD 		(-1)
#define MSG_CONNECT_INVALID_FD 		(-1)
#define MSG_EPOLL_INVALID_FD        (-1)

typedef enum tag_MessageType
{
	MSG_TYPE_NSDB_NONE = 0,

	MSG_TYPE_NSDB_ASK_SERVICE_REQ,
	MSG_TYPE_NSDB_ASK_SERVICE_RESP,

	MSG_TYPE_NSDB_SELECT_REQ,
	MSG_TYPE_NSDB_SELECT_RESP,

	MSG_TYPE_NSDB_SELECT_CREATE_TABLE_REQ,
	MSG_TYPE_NSDB_SELECT_CREATE_TABLE_RESP,

	MSG_TYPE_NSDB_TOP,
}MSG_TYPE_E;

typedef enum tag_MessageDataType
{
	MSG_DATA_TYPE_NONE = 0,
		
	MSG_DATA_TYPE_CLIENT_INFO,
	MSG_DATA_TYPE_ASK_SERVER_RESP_CODE,

	MSG_DATA_TYPE_NSDB_TABLE_INFO,
	MSG_DATA_TYPE_NSDB_TABLE_CREATE_RESP_CODE,

	MSG_DATA_TYPE_NSDB_TOP,
}MSG_DATA_TYPE_E;

typedef struct tag_MsgClientInfo
{
	CHAR szClientHostName[NSDB_HOST_NAME_LEN_MAX];
	NET_IF_DATA_S stCNetIFData;
}MSG_CLIENT_INFO_S;

typedef struct tag_AskServerRespCode
{	
	UINT uiRespCode;
}MSG_SERVER_RESP_CODE_S;

typedef struct tag_Msg_Send_Info
{
	UCHAR 	ucMsgType;
	UINT 	uiDataType;
	VOID   *pSendData;
	UINT    uiSendDataLen;
	VOID   *pDataBuf;
}MSG_MSG_SEND_INFO_S;

typedef struct tag_Msg_Recv_Info
{
	UCHAR 	ucMsgType;
	UINT    uiMsgLen;
	UINT 	uiDataType;

	
	UINT    uiRecvDataLen;
	//VOID   *pRecvData;
	
	VOID   *pRecvDataBuf;	
	UINT    uiLinkFD;
}MSG_MSG_RECV_INFO_S;

typedef struct tag_ClientNsdbCreateTable
{
	CHAR szTableName[TABLE_NAME_LEN_MAX];
	UINT uiTableEleCnt;
	TABLE_ELE_S *pstTableEle;
}CLIENT_NSDB_CREATE_TABLE_INFO_S;

typedef struct TAG_ClientNsdbCreateTableResp
{
	UINT uiRespCode;
}CLIENT_NSDB_CREATE_TABLE_RESP;

typedef enum tag_ServerReapCode
{
	SERVER_ASK_NONE = 0,
	SERVER_ASK_IS_ACCEPT,
	SERVER_ASK_IS_REFUSE,

	SERVER_CLIENT_NSdB_CREATE_TABLE_SUCCESS,
	SERVER_CLIENT_NSdB_CREATE_TABLE_FAILED,
}SERVER__RESP_CODE;


typedef struct tag_RecvPacket
{
    UINT uiInterActionID;       /* 本次完整交互的ID */
    UINT uiInterActionSeqNum;   /* 本次数据接收的秩序号 */
    UINT uiDataTotalLen;        /* 本次完整交互的数据总长度 */
    UINT uiDataLen;             /* 本次数据接收的有效数据长度 */
    UINT uiDataRemainLen;       /* 还需接收的有效数据长度 */
    UCHAR *pucDataStream;       /* 本次的数据流 */
}RECV_PACKET_S;

UINT MSG_de_DecData(VOID *pDataBuf,
	 					  UINT *uiDataType,
	 					  VOID *pDataStruct);

UINT MSG_de_DecMsgHead(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo, VOID *pRecvDataBuf);
ULONG Server_ProcServerAskReq(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo,
									 MSG_CLIENT_INFO_S *pstMsgClientInfo);
ULONG Server_ProcClientCreateTable(MSG_MSG_RECV_INFO_S *pstMsgRecvInfo,
									  CLIENT_NSDB_CREATE_TABLE_INFO_S *pstTableInfo);

UINT MSG_ec_EncMsg(UCHAR ucMsgType, 
				  UINT uiDataType,
				  VOID *pSendData,
				  UINT	uiSendDataLen,
				  VOID *pDataBuf);

UINT MSG_datadec_Uint(IN CHAR *pcDataBuf, OUT UINT *puiData);

UINT MSG_datadec_DataStruct(IN  UINT uiDataType, 
 					        IN  VOID *pDataBuf ,
 					        OUT VOID *pDataStruct);

UINT MSG_datadec_Char(IN CHAR *pcDataBuf, OUT CHAR *pcData);

UINT MSG_datadec_UChar(IN CHAR *pcDataBuf, OUT UCHAR *pucData);

UINT  MSG_datadenc_DataStruct(UINT uiDataType, 
							  VOID *pDataStruct, 
							  VOID *pDataBuf);
							  
UINT MSG_dataenc_Uint(IN UINT uiData, OUT CHAR *pcDataBuf);

ULONG MSG_RECV_RecvData(IN INT iConnFd, IN INT events, IN VOID *arg);

ULONG MSG_server_AskServerResp(IN INT iClientfd, IN UINT uiRespCode);


/***********************************
 * for msg decode & encode.
***********************************/

typedef enum tag_MSGDataType
{
    NS_MSG_DATA_TYPE_NONE = 0, 

    /* system msg type define  */
    NS_MSG_DATA_TYPE_SYS_NONE,
    NS_MSG_DATA_TYPE_SYS_HELLO,
    NS_MSG_DATA_TYPE_SYS_MAX,

    NS_MSG_DATA_TYPE_MAX,
}NS_MSG_DATA_TYPE_EN; 

/*  pf_encode_TLV need returen the length of pDataFlow */
typedef UINT (* pf_encode_TLV) (IN VOID *pStruct,   OUT VOID *pDataFlow, IN UINT uiDataFlowLen);
typedef UINT (* pf_decode_TLV) (IN VOID *pDataFlow, IN UINT DataFlwoLen, OUT VOID *pStruct);

/* Data desc : DD : 记录一个消息的一些信息 */
typedef struct tag_MsgDataDesc
{
    UINT  uiDataType;	/* 消息类型 */
    UINT  uiDataLen;    /* 消息长度 */
    VOID *pDataStruct;  /* 消息在发送encode前，接收后decode可以直接饮用的结构体/buffer */
    VOID *pDataFlow;    /* 消息被encode之后，buffer stream */
    UINT uiDataFlowTotalLen; /*   */
    pf_encode_TLV pfEncodeTLV;	/* encode function */
    pf_decode_TLV pfDecodeTVL;  /* decode function */
}NS_MSG_DATA_DESC_ST;

/*
typedef struct tag_MsgUserInfo
{
    UCHAR ucUser[32];
    UINT  uiAget;
    UINT  code;
}MSG_USER_INFO_ST;
UINT encode_TLV (IN VOID *pStruct, OUT VOID *pDataFlow)
{
    MSG_USER_INFO_ST *pstUserInfo = NULL; 

    pstUserInfo = (MSG_USER_INFO_ST *)pStruct;
     
    return len; 
}
UINT decode_TLV (IN VOID *pDataFlow, OUT VOID *pStruct)
{
    MSG_USER_INFO_ST *pstUserInfo = NULL; 

    pstUserInfo = (MSG_USER_INFO_ST *)pStruct;
     
    return len; 
}
*/

/**********************************************
 * Main message type:
 * MSG_MT_MNG: 连接的建立等(join, join response,...)
 * MSG_MT_CTL: 相互控制(升级，开启功能)
 * MSG_MT_DAT: 大量的数据传输
 * *******************************************/
typedef enum tag_Msg_main_type_list
{
    _MSG_MT_NONE_ = 0,
    
    MSG_MT_MNG, /* managent */
    MSG_MT_CTL, /* control */
    MSG_MT_DAT, /* data */

    _MSG_MT_MAX_,
}MSG_MAIN_TYPE_EN;

typedef enum tag_Msg_sub_type_list
{
    MSG_MNG_START,
    MSG_MNG_JOIN_REQ,
    MSG_MNG_JOIN_RESP,
    MSG_MNG_CONFIRM,
    MSG_MNG_OK,
    MSG_MNG_END,
    MSG_CTL_START = MSG_MNG_END + 1,
    MSG_CTL_ATTACH,
    MSG_CTL_ATTACH_RESP,
    MSG_CTL_UPGRADE,
    MSG_CTL_END,
    MSG_DAT_START = MSG_CTL_END + 1,
    MSG_DAT_END,

    _MSG_SUB_MAX_,
}MSG_SUB_TYPE_EN;

typedef enum tag_Msg_Attach_resp_status_code
{
    MSG_ATTACH_RESP_STATUS_SUCCESS = 0,
    MSG_ATTACH_RESP_STATUS_NEED_UPGRAED,

    MSG_ATTACH_RESP_STATUS_NEED_MAX,
}MSG_ATTACH_RESP_STATUS_EN;

typedef enum tag_Msg_ctl_upgrade_cmd
{
    MSG_CTL_UPGRADE_CMD_NONE = 0,
    MSG_CTL_UPGRADE_CMD_DO_UPGTADE,

    MSG_CTL_UPGRADE_CMD_MAX,
}MSG_CTL_UPGRADE_CMD_EN;

/* type in msg list 注意区别 MSG_SUB_TYPE_EN中的MSG_DAT_xxx 
 * 此处为消息结构类型，相当于sub-sub，三级类型。
 * */
#include <ns_msg_type.h>
#if 0 //move to <ns_msg_type.h> file
typedef enum tag_Msg_msg_type 
{
	MSG_MSG_TYPE_START = _MSG_SUB_MAX_,

	MSG_MSG_TYPE_CLIENT_DEV_INFO,

	_MSG_MSG_TYPE_MAX_,
}MSG_MSG_TYPE_EN;

typedef struct tag_msg_mng_join_req
{
    UINT uiClientID;
    UCHAR ucClientMac[6];
}MSG_MNG_JOIN_REQ_ST;

typedef struct tag_msg_mng_join_resp
{
    UINT  uiServerID;
    ULONG ulSessionID;
}MSG_MNG_JOIN_RESP_ST;

typedef struct tag_msg_mng_confirm
{
    UINT uiConfirmID;
}MSG_MNG_CONFIRM_ST;

typedef struct tag_msg_mng_ok
{
    UINT uiOKID;
}MSG_MNG_OK_ST;


typedef struct tag_msg_ctl_attach
{
    UINT uiAttachMode; /* client mode: slave, mange, server. */
    UINT uiCmdVer; 
    UINT uiCmdID;
}MSG_CTL_ATTACH_ST;

#endif

UINT NS_MSG_DATA_ENCODE_USHORT (INOUT UCHAR *pucBufStart, USHORT usData);
UINT NS_MSG_DATA_ENCODE_UINT (INOUT UCHAR *pucBufStart, UINT uiData);
UINT NS_MSG_DATA_ENCODE_ULONG (INOUT UCHAR *pucBufStart, ULONG ulData);
UINT NS_MSG_DATA_ENCODE_DATAFLOW(INOUT UCHAR *pucBufStart, UINT uiBufLen, UCHAR *pucData, UINT uiDataLen);
UINT NS_MSG_DATA_ENCODE_STRING(INOUT UCHAR *pucBufStart, UINT uiBufLen, IN const CHAR *pucString, UINT uiStrLen);

UINT NS_MSG_DATA_DECODE_USHORT (IN UCHAR *pucBufStart, INOUT USHORT *pusData);
UINT NS_MSG_DATA_DECODE_UINT (IN UCHAR *pucBufStart, INOUT UINT *puiData);
UINT NS_MSG_DATA_DECODE_ULONG (IN UCHAR *pucBufStart, INOUT ULONG *pulData);
UINT NS_MSG_DATA_DECODE_DATAFLOW(OUT UCHAR *pucData, UINT uiDataLen, INOUT UCHAR *pucBufStart, UINT uiBufLen);
UINT NS_MSG_DATA_DECODE_STRING(OUT CHAR *pucString, OUT UINT uiStrBufLen, IN const CHAR *pucBufStart, IN UINT uiBufLen);

typedef struct tag_NsMsg
{
    DCL_HEAD_S stMsgHead;
    USHORT usMainType;
    USHORT usSubType;
    UINT   uiTotalLen;
    UINT   uiSeq;
}NS_MSG_ST;

typedef struct tag_NsMsgValData
{
    DCL_NODE_S stNode;

    UINT uiDataType;
    UINT uiDataLen;
    VOID *pDataFlow;
}NS_MSG_DATA_ST;

#define NS_MSG_DATA_TYPE_LEN     (4)
#define NS_MSG_DATA_LEN_LEN      (4)

#define NS_MSG_HEADER_MAINTYPE_LEN (2)
#define NS_MSG_HEADER_SUBTYPE_LEN  (2)
#define NS_MSG_HEADER_TOTALEN_LEN  (4)
#define NS_MSG_HEADER_SEQ_LEN      (4)
#define NS_MSG_HEADER_LEN          (NS_MSG_HEADER_MAINTYPE_LEN + NS_MSG_HEADER_SUBTYPE_LEN + \
                                    NS_MSG_HEADER_TOTALEN_LEN  + NS_MSG_HEADER_SEQ_LEN)

NS_MSG_ST * MSG_Create (IN USHORT usMainType,
                        IN USHORT usSubType);
VOID MSG_Destroy (NS_MSG_ST *pstMsg);
ULONG MSG_AddData (IN NS_MSG_ST *pstMsg,
                   IN UINT uiMsgType, 
                   INOUT VOID *pDataBuf, 
                   IN UINT uiBufLen);
ULONG MSG_GetData (IN NS_MSG_ST *pstMsg,
                   IN UINT uiMsgType, 
                   INOUT VOID *pDataBuf, 
                   IN UINT uiBufLen);
ULONG NS_MSG_MsgList2MsgBuf (IN NS_MSG_ST *pstNsMsg, IN UCHAR *pucMsgBuf, INOUT UINT *puiLen);
NS_MSG_ST *NS_MSG_MsgBuf2MsgList (IN UCHAR *pucMsgBuf, IN UINT uiLen);


INT  MSG_Desc_Init(VOID);
VOID MSG_Desc_Fini(VOID);
NS_MSG_DATA_DESC_ST * MSG_Desc_Register(IN UINT uiDType, 
                                        IN UINT uiDLen,
                                        pf_encode_TLV pfEnTLV,
                                        pf_decode_TLV pfDeTVL);
NS_MSG_DATA_DESC_ST * MSG_Desc_Get(IN UINT uiDType);
VOID MSG_Desc_UnRegisterAll(VOID);


INT MSG_SSL_recv(SSL *pstSSL, VOID *pRecvBuf, INT iBufLen);
INT MSG_normal_recv(INT iConnFd, VOID *pRecvBuf, INT iBufLen);
INT CLIENT_MSG_recv(MSG_CLT_LINK_ST *pstCltLink, VOID *pRecvBuf, INT iBufLen);
INT SERVER_MSG_send(IN MSG_SRV_LINK_ST *pstSrvLink, IN VOID *pBuf, IN SIZE_T ssSendBuflen);
INT CLIENT_MSG_send(IN MSG_CLT_LINK_ST *pstCltLink, IN VOID *pBuf, IN SIZE_T ssSendBuflen);
ULONG MSG_desc_reg(VOID);
UINT MSG_DATA_encode_TLV_normal(IN VOID *pStruct, OUT VOID *pDataFlow, UINT uiDataFlowLen, INT iMsgType);
UINT MSG_DATA_decode_TLV_normal(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct, INT iMsgType);

#if 0
#define NS_MSG_DESC_REG(stype) \
    MSG_Desc_Register(stype, \
            sizeof(stype##_ST), \
            MSG_DATA_encode_TLV_##stype,  \
            MSG_DATA_decode_TLV_##stype)
#endif
#define NS_MSG_DESC_REG(stype) \
    MSG_Desc_Register(stype, \
            sizeof(stype##_ST), \
            NULL,  \
            NULL)

#define NS_MSG_DATA_ENCODE_TLV(stype) \
    /* STATIC */ UINT MSG_DATA_encode_TLV_##stype(IN VOID *pStruct,   OUT VOID *pDataFlow, UINT uiDataFlowLen)
#define NS_MSG_DATA_DECODE_TLV(stype) \
    /* STATIC */ UINT MSG_DATA_decode_TLV_##stype(IN VOID *pDataFlow, IN UINT uiDataFlowLen, OUT VOID *pStruct)

#endif //__MSG_H__
