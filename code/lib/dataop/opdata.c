/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : opdata.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/7
  Last Modified :
  Description   : 	通过结构体的数据类型的ID ,来获取到相应的链表，
  Function List :

  History       :
  1.Date        : 2013/11/7
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#ifndef __DATAOP_C__
#define __DATAOP_C__

/*
	通过结构体的数据类型的ID ,来获取到相应的链表，
	为每种数据创建一个链表。

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_lilist.h>
#include <ns_table_type.h>
#include <ns_table.h>
#include <ns_opdata.h>

STATIC OPDATA_DATA_S * opdata_data_AllocMem(ULONG ulDataSize)
{
	OPDATA_DATA_S *pstDataTmp = NULL;

	/* 数据节点 */
	pstDataTmp = mem_alloc(sizeof(OPDATA_DATA_S));
	if(NULL == pstDataTmp)
	{
		ERR_PRINTF("alloc mem for data struct failed!");
	}
	
	/* 数据内存 */
	pstDataTmp->pData = mem_alloc(ulDataSize);
	if(NULL == pstDataTmp->pData)
	{
		ERR_PRINTF("alloc mem for data failed!");
	}

	return pstDataTmp;
}

ULONG opdata_data_AddNode(IN TABLE_S *pstTable, OPDATA_DATA_S *pstData)
{
	DBGASSERT(NULL != pstTable);
	DBGASSERT(NULL != pstData);
	
	DCL_AddHead(&(pstTable->stTableHead),&pstData->stNode);
	
	return ERROR_SUCCESS;
}

/* Create data node */
ULLONG Opdata_data_Create(IN TABLE_S *pstTable, ULONG  *pdata, ULONG ulDataSize)
{
	ULONG ulRet = ERROR_FAILE;
	OPDATA_DATA_S *pstData = NULL;

	DBGASSERT(NULL != pdata);

	/* 申请内存 */
	pstData = opdata_data_AllocMem(ulDataSize);
	if(NULL == pstData)
	{
		return ERROR_FAILE;
	}

	/* 数据装载 */
	memcpy(pstData->pData,pdata,ulDataSize);

	/* 添加到链表 */
	ulRet = opdata_data_AddNode(pstTable, pstData);
	
	pstTable->ulTableLen++;

	return ulRet;
}

/* 获取数据个数 */
ULONG Opdata_data_GetDataCount(VOID)
{
	return 0;
}

/* 根据类型和类型名 在指定的链表里查找 指定的数据 */
STATIC OPDATA_DATA_S *opdata_data_FindNode(IN TABLE_S *pstTable,
 										   IN CHAR *szTypeName,										
 										   IN VOID  *ulFindData)
{
	VOID *pdata     = NULL;
	ULONG ulRet 	= ERROR_FAILE;
	ULONG uiTOffset = 0;
	ULONG ulTypeLen	= 0;
	ULONG ulTIndex	= 0;
	ANY_TYPE_EN    atTType;
	DCL_HEAD_S    *pstListHead = NULL;
	TABLE_TYPES_S *pstType     = NULL;
	OPDATA_DATA_S *pstData     = NULL;

	
	pstType     = pstTable->pstTableTypes;
	pstListHead = &(pstTable->stTableHead);

	//获取索引，即要查询的数据的偏移，	
	//ERR_PRINTF("Type name = %s", szTypeName);  
	ulTIndex = Table_type_GetTypeIndex(pstType, (const CHAR *)szTypeName);
	if(ERROR_INVALID_TYPE_NAME == ulTIndex)
	{
		ERR_PRINTF("Type [%s] is not exist!", szTypeName);
		return NULL;
	}

	uiTOffset = pstType->pucTypeOffset[ulTIndex];
	atTType	  = pstType->pucTypeMap[ulTIndex];
	ulTypeLen = pstType->pulTypeLen[ulTIndex];

	DCL_FOREACH_ENTRY(pstListHead,pstData,stNode)
	{
		//pstData = container_of(pstNode, OPDATA_DATA_S, stNode);

		//获取要比较数据在链表节点上的位置
		pdata = ((CHAR *)(ULONG *)(pstData->pData) + uiTOffset);
		//数据比较
		if (AT_SZ == atTType)
		{
			ulRet = str_safe_cmp(pdata, ulFindData, ulTypeLen);			
		}
		else
		{
			ulRet = mem_safe_cmp(pdata, ulFindData, ulTypeLen);			
		}
		
		if(0 == ulRet)
		{
			break;
		}
	}

	return pstData;
}


