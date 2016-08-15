#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <ns_base.h>
#include <ns_id.h>
#include <ns_lilist.h>
#include <ns_avl.h>

#define SON_LEFT		(0x00010000UL)
#define SON_RIGHT		(0x00020000UL)
#define SON_SON_NONE	(0x00000000UL)
#define SON_SON_LEFT	(0x00000001UL)
#define SON_SON_RIGHT	(0x00000002UL)

#define BLCFCT_EP		(0UL)
#define BLCFCT_LT		(1UL)
#define BLCFCT_RT      	(-1UL)

/* 更该节点下树的高度，和平衡因子(不保证平衡) */
STATIC inline 
INT UpdataTreeHeight(AVL_NODE_S *pstCurNode)
{
	INT   iBalance = 0;
	ULONG ulRSonHeight = 0;
	ULONG ulLSonHeight = 0;

	if (NULL == pstCurNode)
	{
		return BLCFCT_EP;
	}

	if (NULL == pstCurNode->pstLeft && NULL == pstCurNode->pstRight)
	{
		pstCurNode->ulHeight = 0;
		return BLCFCT_EP;
	}

	ulLSonHeight = (NULL == pstCurNode->pstLeft  ? 0 : (pstCurNode->pstLeft->ulHeight  + 1));
	ulRSonHeight = (NULL == pstCurNode->pstRight ? 0 : (pstCurNode->pstRight->ulHeight + 1));

	pstCurNode->ulHeight = (ulLSonHeight > ulRSonHeight ? (ulLSonHeight) : (ulRSonHeight));
	iBalance = ulLSonHeight - ulRSonHeight;

	if ((iBalance <= -2) || (iBalance >= 2))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

STATIC inline 
INT GetTreeBalance(AVL_NODE_S *pstCurNode)
{
	INT iBalance = 0;
	ULONG ulRSonHeight = 0;
	ULONG ulLSonHeight = 0;

	if (NULL == pstCurNode || 0 == pstCurNode->ulHeight)
	{
		return BLCFCT_EP;
	}

	ulLSonHeight = (NULL == pstCurNode->pstLeft  ? 0 : (pstCurNode->pstLeft->ulHeight  + 1));
	ulRSonHeight = (NULL == pstCurNode->pstRight ? 0 : (pstCurNode->pstRight->ulHeight + 1));

	pstCurNode->ulHeight = (ulLSonHeight > ulRSonHeight ? (ulLSonHeight) : (ulRSonHeight));
	iBalance = ulLSonHeight - ulRSonHeight;

	return iBalance;
}

/* 左旋转 */
STATIC inline 
VOID Right_Rotate(AVL_NODE_S **ppstCurNode)
{
	AVL_NODE_S *pstNode;
	AVL_NODE_S *pstTemp;

	pstNode = *ppstCurNode;

	pstTemp = pstNode->pstLeft;
	pstNode->pstLeft = pstTemp->pstRight;
	pstTemp->pstRight = pstNode;

	UpdataTreeHeight(pstTemp->pstLeft);
	UpdataTreeHeight(pstTemp->pstRight);
	UpdataTreeHeight(pstTemp);

	*ppstCurNode = pstTemp;
	
	return;
}

/* 右旋转 */
STATIC inline 
VOID Left_Rotate(AVL_NODE_S **ppstCurNode)
{
	AVL_NODE_S *pstNode;
	AVL_NODE_S *pstTemp;

	pstNode = *ppstCurNode;

	pstTemp = pstNode->pstRight;
	pstNode->pstRight = pstTemp->pstLeft;
	pstTemp->pstLeft = pstNode;
	
	UpdataTreeHeight(pstTemp->pstLeft);
	UpdataTreeHeight(pstTemp->pstRight);
	UpdataTreeHeight(pstTemp);

	*ppstCurNode = pstTemp;

	return;
}

/* 
  旋转:
	根据插入方向进行不同的旋转
	@ulSon   :在子树的插入方向( 左/右 )
	@ulSonSon:在 子树的 子树 的插入方向( 左/右 )
	@ppstCurNode:指向当前节点的指针 的地址， 旋转后要把新的root地址放入该地址内，
				 以便让父节点找得到改树。
*/
STATIC inline 
VOID Rotate(ULONG ulSonWay, AVL_NODE_S **ppstCurNode)
{
	INT iSonBalance = 0;
	AVL_NODE_S *pstNode = NULL;
	AVL_NODE_S *pstSonNode = NULL;

	pstNode = *ppstCurNode;
	
	if (SON_LEFT == ulSonWay)
	{
		pstSonNode = pstNode->pstLeft;
	}
	else
	{
		pstSonNode = pstNode->pstRight;
	}
	
	if (NULL == pstSonNode)
	{
		*ppstCurNode = pstNode;
		
		return;
	}

	iSonBalance = GetTreeBalance(pstSonNode);

	/* 【左】孩子树插入 */
	if (SON_LEFT == ulSonWay)
	{
		if (BLCFCT_LT == iSonBalance)
		{
			/* 【左】孩子树【左】插入 */
			Right_Rotate(&pstNode);
		}
		else if (BLCFCT_RT == iSonBalance)
		{
			/* 【左】孩子树【右】插入 */
			Left_Rotate(&(pstNode->pstLeft));
			Right_Rotate(&(pstNode));
		}
		else if (BLCFCT_EP == iSonBalance)
		{
			/* 删除时可能出现这种情况:
					 R
					/ \
				   A   B
				  	  / \
				  	 C   D
				 删除A后，相当于又插入，与真正的插入不同的是，平衡建立的时机不同
				 此时失去平衡需要右旋转即可平衡
			*/
			if (0 != pstNode->ulHeight)Right_Rotate(&pstNode);
		}
	}
	/* 【右】孩子树插入 */
	else if (SON_RIGHT == ulSonWay)
	{
		if (BLCFCT_LT == iSonBalance)
		{
			/* 【右】孩子树【左】插入 */
			Right_Rotate(&(pstNode->pstRight));
			Left_Rotate(&(pstNode));
		}
		else if (BLCFCT_RT == iSonBalance)
		{
			/* 【右】孩子树【右】插入 */
			Left_Rotate(&pstNode);
		}
		/* 删除时才能有的情况吧，参考上面 */
		else if (BLCFCT_EP == iSonBalance)
		{
			if (0 != pstNode->ulHeight)Left_Rotate(&pstNode);
		}
	}
	
	/* 新的子树的 root扔给父节点 */
	*ppstCurNode = pstNode;

	return;
}

/* AVL树高度可平衡的调整 */
VOID AVL_Adjust(ULONG ulSonWay, AVL_NODE_S **ppstCurNode)
{
	INT iBalance = 0;
	AVL_NODE_S *pstNode = NULL;

	pstNode = *ppstCurNode;
	
	/* 更新树的高度 */
	iBalance = UpdataTreeHeight(pstNode);
	
	/* 如果树失去平衡则进行旋转 */
	if (0 != iBalance)
	{
		Rotate(ulSonWay, (&pstNode));
	}
	
	*ppstCurNode = pstNode;
	
	return;
}

/*
	return:
		0:节点已被删除
		1:节点已被替换，需要到左子树去删除
*/
ULONG AVL_RemoveNode(AVL_NODE_S**ppstCurNode, AVL_FREE_CALLBACK pfAvlFreeCB)
{
	AVL_NODE_S  stCpTemp;
	AVL_NODE_S* pstTemp = NULL;
	AVL_NODE_S* pstLittle = NULL;
	AVL_NODE_S* pstNode = NULL;

	pstNode = *ppstCurNode;

	if (NULL == pstNode->pstLeft)
	{
		/* 【左】子树为空，把【右】子树提上来 */
		pstTemp = pstNode;
		pstNode = pstNode->pstRight;
		*ppstCurNode = pstNode;
		if (NULL != pfAvlFreeCB)
		{
			pfAvlFreeCB(pstTemp);
		}
		
		return 0;
	}
	else if (NULL == pstNode->pstRight)
	{
		/* 【右】子树为空，把【左】子树提上来 */
		pstTemp = pstNode;
		pstNode = pstNode->pstLeft;
		*ppstCurNode = pstNode;
		if (NULL != pfAvlFreeCB)
		{
			pfAvlFreeCB(pstTemp);
		}

		return 0;
	}
	else
	{
		/* 左右子树都不为空，把比自己小的最大的数的节点与当前节点交换 */
		pstTemp = pstNode;
		pstLittle = pstTemp->pstLeft;
		while (NULL != pstLittle->pstRight)
		{
			pstTemp = pstLittle;
			pstLittle = pstTemp->pstRight;
		}
		memcpy(&stCpTemp, pstLittle, sizeof(AVL_NODE_S));
		memcpy(pstLittle, pstNode, sizeof(AVL_NODE_S)); /*	*/
		memcpy(pstNode, &stCpTemp, sizeof(AVL_NODE_S));
		
		if (pstLittle == pstLittle->pstLeft)
		{
			pstLittle->pstLeft = pstNode;
		}
		else
		{
			pstTemp->pstRight = pstNode;
		}
		
		*ppstCurNode = pstLittle;

		return 1;
	}
	
}

/****************************************************************************
*	AVL 通用接口															*
****************************************************************************/
INT iAvlIdHandle = 0;
DCL_HEAD_S *pstAVLSysHead = NULL;

/* 初始化 通用接口资源 */
ULONG AVL_Sys_Init(VOID)
{
	iAvlIdHandle  = CreateIDPool(AVL_SYS_MAX);
	if (iAvlIdHandle < 0)
	{
		return ERROR_FAILE;
	}
	
	pstAVLSysHead = malloc(sizeof(DCL_HEAD_S));
	if (NULL == pstAVLSysHead)
	{
		return ERROR_NOT_ENOUGH_MEM;
	}
	
	DCL_Init(pstAVLSysHead);
	
	return ERROR_SUCCESS;
}

VOID AVL_Sys_Fini(VOID)
{
	DCL_Fint(pstAVLSysHead);
	free(pstAVLSysHead);
	DestroyIDPool(iAvlIdHandle);

	return;
}

/* 所有的AVL通过一个链表来管理 */
STATIC VOID AVL_Sys_Add(AVL_INFO_S *pstAvlInfo)
{
	DCL_AddTail(pstAVLSysHead, &(pstAvlInfo->stNode));
	return;
}

STATIC VOID AVL_Sys_Delete(AVL_INFO_S *pstAvlInfo)
{
	DCL_Del(&(pstAvlInfo->stNode));
	free(pstAvlInfo);
	
	return;
}

/*
	递归传参时的指针定义 
*/
typedef struct tag_AVLInsertRecuisionPara
{
	AVL_NODE_S *pstNewNode;
	AVL_INSERT_CMP_CALLBACK pfAvlInsCmpCB;
}AVL_INSERT_REC_PARA_S;

typedef struct tag_AVLDeleteRecuisionPara
{
	VOID *pData;
	ULONG ulKeyIndex;
	AVL_FREE_CALLBACK pfAvlFreeCB;
	AVL_FIND_CMP_CALLBACK pfFindCmpCB;
}AVL_DELETE_REC_PARA_S;


 typedef struct tag_AVLFindRecuisionPara
 {
	 VOID *pKey;
	 ULONG ulKeyIndex;
	 AVL_FIND_CMP_CALLBACK pfFindCmpCB;
 } AVL_FIND_REC_PARA_S;

STATIC VOID avl_Insert(AVL_NODE_S **ppstCurNode, AVL_INSERT_REC_PARA_S *pstAvlInsertRecPara)
{
	INT iRet             = 0;
	ULONG ulSonWay       = 0;
	AVL_NODE_S  *pstNode = NULL;

	if (NULL == ppstCurNode)
	{
		return;
	}

	pstNode = *ppstCurNode;
	
	/* 进行数据插入 */
	if (NULL == pstNode)
	{
		*ppstCurNode = pstAvlInsertRecPara->pstNewNode;
		return;
	}

	iRet = pstAvlInsertRecPara->pfAvlInsCmpCB(pstAvlInsertRecPara->pstNewNode ,pstNode);
	if (iRet < 0)
	{
		avl_Insert(&(pstNode->pstLeft), pstAvlInsertRecPara);
		ulSonWay = SON_LEFT;
	}
	else
	{
		avl_Insert(&(pstNode->pstRight), pstAvlInsertRecPara);
		ulSonWay = SON_RIGHT;
	}
		
	AVL_Adjust(ulSonWay, &pstNode);
	
	*ppstCurNode = pstNode;
	
	return;
}

STATIC VOID avl_Delete(AVL_NODE_S **ppstCurNode, AVL_DELETE_REC_PARA_S *pstAvlDeleteRecPara)
{
	INT   iRet = 0;
	ULONG ulSonWay = 0;
	AVL_NODE_S* pstNode = NULL;
	
	if (NULL == ppstCurNode)
	{
		return;
	}

	pstNode = *ppstCurNode;

	if (NULL == pstNode)
	{
		return;
	}
	
	iRet = pstAvlDeleteRecPara->pfFindCmpCB(pstAvlDeleteRecPara->ulKeyIndex, pstAvlDeleteRecPara->pData, pstNode);

	/* 删除操作 */
	if (0 == iRet)
	{
		if (0 == AVL_RemoveNode(&pstNode, pstAvlDeleteRecPara->pfAvlFreeCB))
		{
			*ppstCurNode = pstNode;
			return;
		}
		
		avl_Delete(&(pstNode->pstLeft), pstAvlDeleteRecPara);
		ulSonWay = SON_RIGHT;
	}
	else if (iRet < 0)
	{
		/* 左边删除相当于右边插入 */
		avl_Delete(&(pstNode->pstLeft), pstAvlDeleteRecPara);
		ulSonWay = SON_RIGHT;
	}
	else
	{
		avl_Delete(&(pstNode->pstRight), pstAvlDeleteRecPara);
		/* 右边删除相当于左边插入 */
		ulSonWay = SON_LEFT;
	}
	
	AVL_Adjust(ulSonWay, &pstNode);

	*ppstCurNode = pstNode;
	
	return;
}

STATIC AVL_NODE_S *avl_Find(AVL_NODE_S *pstCurNode, AVL_FIND_REC_PARA_S *pstAvlFindRecPara)
{
	INT   iRet = 0;	

	if (NULL == pstCurNode)
	{
		return NULL;
	}
	
	iRet = pstAvlFindRecPara->pfFindCmpCB(pstAvlFindRecPara->ulKeyIndex, pstAvlFindRecPara->pKey, pstCurNode);
	if (0 == iRet)
	{
		return pstCurNode;
	}

	if (iRet < 0)
	{
		return avl_Find(pstCurNode->pstLeft, pstAvlFindRecPara);
	}
	else
	{
		return avl_Find(pstCurNode->pstRight, pstAvlFindRecPara);
	}
}

STATIC VOID avl_Destroy(AVL_NODE_S *pstCurNode, AVL_DELETE_REC_PARA_S *pstAvlDeleteRecPara)
{	
	if (NULL == pstCurNode)
	{
		return;
	}
	
	avl_Destroy(pstCurNode->pstLeft, pstAvlDeleteRecPara);
	avl_Destroy(pstCurNode->pstRight, pstAvlDeleteRecPara);
	pstAvlDeleteRecPara->pfAvlFreeCB(pstCurNode);
	pstCurNode = NULL;	
	
	return;
}

/* 初始化AVL比较函数，挂到管理链表 */
AVL_INFO_S * AVL_Init(AVL_INSERT_CMP_CALLBACK pfAvlInsertCmpCallBack,
 					  AVL_FIND_CMP_CALLBACK   pfAvlFindCmpCallBack,
 					  AVL_FREE_CALLBACK       pfAvlFreeCallBack,
 					  AVL_COPY_CALLBACK       pfAvlCopyCallBack)
{
	AVL_INFO_S *pstAvlInfo = NULL;

	pstAvlInfo = malloc(sizeof(AVL_INFO_S));
	if(NULL == pstAvlInfo)
	{
		return NULL;
	}
	
	memset(pstAvlInfo, 0, sizeof(AVL_INFO_S));
	
	pstAvlInfo->pfAvlInsertCmpCb = pfAvlInsertCmpCallBack;
	pstAvlInfo->pfAvlFindCmpCb 	 = pfAvlFindCmpCallBack;
	pstAvlInfo->pfAvlFreeCb   	 = pfAvlFreeCallBack;
	pstAvlInfo->pfAvlCopyCb   	 = pfAvlCopyCallBack;

	pstAvlInfo->lAvlID = AllocID(iAvlIdHandle, 0);
	if (pstAvlInfo->lAvlID < 0)
	{
		return NULL;
	}
	
	AVL_Sys_Add(pstAvlInfo);
	
	return pstAvlInfo;
}

VOID AVL_Fini(AVL_INFO_S *pstAvlInfo)
{
	DBGASSERT(NULL != pstAvlInfo);

	DestroyIDPool(iAvlIdHandle);
	AVL_Sys_Delete(pstAvlInfo);
	
	return;
}

VOID AVL_InsertNode(AVL_NODE_S *pstAvlNode, AVL_INFO_S *pstAvl)
{
	AVL_INSERT_REC_PARA_S stAvlInsertRecPara;

	mem_set0(&stAvlInsertRecPara, sizeof(AVL_INSERT_REC_PARA_S));
	
	stAvlInsertRecPara.pstNewNode		= pstAvlNode;
	stAvlInsertRecPara.pfAvlInsCmpCB	= pstAvl->pfAvlInsertCmpCb;

	/* 插入树 */
	avl_Insert(&(pstAvl->pstAvlHead), &stAvlInsertRecPara);

	return;
}

VOID AVL_DeleteNode(VOID *pKey, AVL_INFO_S *pstAvl)
{

	AVL_DELETE_REC_PARA_S stAvlDeleteRecPara;
	
	mem_set0(&stAvlDeleteRecPara, sizeof(AVL_DELETE_REC_PARA_S));

	stAvlDeleteRecPara.pData		= pKey;
	stAvlDeleteRecPara.pfFindCmpCB	= pstAvl->pfAvlFindCmpCb;
	stAvlDeleteRecPara.pfAvlFreeCB  = pstAvl->pfAvlFreeCb;
	//stAvlDeleteRecPara.ulKeyIndex   = /* 待升级 */
	
	avl_Delete(&(pstAvl->pstAvlHead), &stAvlDeleteRecPara);

	return;
}

AVL_NODE_S *AVL_FindNode(VOID *pKey, AVL_INFO_S *pstAvl)
{
	AVL_FIND_REC_PARA_S stAvlFindRecPara;
	
	mem_set0(&stAvlFindRecPara, sizeof(AVL_FIND_REC_PARA_S));

	stAvlFindRecPara.pKey		 = pKey;
	stAvlFindRecPara.pfFindCmpCB = pstAvl->pfAvlFindCmpCb;
	//stAvlFindRecPara.ulKeyIndex   = /* 待升级 */
	
	return avl_Find(pstAvl->pstAvlHead, &stAvlFindRecPara);
}

VOID AVL_DestroyAll(AVL_INFO_S *pstAvl)
{
	AVL_DELETE_REC_PARA_S stAvlDeleteRecPara;

	mem_set0(&stAvlDeleteRecPara, sizeof(AVL_DELETE_REC_PARA_S));

	stAvlDeleteRecPara.pfAvlFreeCB = pstAvl->pfAvlFreeCb;

	avl_Destroy(pstAvl->pstAvlHead, &stAvlDeleteRecPara);
	
	return;
}

/*************************************************************************
	AVL 通用接口 示例 & 测试 (更详细的测试旋转和删除，是通过下面的测试的)
	可以不使用回调只是用 AVL_Adjust AVL_RemoveNode 两个接口
	AVL TEST START
*************************************************************************/

typedef struct tag_testData
{
	AVL_NODE_S stAvlNode;
	ULONG ulData;
}AVL_TEST_DATA_S;

INT  INSERT_CMP_CallBack(AVL_NODE_S *pstNewNode, AVL_NODE_S *pstCurNode)
{
	AVL_TEST_DATA_S *pstNewData;
	AVL_TEST_DATA_S *pstCurData;

	pstNewData = container_of(pstNewNode, AVL_TEST_DATA_S, stAvlNode);
	pstCurData = container_of(pstCurNode, AVL_TEST_DATA_S, stAvlNode);

	return pstNewData->ulData - pstCurData->ulData;
}
INT  FIND_CMP_CallBack(ULONG ulKeyIndex, VOID *pKey, AVL_NODE_S *pstCurNode)
{
	AVL_TEST_DATA_S *pstCurData;

	pstCurData = container_of(pstCurNode, AVL_TEST_DATA_S, stAvlNode);

	return *((ULONG *)pKey) - pstCurData->ulData;
}

VOID FREE_CallBack(AVL_NODE_S *pstCurNode)
{
	AVL_TEST_DATA_S *pstCurData;

	pstCurData = container_of(pstCurNode, AVL_TEST_DATA_S, stAvlNode);

	free(pstCurData);

	return;
}

VOID COPY_CallBack(AVL_NODE_S *pstDestNode, AVL_NODE_S *pstSrcNode)
{
	

	return;
}

INT AVL_TEST(VOID)
{
	INT i = 0;
	ULONG ulKey = 0;
	AVL_NODE_S *pstNode = NULL;
	AVL_INFO_S *pstAVL = NULL;
	AVL_TEST_DATA_S *pstAvlTD = NULL;
	INT iRandZ = 0;
	struct timeval tpstart;

	mem_set0(&tpstart,sizeof(struct timeval));
	
	
	pstAVL = AVL_Init(INSERT_CMP_CallBack,
					   FIND_CMP_CallBack,
					   FREE_CallBack,
					   COPY_CallBack);

	pstAvlTD = malloc(sizeof(AVL_TEST_DATA_S));
	memset(pstAvlTD, 0, sizeof(AVL_TEST_DATA_S));
	pstAvlTD->ulData = 1990;
	AVL_InsertNode(&(pstAvlTD->stAvlNode), pstAVL);
	
	for (i = 0; i < 100000; )
	{
		gettimeofday(&tpstart,NULL);
		srand(tpstart.tv_usec);
		iRandZ = rand() % 1000000;
		if (1990 == iRandZ)
		{
			continue;
		}
		
		pstAvlTD = malloc(sizeof(AVL_TEST_DATA_S));
		memset(pstAvlTD, 0, sizeof(AVL_TEST_DATA_S));
		pstAvlTD->ulData = (ULONG)iRandZ;
		AVL_InsertNode(&(pstAvlTD->stAvlNode), pstAVL);
		i++;
	}

	MSG_PRINTF("TREE Height = %lu", pstAVL->pstAvlHead->ulHeight);
	
	pstAvlTD = NULL;
	ulKey = 1990;
	pstNode = AVL_FindNode((VOID *)(&ulKey), pstAVL);
	if (NULL != pstNode)
	{
		MSG_PRINTF("AVL TEST FIND SUCCESS!");
	}
	else
	{
		ERR_PRINTF("AVL TEST FIND FAILED!");
	}
	AVL_DeleteNode(&ulKey, pstAVL);
	pstNode = AVL_FindNode((VOID *)(&ulKey), pstAVL);
	if (NULL == pstAvlTD)
	{
		MSG_PRINTF("AVL TEST DELETE SUCCESS!");
	}
	else
	{
		ERR_PRINTF("AVL TEST DELETE FAILED!");
	}

	AVL_DestroyAll(pstAVL);
	AVL_Fini(pstAVL);
	
	return 0;
}
/*************************************************************************
	AVL TEST END
*************************************************************************/

/*************************************************************************
	AVL 自己构建 示例 & 测试 
	可以不使用回调只是用 AVL_Adjust AVL_RemoveNode 两个接口
	AVL SELF TEST START
*************************************************************************/

/* 回调不是必须的 */
typedef INT (*INSERT_DATACMP_CALLBACK)(AVL_NODE_S *, AVL_NODE_S*);
typedef INT (*DELETE_DATACMP_CALLBACK)(ULONG , AVL_NODE_S*);
typedef INT (*FIND_DATACMP_CALLBACK)(ULONG , AVL_NODE_S*);
typedef VOID (*FREE_CALLBACK)(AVL_NODE_S *);

/*  
	AVL插入示例  (AVL_Adjust)
	AVL树插入入口处，
	因为插入后会做旋转处理，因此传入的节点有可能被替换，
	因此采用二级指针(ppstCurNode)来更新子节点，让父节点，知道新的子节点
	@新的节点数据指针
	@指向root子树的指针的地址，
*/

VOID AVL_Insert(AVL_NODE_S *pstNewNode, 
				AVL_NODE_S**ppstCurNode, 
				INSERT_DATACMP_CALLBACK pfDataCmp)
{
	ULONG ulSonWay = 0;
	AVL_NODE_S *pstNode = NULL;

	pstNode = *ppstCurNode;
	
	/* 进行数据插入 */
	if (NULL == pstNode)
	{
		*ppstCurNode = pstNewNode;
		return;
	}

	if (pfDataCmp(pstNewNode ,pstNode) < 0)
	{
		AVL_Insert(pstNewNode, &(pstNode->pstLeft), pfDataCmp);
		ulSonWay = SON_LEFT;
	}
	else
	{
		AVL_Insert(pstNewNode, &(pstNode->pstRight), pfDataCmp);
		ulSonWay = SON_RIGHT;
	}
		
	AVL_Adjust(ulSonWay, &pstNode);
	
	*ppstCurNode = pstNode;
	
	return;
}

/* 
	AVL删除示例 (AVL_RemoveNode, AVL_Adjust)
*/
VOID AVL_Delete(AVL_NODE_S **ppstCurNode, 
				ULONG ulDelData, 
				DELETE_DATACMP_CALLBACK pfDataCmp)
{
	ULONG ulSonWay = 0;
	AVL_NODE_S* pstNode = NULL;

	pstNode = *ppstCurNode;

	if (NULL == pstNode)
	{
		return;
	}

	/* 删除操作 */
	if (0 == pfDataCmp(ulDelData, pstNode))
	{
		if (0 == AVL_RemoveNode(&pstNode, NULL))
		{
			*ppstCurNode = pstNode;
			return;
		}
		
		AVL_Delete(&(pstNode->pstLeft), ulDelData, pfDataCmp);
		ulSonWay = SON_RIGHT;
	}
	else if (pfDataCmp(ulDelData, pstNode) < 0)
	{
		/* 左边删除相当于右边插入 */
		AVL_Delete(&(pstNode->pstLeft), ulDelData, pfDataCmp);
		ulSonWay = SON_RIGHT;
	}
	else
	{
		AVL_Delete(&(pstNode->pstRight), ulDelData, pfDataCmp);
		/* 右边删除相当于左边插入 */
		ulSonWay = SON_LEFT;
	}
	
	AVL_Adjust(ulSonWay, &pstNode);

	*ppstCurNode = pstNode;
	
	return;
}

VOID AVL_LCR_Traversal(AVL_NODE_S* pstCurNode)
{
	if (NULL == pstCurNode)
	{
		return;
	}
	
	AVL_LCR_Traversal(pstCurNode->pstLeft);
	AVL_LCR_Traversal(pstCurNode->pstRight);

	return;
}
VOID AVL_RCL_Traversal(AVL_NODE_S* pstCurNode)
{
	if (NULL == pstCurNode)
	{
		return;
	}
	
	AVL_RCL_Traversal(pstCurNode->pstRight);
	//printf("[Value] = [%lu]\n", pstCurNode->ulData);
	AVL_RCL_Traversal(pstCurNode->pstLeft);

	return;
}

AVL_NODE_S *AVL_Find(AVL_NODE_S* pstCurNode, 
 						 ULONG ulFindData,
 						 FIND_DATACMP_CALLBACK pfDataCmp)
{
	if (NULL == pstCurNode)
	{
		return NULL;
	}

	if (0 == pfDataCmp(ulFindData, pstCurNode))
	{
		return pstCurNode;
	}

	if (pfDataCmp(ulFindData, pstCurNode) < 0)
	{
		return AVL_Find(pstCurNode->pstLeft, ulFindData, pfDataCmp);
	}
	else
	{
		return AVL_Find(pstCurNode->pstRight, ulFindData, pfDataCmp);
	}
	
}

VOID AVL_LRC_TraversalFree(AVL_NODE_S **ppstCurNode, FREE_CALLBACK pfFree)
{
	AVL_NODE_S* pstNode = NULL;

	pstNode = *ppstCurNode;
	
	if (NULL == pstNode)
	{
		*ppstCurNode = NULL;
		return;
	}
	
	AVL_LRC_TraversalFree(&(pstNode->pstLeft), pfFree);
	AVL_LRC_TraversalFree(&(pstNode->pstRight), pfFree);
	pfFree(pstNode);
	pstNode = NULL;
	*ppstCurNode = pstNode;
	
	return;
}

typedef struct tag_mydata
{
	AVL_NODE_S stNode;
	ULONG ulData;
}MY_DATA_S;

STATIC AVL_NODE_S *g_pstAVLHead = NULL;

INT insert_DataCmp(AVL_NODE_S *pstNewNode, AVL_NODE_S *pstCurNode)
{
	MY_DATA_S *pstMyNewdata = NULL;
	MY_DATA_S *pstMyCurdata = NULL;

	pstMyNewdata = container_of(pstNewNode,MY_DATA_S,stNode);
	pstMyCurdata = container_of(pstCurNode,MY_DATA_S,stNode);
	return pstMyNewdata->ulData - pstMyCurdata->ulData;
}

INT find_DataCmp(ULONG ulData, AVL_NODE_S *pstCurNode)
{
	MY_DATA_S *pstMyCurdata = NULL;

	pstMyCurdata = container_of(pstCurNode,MY_DATA_S,stNode);
	return ulData - pstMyCurdata->ulData;
}

INT delete_DataCmp(ULONG ulData, AVL_NODE_S *pstCurNode)
{
	MY_DATA_S *pstMyCurdata = NULL;

	pstMyCurdata = container_of(pstCurNode,MY_DATA_S,stNode);
	return ulData - pstMyCurdata->ulData;
}

VOID free_Data(AVL_NODE_S *pstNode)
{
	MY_DATA_S* pstData = NULL;

	pstData = container_of(pstNode,MY_DATA_S,stNode);
	free(pstData);
	
	return;
}

ULONG* create_data_table(ULONG ulLen)
{
	INT i = 0;
	ULONG *pulData = NULL;
	pulData = malloc(sizeof(ULONG) * ulLen);
	for (i = 0; i < ulLen; i++)
	{
		pulData[i] = i;
	}

	return pulData;
}

VOID destroy_data_table(ULONG *pulData)
{
	free(pulData);
	return;
}

ULONG get_rand_data_ftable(ULONG* pulData, ULONG ulLen)
{
	INT loopI = 0;
	INT iRandZ = 0;
	ULONG ulData = 0;
	struct timeval tpstart;
	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);
	iRandZ = rand() % ulLen;

	while(1)
	{
		if (pulData[iRandZ] != 0)
		{
			ulData = pulData[iRandZ];
			pulData[iRandZ] = 0;
			break;
		}
		iRandZ++;
		loopI++;
		if (loopI >= ulLen)
		{
			ulData = ulLen + 1;
			break;
		}
		
		if (iRandZ >= ulLen)
		{
			iRandZ = 0;
		}
	}
	
	return ulData;
}

