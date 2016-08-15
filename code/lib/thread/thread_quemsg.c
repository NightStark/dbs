#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_thread_quemsg.h>
#include <ns_thread.h>

#ifdef DBG_MOD_ID
#undef  DBG_MOD_ID
#endif
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_THREAD, 1)

STATIC INT g_uiThreadQueMsgSeqNumBase = 0;

typedef struct tag_ThreadQueMsgNode
{
	INT iSrcThrdID;
	INT iQueMsgSeqNum;
	BOOL_T bNeedRespNow; /* 发消息者在等待回复 */

	THREAD_QUEMSG_DATA_S stQuemsgData;
	
	DCL_NODE_S stNode;
}THREAD_QUEMSG_NODE_S;


ULONG THREAD_quemsg_Init(VOID)
{

	return ERROR_SUCCESS;
}

VOID THREAD_quemsg_Fint(VOID)
{
	
	return ;
}

INT Thread_quemsg_GenerateSeqNum(VOID)
{

	if (g_uiThreadQueMsgSeqNumBase < 0 )
		g_uiThreadQueMsgSeqNumBase = 0;
	return g_uiThreadQueMsgSeqNumBase++;
	
}

STATIC THREAD_QUEMSG_NODE_S * thread_quemsg_NodeGetBySeqNum(IN INT iQueMsgSeqNum,
															IN const DCL_HEAD_S *pstHead)
{
	THREAD_QUEMSG_NODE_S *pstThrdQueMsgNode = NULL;

	DBGASSERT(NULL != pstHead);
	
	DCL_FOREACH_ENTRY(pstHead,pstThrdQueMsgNode,stNode)
	{
		if (pstThrdQueMsgNode->iQueMsgSeqNum == iQueMsgSeqNum)
		{
			break;
		}
	}

	return pstThrdQueMsgNode;
}


STATIC THREAD_QUEMSG_NODE_S * thread_quemsg_NodeAdd(IN DCL_HEAD_S * pstHead)
{
    THREAD_QUEMSG_NODE_S *pstThrdQueMsgNode = NULL;
	pstThrdQueMsgNode = mem_alloc(sizeof(THREAD_QUEMSG_NODE_S));
	if (NULL == pstThrdQueMsgNode)
	{
		return NULL;
	}

	pstThrdQueMsgNode->iSrcThrdID = -1;
	pstThrdQueMsgNode->iQueMsgSeqNum = 0;

	DCL_AddTail(pstHead, &(pstThrdQueMsgNode->stNode));

	return pstThrdQueMsgNode;
}

STATIC VOID thread_quemsg_NodeFree(VOID *pstNode)
{
    THREAD_QUEMSG_NODE_S *pstThrdQueMsgNode;

    DBGASSERT(NULL != pstNode);

    pstThrdQueMsgNode = DCL_ENTRY(pstNode, THREAD_QUEMSG_NODE_S, stNode);

    free(pstThrdQueMsgNode);
    
    return;
}

/* 
暂时未被使用 
STATIC VOID thread_quemsg_NodeDel(IN DCL_HEAD_S * pstHead)
{
	// 释放消息节点
	DCL_DelFirst(pstHead, thread_quemsg_NodeFree);
	
	return;
}
*/

