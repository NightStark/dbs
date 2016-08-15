#ifndef __TABLE_H__
#define __TABLE_H__

#include "ns_lilist.h"
#include "ns_table_type.h"

//******************************************************************

/* 全局数据记录当前已定义的数据(结构体)类型 */

typedef struct tag_SelectCondition
{
	
	CHAR szEleName[TABLE_ELEMENT_NAME_LEM_MAX];
	ULONG ulEleValueMax;
	ULONG ulEleValueMin;
}NSDB_SELECT_COND_S;

ULONG DataBase_Init(VOID);
ULONG DataBase_Fini(VOID);
TABLE_S * Table_GetTableByTableName(IN const CHAR *szTableName);
TABLE_S * Table_GetTableByTableID(IN UINT uiTableID);
ULONG Table_GetTableIDByTableName(IN const CHAR *szTableName);
TABLE_S * Table_CreateTable(IN const CHAR *szTableName,
							IN TABLE_TYPES_S *pstTablesType);
VOID  Table_DestroyTable(IN const CHAR *szTableName);
ULONG Table_GetTableConut(IN UINT uiTableID);
ULONG Table_GetTableLen(IN UINT uiTableID);
DCL_HEAD_S * Table_GetTableHead(IN const CHAR *szTableName);

#endif //__TABLE_H__