AVL_NODE_S *AVL_FindNode_test(AVL_NODE_S* pstCurNode, 
 						 ULONG ulFindData,
 						 FIND_DATACMP_CALLBACK pfDataCmp,
 						 INT *time)
{
	(*time)++;
	if (NULL == pstCurNode)
	{
		return NULL;
	}

	if (0 == pfDataCmp(ulFindData, pstCurNode))
	{
		return pstCurNode;
	}

	if (pfDataCmp(ulFindData, pstCurNode) < 0)
	{
		return AVL_FindNode_test(pstCurNode->pstLeft, ulFindData, pfDataCmp, time);
	}
	else
	{
		return AVL_FindNode_test(pstCurNode->pstRight, ulFindData, pfDataCmp, time);
	}

}

/* 测试该树的每个节点是否失去平衡 */
INT avl_balance_test(AVL_NODE_S* pstCurNode)
{
	INT iRet = 0;
	INT iBalance = 0;

	if (NULL == pstCurNode)
	{
		return 0;
	}
	
	iRet = avl_balance_test(pstCurNode->pstLeft);
	
	iBalance = GetTreeBalance(pstCurNode);
	if ((iBalance <= -2) || (iBalance >= 2))
	{
		ERR_PRINTF("!!!!!!Tree is lost balance\n");
		iRet |= 1;
	}
	else
	{
		iRet |= 0;
	}
	iRet |= avl_balance_test(pstCurNode->pstRight);

	return iRet;
}

