#ifndef __AVL_H__
#define __AVL_H__

typedef struct tag_AVLNode
{
	unsigned long       ulHeight;
	struct tag_AVLNode *pstLeft;
	struct tag_AVLNode *pstRight;
}AVL_NODE_S;

/* AVL_INSERT_CMP_CALLBACK(New Node, Current Node) */
typedef INT  (*AVL_INSERT_CMP_CALLBACK)(AVL_NODE_S *, AVL_NODE_S *);
/* AVL_FIND_CMP_CALLBACK(compare key index, point to key, current node) */
typedef INT  (*AVL_FIND_CMP_CALLBACK)(ULONG ,VOID *, AVL_NODE_S *);
/* AVL_FREE_CALLBACK(current node) */
typedef VOID (*AVL_FREE_CALLBACK)(AVL_NODE_S *);
/* AVL_COPY_CALLBACK(Destnation node ,Source node) */
typedef VOID (*AVL_COPY_CALLBACK)(AVL_NODE_S *, AVL_NODE_S *);

VOID AVL_Adjust(ULONG ulSonWay, AVL_NODE_S **ppstCurNode);
ULONG AVL_RemoveNode(AVL_NODE_S**ppstCurNode, AVL_FREE_CALLBACK pfAvlFreeCB);

#define AVL_SYS_MAX (4096)

typedef struct tag_AvlInfo
{
	DCL_NODE_S  stNode;

	LONG lAvlID;
	AVL_NODE_S *pstAvlHead;

	AVL_INSERT_CMP_CALLBACK pfAvlInsertCmpCb;
	AVL_FIND_CMP_CALLBACK   pfAvlFindCmpCb;
	AVL_FREE_CALLBACK		pfAvlFreeCb;
	AVL_COPY_CALLBACK		pfAvlCopyCb;
}AVL_INFO_S;

ULONG AVL_Sys_Init(VOID);
VOID  AVL_Sys_Fini(VOID);
AVL_INFO_S * AVL_Init(AVL_INSERT_CMP_CALLBACK pfAvlInsertCmpCallBack,
 					  AVL_FIND_CMP_CALLBACK   pfAvlFindCmpCallBack,
 					  AVL_FREE_CALLBACK       pfAvlFreeCallBack,
 					  AVL_COPY_CALLBACK       pfAvlCopyCallBack);
VOID AVL_Fini(AVL_INFO_S *pstAvlInfo);
VOID AVL_InsertNode(AVL_NODE_S *pstAvlNode, AVL_INFO_S *pstAvl);
VOID AVL_DeleteNode(VOID *pKey, AVL_INFO_S *pstAvl);
AVL_NODE_S *AVL_FindNode(VOID *pKey, AVL_INFO_S *pstAvl);

#endif //__AVL_H__

