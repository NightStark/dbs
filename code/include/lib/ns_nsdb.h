/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : nsdb.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/3
  Last Modified :
  Description   : NSDB.c的头文件
  Function List :
  History       :
  1.Date        : 2013/11/3
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#ifndef __NSDB_H__
#define __NSDB_H__

#define NSDB_TRANS_DATA_BUF_MAX (1024 * 1024 * 4) /* 4MB */

ULONG NSDB_CreateTable(IN const CHAR *szTableName, TABLE_ELE_S *pstTableEle);
ULONG NSDB_DeleteTable(CHAR *tableName);
ULONG NSDB_FindData(IN  CHAR *TableName, 
					IN  CHAR * szTypeName,
					IN  VOID * ulFindData,
					OUT GET_DATA_ELE_S *pstGetDataEle );
ULONG NSDB_DelData(IN CHAR *TableName, 
 				   IN CHAR *szTypeName,
 				   IN VOID  *ulFindData);
ULONG NSDB_AddData(CHAR *TableName, VOID *pAddEle, UINT uiEleNum);
ULONG NSDB_UpdateData(IN CHAR *TableName, 
					  IN VOID *pAddEle, 
					  IN UINT  uiEleNum,
					  IN CHAR *szTypeName,
					  IN VOID *ulFindData);

#endif //__NSDB_H__