ULONG THREAD_quemsg_Write(IN INT iQMSNum,
						  IN BOOL_T bNeedResp,
						  IN THREAD_QUEMSG_INFO_S *pstThrdQueMsg,
						  INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsgData)
{	
	
	THREAD_INFO_S *pstSelfThrdInfo;
	THREAD_QUEMSG_NODE_S *pstThrdQueMsgNode = NULL;

	DBGASSERT(NULL != pstThrdQueMsg);
	DBGASSERT(NULL != pstThrdQueMsgData);
	

	pstSelfThrdInfo = Thread_server_GetCurrent();

	DBGASSERT(NULL != pstSelfThrdInfo);
	
    /* 分配消息节点 & 挂到目标线程的消息列队链表之下 */
	pstThrdQueMsgNode = thread_quemsg_NodeAdd(&(pstThrdQueMsg->stThreadQuemsgHead));
	if (NULL == pstThrdQueMsgNode)
	{
	    ERR_PRINTF("thread quemsg Node Add Failed!");
		return ERROR_NOT_ENOUGH_MEM;
	}

	pstThrdQueMsgNode->iSrcThrdID    = pstSelfThrdInfo->iThreadID;
	pstThrdQueMsgNode->iQueMsgSeqNum = iQMSNum;
	pstThrdQueMsgNode->bNeedRespNow  = bNeedResp;
	memcpy(&(pstThrdQueMsgNode->stQuemsgData),
			pstThrdQueMsgData,
			sizeof(THREAD_QUEMSG_DATA_S));

	pstThrdQueMsg->uiThreadQuemsgLen++;
	
	return ERROR_SUCCESS;
}

/* 
	只可以再目标线程里面读取
	消息被读取后，节点会被删除，数据放到目标线程的临时变量之中。
*/

ULONG THREAD_quemsg_Read(IN THREAD_QUEMSG_INFO_S *pstThrdQueMsg,						 
						 OUT INT *piSrcThrdId,
						 INOUT INT *piQMSNum, 
						 OUT BOOL_T *pbNeedResp,
						 INOUT THREAD_QUEMSG_DATA_S *pstThrdQueMsgData)
{	
	INT iQMSNum = *piQMSNum;
	DCL_HEAD_S *pstHead = &(pstThrdQueMsg->stThreadQuemsgHead);
	THREAD_QUEMSG_NODE_S *pstThrdQueMsgNode = NULL;

	DBGASSERT(NULL != piSrcThrdId);
	DBGASSERT(NULL != piQMSNum);
	DBGASSERT(NULL != pbNeedResp);
	DBGASSERT(NULL != pstThrdQueMsg);
	DBGASSERT(NULL != pstThrdQueMsgData);

	if (0 != iQMSNum) /* SeqNum 不为零则认为，需要读取指定的回复消息，但不一定存在 */
	{
		pstThrdQueMsgNode = thread_quemsg_NodeGetBySeqNum(iQMSNum,pstHead);
	}
	else
	{
		pstThrdQueMsgNode = DCL_FIRST_ENTRY(pstHead,
											pstThrdQueMsgNode,
											stNode);
	}
	
	if (NULL == pstThrdQueMsgNode)
	{
	    MSG_PRINTF("Thread msg is empty!");
		return ERROR_QUEMSG_EMPTY;
	}

	*piSrcThrdId = pstThrdQueMsgNode->iSrcThrdID;
	*piQMSNum    = pstThrdQueMsgNode->iQueMsgSeqNum;
	*pbNeedResp  = pstThrdQueMsgNode->bNeedRespNow;
	
	memcpy(pstThrdQueMsgData, 
			&(pstThrdQueMsgNode->stQuemsgData),
			sizeof(THREAD_QUEMSG_DATA_S));

	/* 释放消息节点 */
	//thread_quemsg_NodeDel(&(pstThrdInfo->stThrdQueMsg.stThreadQuemsgHead));
	DCL_Del(&(pstThrdQueMsgNode->stNode));
	free(pstThrdQueMsgNode);

	pstThrdQueMsg->uiThreadQuemsgLen--;

	MSG_PRINTF("Thread Quemsg Len %d",pstThrdQueMsg->uiThreadQuemsgLen);
		
	return ERROR_SUCCESS;
}

/* 释放不会被读取的消息节点 */
VOID THREAD_quemsg_Free(IN INT iQueMsgSeqNum)
{
    /* QueMsgSeqNum暂时未实现 */
    return ;
}

VOID THREAD_quemsg_DestroyAll(IN DCL_HEAD_S *pstThreadQuemsgHead)
{	
	DBGASSERT(NULL != pstThreadQuemsgHead);

	while(!DCL_IS_EMPTY(pstThreadQuemsgHead))
	{
		DCL_DelFirst(pstThreadQuemsgHead, thread_quemsg_NodeFree);	
	}

	return;
}

