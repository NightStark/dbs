#ifndef __TABLE_C__
#define __TABLE_C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <ns_base.h>

#include <ns_bitmap.h>
#include <ns_id.h>
#include <ns_lilist.h>
#include <ns_table.h>

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
typedef struct st_database_tag
{
	LONG    lTableIDPoolFd;
	ULONG	ulTableConut;
	TABLE_S **ppstTable;
	DCL_HEAD_S g_stDataBaseHead;
}DATABASE_S;

DATABASE_S g_st_database = {};


//STATIC DCL_HEAD_S g_stDataBaseHead = {};


/* 把数据结构体类型注册到全局变量
	返回注册的下标
*/
//free 和 DATABASE_S malloc //改造

/*****************************************************************************
 Prototype    : DataBase_Init
 Description  : Database initialize
 Input        : VOID  
 Output       : None
 Return Value : ULONG 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2013/12/26
    Author       : Night Stark [lyj051031448@163.com]
    Modification : Created function

*****************************************************************************/
ULONG DataBase_Init(VOID)
{
	DCL_Init(&g_st_database.g_stDataBaseHead);

	g_st_database.lTableIDPoolFd = CreateIDPool(TABLE_NUM_MAX);
	if (BIT_MAP_INVALID_ID == g_st_database.lTableIDPoolFd)
	{
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : DataBase_Fini
 Description  : 释放数据库表挂接的内存，释放前要保证所有的表已经被释放
 Input        : VOID  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2013/12/26
    Author       : Night Stark [lyj051031448@163.com]
    Modification : Created function

*****************************************************************************/
ULONG DataBase_Fini(VOID)
{
	DCL_Fint(&g_st_database.g_stDataBaseHead);
	
	DestroyIDPool(g_st_database.lTableIDPoolFd);

	return ERROR_SUCCESS;
}

TABLE_S * Table_GetTableByTableName(IN const CHAR *szTableName)
{
	ULONG ulRet = 0;

	TABLE_S *pstTable = NULL;

	DCL_FOREACH_ENTRY(&(g_st_database.g_stDataBaseHead) ,pstTable,stNode)
	{
		ulRet = str_safe_cmp(pstTable->szTableName,
	 						 szTableName,
	 						 sizeof(pstTable->szTableName));

	 	if (0 == ulRet)
	 	{
			break;
	 	}
	}

	if (NULL == pstTable)
	{
		//ERR_PRINTF("Can't find table!");
	}

	return pstTable;
}

TABLE_S * Table_GetTableByTableID(IN UINT uiTableID)
{
	TABLE_S *pstTable = NULL;

	DCL_FOREACH_ENTRY(&(g_st_database.g_stDataBaseHead) ,pstTable,stNode)
	{
	 	if (uiTableID == pstTable->uiTableID)
	 	{
			break;
	 	}
	}

	return pstTable;
}


ULONG Table_GetTableIDByTableName(IN const CHAR *szTableName)
{
	TABLE_S *pstTable = NULL;

	pstTable = Table_GetTableByTableName(szTableName);
	if (NULL == pstTable)
	{
		return TABLE_FD_INVALID;
	}

	return pstTable->uiTableID;
}

TABLE_S * Table_CreateTable(IN const CHAR *szTableName,
							IN TABLE_TYPES_S *pstTablesType)
{
	TABLE_S *pstTable = NULL;

	DBGASSERT(NULL != szTableName);
	DBGASSERT(NULL != pstTablesType);

	pstTable = (TABLE_S *)mem_alloc(sizeof(TABLE_S));
	if (NULL == pstTable)
	{
		return NULL;
	}

	pstTable->uiTableID = AllocID(g_st_database.lTableIDPoolFd, 1);
	if (-1 == pstTable->uiTableID)
	{
		return NULL;
	}
	
	strcpy(pstTable->szTableName, szTableName);
	pstTable->pstTableTypes = pstTablesType;
	DCL_Init(&(pstTable->stTableHead));

	DCL_AddTail(&(g_st_database.g_stDataBaseHead), &(pstTable->stNode));
	
	g_st_database.ulTableConut++;

	return pstTable;
}

VOID table_nodefree(TABLE_S *pstTable)
{
	//释放 Type数据
	//释放所有数据
	
	free(pstTable);

	return;
}

VOID Table_DestroyTable(IN const CHAR *szTableName)
{
	TABLE_S *pstTable = NULL;

	DBGASSERT(NULL != szTableName);

	pstTable = Table_GetTableByTableName(szTableName);
	if (NULL == szTableName)
	{
		return;
	}
	
	DCL_Del(&(pstTable->stNode));
	
	table_nodefree(pstTable);

	return;
}

ULONG Table_GetTableConut(IN UINT uiTableID)
{	
	return g_st_database.ulTableConut;
}

ULONG Table_GetTableLen(IN UINT uiTableID)
{
	TABLE_S *pstTable = NULL;

	pstTable = Table_GetTableByTableID(uiTableID);
	if (NULL == pstTable)
	{
		return TABLE_FD_INVALID;
	}

	return pstTable->ulTableLen;
}

DCL_HEAD_S * Table_GetTableHead(IN const CHAR *szTableName)
{
	TABLE_S *pstTable = NULL;

	pstTable = Table_GetTableByTableName(szTableName);
	if (NULL == pstTable)
	{
		return NULL;
	}

	return &pstTable->stTableHead;
}

#endif //__TABLE_C__