/* 旋转测试，测试旋转后高度是否合理，是否平衡 */
INT avl_rotate_test(INT cnt)
{
	INT i = 0;
	INT time = 0;
	ULONG data;
	ULONG *pulData = NULL;
	MY_DATA_S* pstData = NULL;

	/* 1 ~ cnt 随机插入到AVL中 */
	pulData = create_data_table(cnt);
	i = 0;
	while(1)
	{
		data = get_rand_data_ftable(pulData, cnt);
		
		pstData = malloc(sizeof(MY_DATA_S));
		memset(pstData, 0, sizeof(AVL_NODE_S));
		pstData->ulData = data;
		AVL_Insert(&(pstData->stNode), &g_pstAVLHead, insert_DataCmp);
		i++;
		if (data > cnt)
		{
			break;
		}
	}

	/* 是否失去平衡 */
	if (avl_balance_test(g_pstAVLHead))
	{
		ERR_PRINTF("!!!!!!Tree is lost balance\n");
	}

	/* 检查查找速度 */
	AVL_FindNode_test(g_pstAVLHead, cnt, find_DataCmp, &time);

	MSG_PRINTF("TreeHeight = [%lu]", g_pstAVLHead->ulHeight);
	MSG_PRINTF("Time = [%d]", time);

	/* 释放数据 */
	AVL_LRC_TraversalFree(&g_pstAVLHead, free_Data);
	destroy_data_table(pulData);

	return 0;
}

