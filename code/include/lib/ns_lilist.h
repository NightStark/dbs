#ifndef __LILIST_H__
#define __LILIST_H__

typedef void (*PF_FREE)(void *);

#if 0
/**********************************************************************************/
/* 双向循环链表  ( 带有尾巴节点 ) */
/**********************************************************************************/

typedef struct tag_DtqNode
{
	struct tag_DtqNode *pstPrev;
	struct tag_DtqNode *pstNext;
}DTQ_NODE_S;

typedef struct tag_DtqHead
{
	void *pHeadInfo;
	unsigned int uiDTQLength;
	DTQ_NODE_S stNode;
}DTQ_HEAD_S;

typedef struct tag_DtqTail
{
	void *pTailInfo;
	DTQ_NODE_S stNode;
}DTQ_TAIL_S;

#define DTQ_NEXT(pstNode)	\
		pstNode->pstNext

#define DTQ_ENTYR(ptr, type, member) 	\
	container_of(ptr, type, member)
	
#define DTQ_NEXT_ENTRY(pstNode, type, member)	\
	(pstNode->pstNext) == (pstNode->pstPrev))?	\
	(NULL):										\
	container_of(pstNode->pstNext, type, member)
	

#define DTQ_FOREACH(pstHead, pstNode, pstTail) 	\
	for(pstNode  = pstHead->stNode.pstNext; 	\
		pstNode != &pstTail->stNode; 			\
		pstNode  = pstNode->pstNext)		 
		
#define DTQ_FOREACH_ENTRY(pstHead, pstEntry, stNode, pstTail) \
	for((pstEntry) = container_of((pstHead)->stNode.pstNext, typeof(*(pstEntry)), (stNode));\
		(pstEntry) != NULL;\
		(pstEntry) = ((pstEntry)->stNode.pstNext == &((pstTail)->stNode))?(NULL):\
		container_of((pstEntry)->stNode.pstNext, typeof(*(pstEntry)), (stNode)))	   

void  LILI_Init(DTQ_HEAD_S *pstHead, DTQ_TAIL_S *pstTail);

void  LILI_Fint(DTQ_HEAD_S *pstHead, DTQ_TAIL_S *pstTail);

void  LILI_AddHead(DTQ_HEAD_S *pstHead, DTQ_NODE_S *pstNode);

void  LILI_AddTail(DTQ_TAIL_S *pstTail, DTQ_NODE_S *pstNode);

void  LILI_AddAfter(DTQ_NODE_S *pstOldNode, DTQ_NODE_S *pstNewNode);

void  LILI_AddBefore(DTQ_NODE_S *pstOldNode, DTQ_NODE_S *pstNewNode);


void  LILI_Del(DTQ_NODE_S *pstDNode, PF_FREE pfFree);

void  LILI_DelFirst(DTQ_HEAD_S *pstHead, PF_FREE pfFree);

void  LILI_DelLast(DTQ_TAIL_S *pstTail, PF_FREE pfFree);


/* WRAING: this function can delete the head and the tail node */
void  LILI_DelAfter(DTQ_NODE_S *pstFNode, PF_FREE pfFree);

/* WRAING: this function can delete the head and the tail node */
void  LILI_DelBefore(DTQ_NODE_S *pstANode, PF_FREE pfFree);
#endif

/**********************************************************************************/
/* 双向循环链表  ( 不带有尾巴节点 ) */
/**********************************************************************************/
typedef struct tag_DclNode
{
	struct tag_DclNode *pstPrev;
	struct tag_DclNode *pstNext;
}DCL_NODE_S;

typedef struct TAG_DclHead
{
	void *pHeadInfo;
	unsigned int uiLiLiLength;
	DCL_NODE_S stNode;
}DCL_HEAD_S;

#define DCL_IS_EMPTY(pstHead)\
	(((pstHead)->stNode.pstNext == &((pstHead)->stNode)) ? (1) : (0) )

#define DCL_IS_END(pstHead, pstNode) 	\
	((pstNode)->pstNext == &((pstHead)->stNode) ? (1) : (0))

#define DCL_NEXT(pstHead, pstNode) \
	(((DCL_IS_EMPTY(pstHead) || DCL_IS_END(pstHead, pstNode)) ? (NULL) : (pstNode)->pstNext))

#define DCL_NEXT_ENTRY(pstHead, pstEntry, stNode) \
	((NULL == DCL_NEXT(pstHead, &(pstEntry->stNode)) ? (NULL) :			\
	 container_of(DCL_NEXT(pstHead, &(pstEntry->stNode)), typeof(*(pstEntry)), stNode)))
	

