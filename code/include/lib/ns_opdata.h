/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : opdata.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/7
  Last Modified :
  Description   : opdata.c的头文件
  Function List :
  History       :
  1.Date        : 2013/11/7
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#ifndef	__OPDATA_H__
#define __OPDATA_H__

#include "ns_table.h"

typedef struct st_data_tag
{
	ULONG		*pData;	
	DCL_NODE_S   stNode;
}OPDATA_DATA_S;

ULLONG Opdata_data_Create(IN TABLE_S *pstTable, ULONG  *pdata, ULONG ulDataSize);
/* 获取数据个数 */
ULONG Opdata_data_GetDataCount(VOID);

/* 数据查找 */
ULONG Opdata_data_FindData(IN TABLE_S  *pstTable,
 						   IN CHAR *szTypeName,
 						   IN VOID  *ulFindData, /* 要比较的数据 */
 						   OUT VOID *pData);

ULONG Opdata_data_getAllData(TABLE_S *pstTable, ULONG *pulData);
ULONG Opdata_data_GetSelectDataCount(ULONG ulTableFd, 
									 NSDB_SELECT_COND_S  *pstSltCond);

ULONG Opdata_data_GetSelectData(IN ULONG ulTableFd, 
								IN  NSDB_SELECT_COND_S  *pstSltCond,
								OUT ULONG *pulData);
ULONG Opdata_data_Init(ULONG ulTableFd);
ULONG Opdata_data_DelNode(IN TABLE_S *pstTable, OPDATA_DATA_S *pstData);
ULONG Opdata_data_DelData(IN TABLE_S *pstTable,
 						  IN CHAR *szTypeName,
 						  IN VOID  *ulFindData);
ULONG 			opdata_data_UpdateNode(OPDATA_DATA_S * pstData);
/********************************************************************************
	IN TABLE_TYPES_S * pstType,		要操作的数据类型
	IN UCHAR *szTypeName,		查找的类型，用于查找到要修改的节点数据
	IN VOID  *ulFindData		查找的数据，用于查找到要修改的节点数据
	IN UCHAR **szTypeNameList,	要修改的类型名列表
	IN VOID  *ulNewDataList,		要修该的新数据的列表
********************************************************************************/
ULONG Opdata_data_UpdateData(	IN TABLE_S *pstTable,
								IN CHAR    *szFindTypeName,
								IN VOID    *ulFindData,
								IN VOID    *pNewData);
VOID Opdata_data_DelAll(TABLE_S *pstTable);
VOID Opdata_data_Fini(TABLE_S *pstTable);

#endif //__OPDATA_H__