/* 插入测试 */
INT avl_insert_test(INT cnt)
{
	INT i = 0;
	INT iRet = 0;
	ULONG data;
	ULONG *pulData = NULL;
	MY_DATA_S* pstData = NULL;
	AVL_NODE_S *pstNode = NULL;
	
	
	pulData = create_data_table(cnt);
	i = 0;
	while(1)
	{
		data = get_rand_data_ftable(pulData, cnt);
		
		pstData = malloc(sizeof(MY_DATA_S));
		memset(pstData, 0, sizeof(AVL_NODE_S));
		pstData->ulData = data;
		AVL_Insert(&(pstData->stNode), &g_pstAVLHead, insert_DataCmp);
		i++;
		if (data > cnt)
		{
			break;
		}
	}
	
	for(i = 1; i < cnt; i++)
	{
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		if (pstNode == NULL)
		{
			ERR_PRINTF("Find CHECK Failed!\n");
			break;
		}
	}

	if (i != cnt)
	{
		ERR_PRINTF("i = %d ; cnt = %d\n", i ,cnt);
		ERR_PRINTF("Insert Find CHECK Failed!\n");
		iRet = -1;
	}

	AVL_LRC_TraversalFree(&g_pstAVLHead, free_Data);
	destroy_data_table(pulData);

	return iRet;
}

