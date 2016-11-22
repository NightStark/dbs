#include "include/debug.h"
#include "list.h"

/**********************************************************************************/
/* 双向循环链表  ( 带有尾巴节点 ) */
/* Double Cricle List with Tail(DTQ)*/
/**********************************************************************************/
   

void inline LILI_Init(DTQ_HEAD_S *pstHead, DTQ_TAIL_S *pstTail)
{
	pstHead->stNode.pstNext = &pstTail->stNode;
	pstHead->stNode.pstPrev = &pstTail->stNode;

	pstTail->stNode.pstNext = &pstHead->stNode;
	pstTail->stNode.pstPrev = &pstHead->stNode;

	return;
}

void inline LILI_Fint(DTQ_HEAD_S *pstHead, DTQ_TAIL_S *pstTail)
{
	pstHead->stNode.pstNext = NULL;
	pstHead->stNode.pstPrev = NULL;

	pstTail->stNode.pstNext = NULL;
	pstTail->stNode.pstPrev = NULL;

	return;
}

void inline LILI_AddHead(DTQ_HEAD_S *pstHead, DTQ_NODE_S *pstNode)
{
	pstNode->pstPrev = &pstHead->stNode;
	pstNode->pstNext =  pstHead->stNode.pstNext;
	
	pstHead->stNode.pstNext   = pstNode;
	pstNode->pstNext->pstPrev = pstNode;

	return;
}

void inline LILI_AddTail(DTQ_TAIL_S *pstTail, DTQ_NODE_S *pstNode)
{
	pstNode->pstPrev =  pstTail->stNode.pstPrev;
	pstNode->pstNext = &pstTail->stNode;

	pstNode->pstPrev->pstNext = pstNode;
	pstTail->stNode.pstPrev   = pstNode;

	return;
}

void inline LILI_AddAfter(DTQ_NODE_S *pstOldNode, DTQ_NODE_S *pstNewNode)
{
	pstNewNode->pstPrev = pstOldNode;
	pstNewNode->pstNext = pstOldNode->pstNext;

	pstOldNode->pstNext = pstNewNode;
	pstNewNode->pstNext->pstPrev = pstNewNode;

	return;
}

void inline LILI_AddBefore(DTQ_NODE_S *pstOldNode, DTQ_NODE_S *pstNewNode)
{
	pstNewNode->pstPrev = pstOldNode->pstPrev;
	pstNewNode->pstNext = pstOldNode;
	
	pstNewNode->pstPrev->pstNext = pstNewNode;
	pstOldNode->pstPrev = pstNewNode;
	
	return;
}

void inline LILI_Del(DTQ_NODE_S *pstDNode, PF_FREE pfFree)
{
	pstDNode->pstPrev->pstNext = pstDNode->pstNext;
	pstDNode->pstNext->pstPrev = pstDNode->pstPrev;
	
	if (NULL != pfFree)
		pfFree(pstDNode);
	
	return;
}

void inline LILI_DelFirst(DTQ_HEAD_S *pstHead, PF_FREE pfFree)
{
	DTQ_NODE_S *pstDelNode = pstHead->stNode.pstNext;

	pstHead->stNode.pstNext          =  pstHead->stNode.pstNext->pstNext;
	pstHead->stNode.pstNext->pstPrev = &pstHead->stNode;
	
	if (NULL != pfFree)
		pfFree(pstDelNode);
	
	return;
}

void inline LILI_DelLast(DTQ_TAIL_S *pstTail, PF_FREE pfFree)
{
	DTQ_NODE_S *pstDelNode = pstTail->stNode.pstPrev;

	pstTail->stNode.pstPrev          =  pstTail->stNode.pstPrev->pstPrev;
	pstTail->stNode.pstPrev->pstNext = &pstTail->stNode;

	if (NULL != pfFree)
		pfFree(pstDelNode);
	
	return;
}


/* WRAING: this function can delete the head and the tail node */
void inline LILI_DelAfter(DTQ_NODE_S *pstFNode, PF_FREE pfFree)
{
	DTQ_NODE_S *pstDelNode = pstFNode->pstNext;

	pstFNode->pstNext          =  pstFNode->pstNext->pstNext;
	pstFNode->pstNext->pstPrev = pstFNode;

	if (NULL != pfFree)
		pfFree(pstDelNode);
	
	return;
}

/* WRAING: this function can delete the head and the tail node */
void inline LILI_DelBefore(DTQ_NODE_S *pstANode, PF_FREE pfFree)
{
	DTQ_NODE_S *pstDelNode = pstANode->pstPrev;
	
	pstANode->pstPrev          = pstANode->pstPrev->pstPrev;
	pstANode->pstPrev->pstNext = pstANode;

	if (NULL != pfFree)
		pfFree(pstDelNode);
	
	return;
}

/**********************************************************************************/
/* 双向循环链表  ( 不带有尾巴节点 ) */
/* Double Cricle List(DCL)*/
/**********************************************************************************/