/* 数据查找 */
ULONG Opdata_data_FindData(IN TABLE_S  *pstTable,
 						   IN CHAR *szTypeName,
 						   IN VOID  *ulFindData, /* 要比较的数据 */
 						   OUT VOID *pData)
{
	OPDATA_DATA_S *pstData;

	pstData = opdata_data_FindNode(pstTable, szTypeName, ulFindData);
	if(NULL == pstData)
	{
		return ERROR_DATA_NOT_EXIST;
	}
 
	/* 待升级--直接考到目标内存数组，用回调 */
	memcpy(pData, pstData->pData, pstTable->pstTableTypes->ulSTLong);

	return ERROR_SUCCESS;

}


ULONG Opdata_data_getAllData(TABLE_S *pstTable, ULONG *pulData)
{
	OPDATA_DATA_S *pstData   = NULL;
	DCL_HEAD_S *pstListHead  = NULL;

	UCHAR * pcData = (UCHAR *)pulData;
	
	ULONG  ulTypeLen			= 0;
		
	pstListHead = &(pstTable->stTableHead);
	
	ulTypeLen = pstTable->pstTableTypes->ulSTLong;
	
	DCL_FOREACH_ENTRY(pstListHead, pstData, stNode)
	{
		//pstData = container_of(pstNode, OPDATA_DATA_S, stNode);

		memcpy(pcData, pstData->pData, ulTypeLen);
		pcData += ulTypeLen;
	}
	
	return ERROR_SUCCESS;
}

#if 0
ULONG Opdata_data_GetSelectDataCount(ULONG ulTableFd, 
											  NSDB_SELECT_COND_S  *pstSltCond)
{
	ULONG  ulGetData = 0;
	ULONG  ulRet     = ERROR_SUCCESS;
	ULONG  ulSelectDataCounter = 0;
	
	TABLE_S 	  *pstTable    = NULL;
	TABLE_TYPES_S *pstType 	   = NULL;
	LIST_NODE_S   *pstNode	   = NULL;
	OPDATA_DATA_S *pstData	   = NULL;
	LIST_HEAD_S   *pstListHead = NULL;
	LIST_TAIL_S   *pstListTail = NULL;
		
	pstTable = LIST_Table_GetTable(ulTableFd);
	pstType = pstTable->pstTableTypes;	
	
	pstListHead = Table_GetTableHead(ulTableFd);
	pstListTail = LIST_Table_GetTableTail(ulTableFd);
	
	FOREACH(pstListHead, pstNode, pstListTail)
	{
		pstData = container_of(pstNode, OPDATA_DATA_S, stNode);
		ulRet = Table_type_DataGet(pstType,
									pstSltCond->szEleName,
									pstData->pData,
									&ulGetData);
		if ( ERROR_SUCCESS != ulRet)
		{
		    return 0;
		}

		if(ulGetData >= pstSltCond->ulEleValueMin &&
		   ulGetData <= pstSltCond->ulEleValueMax)
		{
			ulSelectDataCounter++;
		}

	}
	return ulSelectDataCounter;
}


ULONG Opdata_data_GetSelectData(ULONG ulTableFd, 
									   IN  NSDB_SELECT_COND_S  *pstSltCond,
									   OUT ULONG *pulData)
{
	ULONG  ulRet     		   = ERROR_SUCCESS;
	ULONG  ulGetData 		   = 0;
	ULONG  ulTypeLen		   = 0;
	ULONG  ulSelectDataCounter = 0;
	
	TABLE_S 	  *pstTable    = NULL;
	TABLE_TYPES_S *pstType 	   = NULL;
	LIST_NODE_S   *pstNode	   = NULL;
	OPDATA_DATA_S *pstData	   = NULL;
	LIST_HEAD_S   *pstListHead = NULL;
	LIST_TAIL_S   *pstListTail = NULL;
		
	UCHAR * pcData = (UCHAR *)pulData;
	pstTable = LIST_Table_GetTable(ulTableFd);
	pstType = pstTable->pstTableTypes;
	ulTypeLen = pstTable->pstTableTypes->ulSTLong;
 	
	pstListHead = Table_GetTableHead(ulTableFd);
	pstListTail = LIST_Table_GetTableTail(ulTableFd);
	
	FOREACH(pstListHead, pstNode, pstListTail)
	{
		pstData = container_of(pstNode, OPDATA_DATA_S, stNode);
		ulRet = Table_type_DataGet(pstType,
									pstSltCond->szEleName,
									pstData->pData,
									&ulGetData);
		if ( ERROR_SUCCESS != ulRet)
		{
		    return ERROR_FAILE;
		}

		if(ulGetData >= pstSltCond->ulEleValueMin &&
		   ulGetData <= pstSltCond->ulEleValueMax)
		{
			memcpy(pcData, pstData->pData, ulTypeLen);
			pcData += ulTypeLen;
		}

	}
	return ERROR_SUCCESS;
}
#endif