/* 删除一个节点测试 */
INT avl_delete_test(INT cnt)
{
	INT i = 0;
	INT iRet = 0;
	ULONG data;
	ULONG *pulData = NULL;
	MY_DATA_S* pstData = NULL;
	AVL_NODE_S *pstNode = NULL;
	
	pulData = create_data_table(cnt);
	i = 0;
	while(1)
	{
		data = get_rand_data_ftable(pulData, cnt);
		
		pstData = malloc(sizeof(MY_DATA_S));
		memset(pstData, 0, sizeof(MY_DATA_S));
		pstData->ulData = data;
		AVL_Insert(&(pstData->stNode), &g_pstAVLHead, insert_DataCmp);
		i++;
		if (data > cnt)
		{
			break;
		}
	}
	
	for(i = 1; i < cnt; i++)
	{
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		if (pstNode == NULL)
		{
			ERR_PRINTF("Before Delete Insert CHECK Failed!\n");
			break;
		}
		
		AVL_Delete(&g_pstAVLHead, i, delete_DataCmp);
		free(pstNode);
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		if (pstNode != NULL)
		{
			ERR_PRINTF("Delete  Failed!\n");
			break;
		}
		
		if(avl_balance_test(g_pstAVLHead)){ERR_PRINTF("LOST Balance Delete\n");};
		
		pstData = malloc(sizeof(MY_DATA_S));
		memset(pstData, 0, sizeof(MY_DATA_S));
		pstData->ulData = i;
		AVL_Insert(&(pstData->stNode), &g_pstAVLHead, insert_DataCmp);
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		if (pstNode == NULL)
		{
			ERR_PRINTF("Delete Insert CHECK Failed!\n");
			break;
		}

		if(avl_balance_test(g_pstAVLHead)){ERR_PRINTF("LOST Balance Delete Insert\n");};
	}
	if (i != cnt)
	{
		ERR_PRINTF("i = %d ; cnt = %d\n", i ,cnt);
		ERR_PRINTF("Delete Insert CHECK Failed!\n");
		iRet = -1;
	}

	AVL_LRC_TraversalFree(&g_pstAVLHead, free_Data);
	destroy_data_table(pulData);

	return iRet;
}

