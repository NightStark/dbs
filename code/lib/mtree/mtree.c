#include <ns_base.h>

#include <ns_lilist.h>

#define MTREE_LEVEL_DEEP_MAX (32)

#define MTREE_KEY_JUDGE_LESS    ((ULONG)(0 - 1))
#define MTREE_KEY_JUDGE_EQUAL   ((ULONG)(0))
#define MTREE_KEY_JUDGE_GREATER ((ULONG)(1))

/* ULONG judge(UINT uiKey1, UINT uiKe2) */
typedef ULONG (*pfMTREEJUDGE)(UINT, UINT);

typedef struct tag_MTreeNode
{
	struct tag_MTreeNode stParent; 	/* point to the parent of this node */
	struct tag_MTreeNode stSonHead;	/* point to the head of the son list of this node*/
	struct tag_MTreeNode stPre;		/* point to the  previous node of this node */
	struct tag_MTreeNode stNext;	/* point to the next nodes of this node */

//	VOID *pKey;						/* The Key to find out one node is a son of this node or not */
	UINT  uiLeveKey[MTREE_LEVEL_DEEP_MAX]; /* Node have a key for ever level */
	VOID *pData;
}MTREE_NODE_S;

typedef struct tagMTreeHead
{
	MTREE_NODE_S *pstFirstNode;
	UINT		  uiSonCount;	/* the count of sons num */
	pfMTREEJUDGE  pfMTreeJudge;
}MTREE_HEAD_S;

ULONG MTree_Head_Init(IN MTREE_HEAD_S *pstMTreeHead)
{
	pstMTreeHead->pstFirstNode = NULL;
	pstMTreeHead->uiSonCount   = 0;

	return ERROR_SUCCESS;
}

VOID MTree_Head_Fini(IN MTREE_HEAD_S *pstMTreeHead)
{
	pstMTreeHead->pstFirstNode = NULL;
	pstMTreeHead->uiSonCount   = 0;

	return;
}

ULONG MTree_InsertNode(IN const MTREE_HEAD_S *pstMTreeHead, IN const MTREE_NODE_S *pstNewNode)
{
	DBGASSERT(NULL != pstMTreeHead);
	DBGASSERT(NULL != pstNewNode);

	if(NULL == pstMTreeHead)
	{
		pstMTreeHead->pstFirstNode = pstNewNode;
	}

	///* 1, calculate the key */
	/* 2, Insert before or after according to the key */
	pstMTreeHead->pfMTreeJudge(pstNewNode->uiLeveKey[X], pstMTreeHead->pstFirstNode->uiLeveKey[X]);
	/* 3, Recursion the key list */


	return ERROR_SUCCESS;
}



ULONG judge(UINT uiKey1, UINT uiKe2)
{
	if (uiKey1 < uiKe2)
	{
		return MTREE_KEY_JUDGE_LESS;
	}
	else if (uiKey1 == uiKe2)
	{
		return MTREE_KEY_JUDGE_EQUAL;
	}
	else
	{
		return MTREE_KEY_JUDGE_GREATER;
	}
}