ULONG Opdata_data_Init(ULONG ulTableFd)
{
/*
	LIST_HEAD_S *pstListHead = NULL;
	LIST_TAIL_S *pstListTail = NULL;
	
	pstListHead = Table_GetTableHead(ulTableFd);
	pstListTail = LIST_Table_GetTableTail(ulTableFd);

	LIST_Init(pstListHead,pstListTail);
*/	
	return ERROR_SUCCESS;
}

VOID opdata_data_FreeNode(VOID *pNode)
{
	OPDATA_DATA_S *pstData = NULL;

	pstData = DCL_ENTRY((DCL_NODE_S *)pNode, OPDATA_DATA_S, stNode);

	free(pstData->pData);

	free(pstData);

	return;
}

ULONG Opdata_data_DelNode(IN TABLE_S *pstTable, OPDATA_DATA_S *pstData)
{
	DCL_HEAD_S  *pstListHead = NULL;
	
	DBGASSERT(NULL != pstData);
	
	pstListHead = &(pstTable->stTableHead);

	if(DCL_IS_EMPTY(pstListHead))
	{
		return  ERROR_LIST_IS_EMPTY;
	}

	DCL_Del(&pstData->stNode);
	opdata_data_FreeNode(pstData);
	
	pstTable->ulTableLen--;
	return ERROR_SUCCESS;
}

ULONG Opdata_data_DelData(IN TABLE_S *pstTable,
 						  IN CHAR *szTypeName,
 						  IN VOID  *ulFindData)
{
	ULONG ulRet = ERROR_FAILE;
	OPDATA_DATA_S *pstData;

	pstData = opdata_data_FindNode(pstTable, szTypeName, ulFindData);
	if(NULL == pstData)
	{
		return ERROR_DATA_NOT_EXIST;
	}
	ulRet = Opdata_data_DelNode(pstTable, pstData);
	
	return ulRet;
}

ULONG opdata_data_UpdateNode(OPDATA_DATA_S * pstData)
{
	return ERROR_SUCCESS;
}
/********************************************************************************
	IN TABLE_TYPES_S * pstType,		要操作的数据类型
	IN UCHAR *szTypeName,		查找的类型，用于查找到要修改的节点数据
	IN VOID  *ulFindData		查找的数据，用于查找到要修改的节点数据
	IN UCHAR **szTypeNameList,	要修改的类型名列表
	IN VOID  *ulNewDataList,		要修该的新数据的列表
********************************************************************************/
ULONG Opdata_data_UpdateData(IN TABLE_S *pstTable,
							 IN CHAR    *szFindTypeName,
							 IN VOID    *ulFindData,
							 IN VOID    *pNewData)
{
	OPDATA_DATA_S *pstData  = NULL;
	TABLE_TYPES_S *pstType  = NULL;

	pstType = pstTable->pstTableTypes;

	pstData = opdata_data_FindNode(pstTable, szFindTypeName, ulFindData);
	if(NULL == pstData)
	{
		return ERROR_DATA_NOT_EXIST;
	}

	/* 数据装载 */
	memcpy(pstData->pData,pNewData,pstType->ulSTLong);
	
	return ERROR_SUCCESS;
}

VOID Opdata_data_DelAll(TABLE_S *pstTable)
{
	DCL_HEAD_S *pstListHead = NULL;
				
	pstListHead = &(pstTable->stTableHead);
	
	while(!DCL_IS_EMPTY(pstListHead))
	{
		DCL_DelFirst(pstListHead, opdata_data_FreeNode);	
	}
	
	return;
}


VOID Opdata_data_Fini(TABLE_S *pstTable)
{
	Opdata_data_DelAll(pstTable);
	Table_type_Destroy(pstTable->pstTableTypes);//暂时现在这里调用

	return;
}

#endif  //__DATAOP_C__