/* 删除多个节点测试 */
INT avl_delete_lots_test(INT cnt)
{
	INT i = 0;
	INT iRet = 0;
	ULONG data;
	ULONG *pulData = NULL;
	AVL_NODE_S* pstNode = NULL;
	MY_DATA_S* pstData = NULL;
	INT iRandStatr = 0;
	INT iRandDeletLen = 0;

	
	struct timeval tpstart;
	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);
	iRandStatr = rand() % cnt;
	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);
	iRandDeletLen = rand() % cnt;
	if ((iRandStatr + iRandDeletLen) > cnt)
	{
		iRandDeletLen = cnt - iRandStatr;
	}
	
	pulData = create_data_table(cnt);
	i = 0;
	while(1)
	{
		data = get_rand_data_ftable(pulData, cnt);
		
		pstData = malloc(sizeof(MY_DATA_S));
		memset(pstData, 0, sizeof(MY_DATA_S));
		pstData->ulData = data;
		AVL_Insert(&(pstData->stNode), &g_pstAVLHead, insert_DataCmp);
		i++;
		if (data > cnt)
		{
			break;
		}
	}

	/* 在随机位置，删除随机个数节点 */
	for (i = iRandStatr; i < (iRandStatr + iRandDeletLen); i++)
	{
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		AVL_Delete(&g_pstAVLHead, i, delete_DataCmp);
		free(pstNode);
		if(avl_balance_test(g_pstAVLHead))
		{
			ERR_PRINTF("LOST Balance Delete lots[%d-%d][at %d]\n",iRandStatr, (iRandStatr+iRandDeletLen), i);
			break;
		};
	}
	
	if(avl_balance_test(g_pstAVLHead)){ERR_PRINTF("LOST Balance Delete lots\n");};

	for (i = 1; i < cnt; i++)
	{
		pstNode = AVL_Find(g_pstAVLHead, i, find_DataCmp);
		if ((i >=  iRandStatr) && (i < (iRandStatr + iRandDeletLen)))
		{
			if (pstNode != NULL)
			{
				ERR_PRINTF("[Delete find = %d]\n", i);
				ERR_PRINTF("Delete lots	Failed!\n");
				break;
			}
		}
		else
		{
			if (pstNode == NULL)
			{
				ERR_PRINTF("Delets lots wrong!\n");
				break;
			}
		}
	}
	
	if (i != cnt)
	{
		ERR_PRINTF("i = %d ; cnt = %d\n", i ,cnt);
		ERR_PRINTF("Delete LOST CHECK Failed!\n");
		iRet = -1;
	}
	
	AVL_LRC_TraversalFree(&g_pstAVLHead, free_Data);
	destroy_data_table(pulData);

	return iRet;
}