#define DCL_FIRST(pstHead) \
	((DCL_IS_EMPTY(pstHead) ? (NULL) : (pstHead)->stNode.pstNext))

#define DCL_FIRST_ENTRY(pstHead, pstEntry, stNode) 	\
	((NULL == DCL_FIRST(pstHead) ? (NULL) :			\
	 container_of(DCL_FIRST(pstHead), typeof(*(pstEntry)), stNode)))

#define DCL_ENTRY(ptr, type, member)\
	container_of(ptr, type, member)
	
#define DCL_FOREACH(pstHead, pstNode, stNode)  			  			\
for((pstNode) = (pstHead)->stNode.pstNext;						    \
	((pstNode) != NULL) &&  pstNode->pstNext != &((pstHead)->stNode);  \
	(pstNode) == (pstNode)->pstNext)

#define DCL_FOREACH_ENTRY(pstHead, pstEntry, stNode) 					\
	for(pstEntry = (DCL_IS_EMPTY((pstHead)))?(NULL):					\
			DCL_FIRST_ENTRY(pstHead,pstEntry,stNode);					\
		pstEntry != NULL;												\
		pstEntry = (DCL_IS_END(pstHead, &(pstEntry)->stNode))?(NULL):	\
			DCL_NEXT_ENTRY(pstHead,pstEntry,stNode))

#define DCL_FOREACH_ENTRY_SAFE(pstHead, pstEntry, pstEntryNext, stNode) 	\
		for((pstEntry = (DCL_IS_EMPTY((pstHead)))?(NULL):					\
				        DCL_FIRST_ENTRY(pstHead,pstEntry,stNode)),			\
				(pstEntryNext = (NULL == pstEntry)?(NULL):							\
									((DCL_IS_END((pstHead),&((pstEntry)->stNode)))?(NULL): \
											 DCL_NEXT_ENTRY(pstHead,pstEntry,stNode)));\
			pstEntry != NULL;												\
			pstEntry = pstEntryNext,										\
				pstEntryNext = (NULL == pstEntry)?(NULL):							\
									((DCL_IS_END((pstHead),&((pstEntry)->stNode)))?(NULL):	\
			    			    	         DCL_NEXT_ENTRY(pstHead,pstEntry,stNode)))					


/*
#define list_for_each_entry_safe(pos, n, head, member)          \
for (pos = list_entry((head)->next, typeof(*pos), member),  \
n = list_entry(pos->member.next, typeof(*pos), member); \
&pos->member != (head);                    \
pos = n, n = list_entry(n->member.next, typeof(*n), member))
*/


void  DCL_Init(DCL_HEAD_S *pstHead);

void  DCL_Fint(DCL_HEAD_S *pstHead);

void  DCL_AddHead(DCL_HEAD_S *pstHead, DCL_NODE_S *pstNode);

void  DCL_AddTail(DCL_HEAD_S *pstHead, DCL_NODE_S *pstNode);

/* pstOldNode 不会是头节点 */
void  DCL_AddAfter(DCL_NODE_S *pstOldNode, DCL_NODE_S *pstNewNode);

void  DCL_AddBefore(DCL_NODE_S *pstOldNode, DCL_NODE_S *pstNewNode);

void  DCL_Del(DCL_NODE_S *pstDNode);

void  DCL_DelFirst(DCL_HEAD_S *pstHead, PF_FREE pfFree);

void  DCL_DelLast(DCL_HEAD_S *pstHead, PF_FREE pfFree);


/* WRAING: this function can delete the head and the tail node */
void  DCL_DelAfter(DCL_HEAD_S *pstHead, DCL_NODE_S *pstFNode, PF_FREE pfFree);

/* WRAING: this function can delete the head and the tail node */
void  DCL_DelBefore(DCL_HEAD_S *pstHead, DCL_NODE_S *pstANode, PF_FREE pfFree);

#define LILI_NT_DELALL_ENTRY(pstHead, pstEntry, stNode, pfree) 							\
		for(pstEntry = ((pstHead)->stNode.pstNext == NULL)?(NULL):						\
				container_of((pstHead)->stNode.pstNext, typeof(*(pstEntry)), stNode);	\
			pstEntry != NULL;															\
			pstEntry = ((pstEntry)->stNode.pstNext == &((pstHead)->stNode))?(NULL): 	\
				container_of((pstEntry)->stNode.pstNext, typeof(*(pstEntry)), stNode))	\
		{																				\
			LILI_Del(&((pstEntry)->stNode), pfree)										\
		}

#endif //__LILIST_H__