void inline DCL_Init(DCL_HEAD_S *pstHead)
{
	DBGASSERT(NULL != pstHead);

	pstHead->stNode.pstPrev = &(pstHead->stNode);
	pstHead->stNode.pstNext = &(pstHead->stNode);


    pstHead->uiLiLiLength = 0;
	return;
}

void inline DCL_Fint(DCL_HEAD_S *pstHead)
{
	DBGASSERT(NULL != pstHead);
	
	pstHead->stNode.pstPrev = &(pstHead->stNode);
	pstHead->stNode.pstNext = &(pstHead->stNode);

	return;
}

void inline DCL_AddHead(DCL_HEAD_S *pstHead, DCL_NODE_S *pstNode)
{
	DBGASSERT(NULL != pstHead);
	DBGASSERT(NULL != pstNode);
	
	pstNode->pstPrev = &pstHead->stNode;
	pstNode->pstNext =	pstHead->stNode.pstNext;
	
	pstHead->stNode.pstNext = pstNode;
	pstNode->pstNext->pstPrev = pstNode;

	return;
}

void inline DCL_AddTail(DCL_HEAD_S *pstHead, DCL_NODE_S *pstNode)
{
	DBGASSERT(NULL != pstHead);
	DBGASSERT(NULL != pstNode);
	
	pstNode->pstPrev =	pstHead->stNode.pstPrev;
	pstNode->pstNext = &pstHead->stNode;

	pstNode->pstPrev->pstNext = pstNode;
	pstHead->stNode.pstPrev   = pstNode;

	return;
}

/* pstOldNode 不会是头节点 */
void inline DCL_AddAfter(DCL_NODE_S *pstOldNode, DCL_NODE_S *pstNewNode)
{
	DBGASSERT(NULL != pstOldNode);
	DBGASSERT(NULL != pstNewNode);
	
	pstNewNode->pstPrev = pstOldNode;
	pstNewNode->pstNext = pstOldNode->pstNext;

	pstOldNode->pstNext = pstNewNode;
	pstNewNode->pstNext->pstPrev = pstNewNode;

	return;
}

void inline DCL_AddBefore(DCL_NODE_S *pstOldNode, DCL_NODE_S *pstNewNode)
{
	DBGASSERT(NULL != pstOldNode);
	DBGASSERT(NULL != pstNewNode);
	
	pstNewNode->pstPrev = pstOldNode->pstPrev;
	pstNewNode->pstNext = pstOldNode;
	
	pstNewNode->pstPrev->pstNext = pstNewNode;
	pstOldNode->pstPrev = pstNewNode;
	
	return;
}

void inline DCL_Del(DCL_NODE_S *pstDNode)
{
	DBGASSERT(NULL != pstDNode);
	
	pstDNode->pstPrev->pstNext = pstDNode->pstNext;
	pstDNode->pstNext->pstPrev = pstDNode->pstPrev;
		
	return;
}

void inline DCL_DelFirst(DCL_HEAD_S *pstHead, PF_FREE pfFree)
{
	DCL_NODE_S *pstDelNode = NULL;

	DBGASSERT(NULL != pstHead);
	DBGASSERT(NULL != pfFree);

	if (DCL_IS_EMPTY(pstHead))
	{
		return;
	}
	
	pstDelNode = pstHead->stNode.pstNext;

	DCL_Del(pstDelNode);

	pfFree(pstDelNode);
			
	return;
}

void inline DCL_DelLast(DCL_HEAD_S *pstHead, PF_FREE pfFree)
{
	DCL_NODE_S *pstDelNode = NULL;
	
	DBGASSERT(NULL != pstHead);
	DBGASSERT(NULL != pfFree);

	if (DCL_IS_EMPTY(pstHead))
	{
		return;
	}
	
	pstDelNode = pstHead->stNode.pstPrev;
	
	DCL_Del(pstDelNode);

	if (NULL != pfFree)
		pfFree(pstDelNode);
	
	return;
}


/* WRAING: this function can delete the head and the tail node */
void inline DCL_DelAfter(DCL_HEAD_S *pstHead, DCL_NODE_S *pstFNode, PF_FREE pfFree)
{
	DCL_NODE_S *pstDelNode = NULL;
	
	DBGASSERT(NULL != pstFNode);
	DBGASSERT(NULL != pfFree);

	if(pstFNode->pstNext == &(pstHead->stNode))
	{
		return;
	}
	
	pstDelNode = pstFNode->pstNext;

	
	DCL_Del(pstDelNode);

	if (NULL != pfFree)
		pfFree(pstDelNode);
	return;
}

/* WRAING: this function can delete the head and the tail node */
void inline DCL_DelBefore(DCL_HEAD_S *pstHead, DCL_NODE_S *pstANode, PF_FREE pfFree)
{
	DCL_NODE_S *pstDelNode = NULL;
	
	DBGASSERT(NULL != pstANode);
	DBGASSERT(NULL != pfFree);

	if(pstANode->pstNext == &(pstHead->stNode))
	{
		return;
	}
	
	pstDelNode = pstANode->pstPrev;
	
	DCL_Del(pstDelNode);

	if (NULL != pfFree)
		pfFree(pstDelNode);
	return;		
}