INT avl_test(INT Datacnt, INT testCnt)
{
	INT i = 0;
	INT iRet = 0;

	avl_rotate_test(Datacnt);
	
	for(i = 0; i < testCnt; i++)
	{
		iRet=  avl_insert_test(Datacnt);
		if (iRet < 0)
		{
			break;
		}
	}
	
	if (i == testCnt)
	{
		MSG_PRINTF("Insert TEST SUCCESS !");
	}
	else
	{
		ERR_PRINTF("Insert TEST FAILED !");
	}
	
	for(i = 0; i < testCnt; i++)
	{
		iRet=  avl_delete_test(Datacnt);
		if (iRet < 0)
		{
			break;
		}
	}
	
	if (i == testCnt)
	{
		MSG_PRINTF("Delete TEST SUCCESS !");
	}
	else
	{
		ERR_PRINTF("Delete TEST FAILED !");
	}
	
	for(i = 0; i < testCnt; i++)
	{
		iRet=  avl_delete_lots_test(Datacnt);
		if (iRet < 0)
		{
			break;
		}
	}
	
	if (i == testCnt)
	{
		MSG_PRINTF("Delete LOST TEST SUCCESS !");
	}
	else
	{
		ERR_PRINTF("Delete LOST FAILED !");
	}
		
	return iRet;
}

/* 测试总入口 */
INT AVL_test_Main(VOID/* INT argc, char **argv */)
{
	INT cnt = 1000, testCnt = 100;
/*
	if (argc == 1)
	{
		cnt = 100;
	}
	else
	{
		cnt = atoi(argv[1]);
	}
	if (argc == 2)
	{
		testCnt = 2;
	}
	else
	{
		testCnt = atoi(argv[2]);
	}
*/
	MSG_PRINTF("cnt = %d, testCnet = %d", cnt, testCnt);
	avl_test(cnt,testCnt);
	MSG_PRINTF("TEST SUCCESS !");

	AVL_TEST();//奶奶的两个一块测就有问题妈蛋
	
	return 0;
}
/*************************************************************************
	AVL SELF TEST END
*************************************************************************/


