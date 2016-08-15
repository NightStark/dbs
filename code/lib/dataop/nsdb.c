#ifndef	__NSDB_C__
#define __NSDB_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_opdata.h>
#include <ns_nsdb.h>


UINT nsdb_getTableEleCnt(TABLE_ELE_S *pstTableEle)
{
	UINT uiTableEleCnt = 0;
	while(AT_NONE != pstTableEle[uiTableEleCnt++].ucEleType)
	{
	}

	return uiTableEleCnt - 1;
}

ULONG NSDB_CreateTable(IN const CHAR *szTableName, TABLE_ELE_S *pstTableEle)
{
	ULONG ulNum = 0;
	TABLE_S *pstTable = NULL;
	TABLE_TYPES_S *pstType 	= NULL;

	ulNum = nsdb_getTableEleCnt(pstTableEle);

	/* 构建表包含数据类型字段 */
	pstType = Table_Type_Creat(pstTableEle, ulNum);
	if (NULL == pstType)
	{
		Table_type_Destroy(pstType);
		return ERROR_FAILE;
	}

	/* 创建表头 */
	pstTable = Table_CreateTable(szTableName, pstType);
	if (NULL == pstTable)
	{
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

ULONG NSDB_DeleteTable(CHAR *tableName)
{
	TABLE_S *pstTable = NULL;
	
	pstTable = Table_GetTableByTableName(tableName);
	if (NULL == pstTable)
	{
		return ERROR_DATA_NOT_EXIST;
	}

	Opdata_data_DelAll(pstTable);

	Table_type_Destroy(pstTable->pstTableTypes);

	Table_DestroyTable(tableName);
	
	return ERROR_SUCCESS;
}

#if 0
typedef struct tag_SelectDatas
{
	UCHAR   ucDataNum;
	UCHAR  *ucLineNameList;
	UCHAR  *ucLineTypeList;
	ULONG  *ulLineEleLenList;
	VOID   *DataList;
}NSDB_SELECTDATAS_S;

/* 
| 总类型 | 总长度 | 有效数据条数 | 数据类型名列表 | 数据类型表(字节) | 长度表(UINT) | 数据区 | 
  1Byte      UINT       UINT-n        UCHAR[32n]        UCHAR[n]         ULONG[n]     [data]n
*/

/* 根据表名和条件分配空间 */

NSDB_SELECTDATAS_S * nsdb_AllocSelectData(UCHAR *ucTableName, 
								               NSDB_SELECT_COND_S* pstSltCond)
{
	ULONG 				 ulTableFd 			= TABLE_FD_INVALID;
	TABLE_S 			*pstTable 			= NULL;
	TABLE_TYPES_S   	*pstTypes 			= NULL;
	ULONG 				 ulSelectDataConut 	= 0;
	NSDB_SELECTDATAS_S  *pstSelectDatas 	= NULL;


	ulTableFd = Table_GetTableIDByTableName((const UCHAR *)ucTableName);
	pstTable  = LIST_Table_GetTable(ulTableFd);
	pstTypes  = pstTable->pstTableTypes;

	ulSelectDataConut = Opdata_data_GetSelectDataCount(ulTableFd, pstSltCond);
	ERR_PRINTF("ulSelectDataConut : %lu", ulSelectDataConut);

	pstSelectDatas					 = mem_alloc(sizeof(NSDB_SELECTDATAS_S));
	pstSelectDatas->ucDataNum = ulSelectDataConut;
	pstSelectDatas->ucLineNameList	 = mem_alloc(TABLE_ELEMENT_NAME_LEM_MAX * ulSelectDataConut);
	pstSelectDatas->ulLineEleLenList = mem_alloc(sizeof(CHAR) * ulSelectDataConut);
	pstSelectDatas->ucLineTypeList	 = mem_alloc(sizeof(UINT) * ulSelectDataConut);
	pstSelectDatas->DataList		 = mem_alloc(pstTypes->ulSTLong * ulSelectDataConut);

	return pstSelectDatas;
}

ULONG nsdb_DelSelectData(NSDB_SELECTDATAS_S  *pstSelectDatas)
{
	DBGASSERT(NULL != pstSelectDatas);

	free(pstSelectDatas->ucLineNameList);
	free(pstSelectDatas->ulLineEleLenList);
	free(pstSelectDatas->ucLineTypeList);
	free(pstSelectDatas->DataList);
	pstSelectDatas->ucDataNum = 0;
	free(pstSelectDatas);
	pstSelectDatas = NULL;

	return ERROR_SUCCESS;
}

ULONG NSDB_SelectDataGetFromDB(UCHAR *ucTableName, 
								         NSDB_SELECT_COND_S* pstSltCond,
								         NSDB_SELECTDATAS_S *pstSelectDatas)
{
	int 				 i					= 0;
	ULONG 				 ulRet				= ERROR_SUCCESS;
	TABLE_S 			*pstTable 			= NULL;
	TABLE_TYPES_S   	*pstTypes 			= NULL;
	ULONG 				 ulTableFd 			= TABLE_FD_INVALID;

	ulTableFd = Table_GetTableIDByTableName((const UCHAR *)TABLE_TEST_NAME);
	pstTable = LIST_Table_GetTable(ulTableFd);
	pstTypes = pstTable->pstTableTypes;

	for (i = 0; i < pstTypes->ucTypeCount; i++)
	{
		str_safe_cpy((CHAR *)(pstSelectDatas->ucLineNameList + TABLE_ELEMENT_NAME_LEM_MAX * i),
 					 (const char *)pstTypes->pucTypeName[i]
 					 ,strlen((const char *)pstTypes->pucTypeName[i]));

		pstSelectDatas->ulLineEleLenList[i] = pstTypes->pulTypeLen[i];
		pstSelectDatas->ucLineTypeList[i]   = pstTypes->pucTypeMap[i];
	}

	ulRet = Opdata_data_GetSelectData(ulTableFd, 
									  pstSltCond, 
									  (ULONG *)pstSelectDatas->DataList);	
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Get Select Data Failed!");
	}

	return ulRet;
}


ULONG _NSDB_SelectData(UCHAR *ucTableName, NSDB_SELECT_COND_S* pstSltCond)
{
	int i;
	ULONG ulSelectDataConut = 0;
	TABLE_S *pstTable = NULL;
	TABLE_TYPES_S  *pstTypes = NULL;
	NSDB_SELECTDATAS_S *pstSelectDatas = NULL;

	
	ULONG ulDataGetID = 0;
	UCHAR szDataGetName[32] = {};
	UCHAR ucDataGetAge= 0;
	BOOL_T bDataGetSex= 0;
	UCHAR *pucPTDataBuf = NULL;
	ULONG  ulTypeLen			= 0;

	ULONG ulTableFd = TABLE_FD_INVALID;

	ulTableFd = Table_GetTableIDByTableName((const UCHAR *)TABLE_TEST_NAME);
	pstTable = LIST_Table_GetTable(ulTableFd);
	pstTypes = pstTable->pstTableTypes;

	ulSelectDataConut = Opdata_data_GetSelectDataCount(ulTableFd, pstSltCond);
	ERR_PRINTF("ulSelectDataConut : %lu", ulSelectDataConut);
	ulTypeLen = pstTypes->ulSTLong;

	pstSelectDatas = nsdb_AllocSelectData(ucTableName, pstSltCond);

	//????复制出的数据不正确????尼玛啊
	NSDB_SelectDataGetFromDB(ucTableName, pstSltCond, pstSelectDatas);
				
	pucPTDataBuf = pstSelectDatas->DataList;
		
	i = 0;
	while(i < ulSelectDataConut)
	{
		Table_type_DataGet(pstTypes, (const UCHAR *)"User ID", pucPTDataBuf ,(VOID *)(&ulDataGetID));		
		ERR_PRINTF("User ID = %lu", ulDataGetID);	
		Table_type_DataGet(pstTypes, (const UCHAR *)"User Name",pucPTDataBuf,(VOID *)szDataGetName);
		DBGASSERT(NULL != pstTypes);
		ERR_PRINTF("User Name = %s", szDataGetName);
		Table_type_DataGet(pstTypes, (const UCHAR *)"User Age",pucPTDataBuf,(VOID *)(&ucDataGetAge));
		ERR_PRINTF("User Age = %d", ucDataGetAge);
		Table_type_DataGet(pstTypes, (const UCHAR *)"User Sex",pucPTDataBuf,(VOID *)(&bDataGetSex));
		ERR_PRINTF("User Sex = %d", bDataGetSex);

		pucPTDataBuf += ulTypeLen;
		i++;
		ERR_PRINTF("Data : %d",i);		
	}

	nsdb_DelSelectData(pstSelectDatas);


}

CHAR NSDB_GetTableInfo(IN  UCHAR *ucTableName, IN TABLE_ELE_S *pstTableEle)
{
	UINT	 uiIndex = 0;
	ULONG    ulRet = ERROR_SUCCESS;
	ULONG    ulTableFd = 0;
	ULONG   *pulDataTemp     = NULL;
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	
	ulTableFd = Table_GetTableIDByTableName((const UCHAR *)ucTableName);
	pstTable  = LIST_Table_GetTable(ulTableFd);
	pstType   = pstTable->pstTableTypes;

	for(uiIndex = 0;uiIndex < pstType->ucTypeCount; uiIndex++)
	{
		str_safe_cpy(pstTableEle[uiIndex].szTypeName, 
						pstType->pucTypeName[uiIndex],
						sizeof(pstTableEle[uiIndex].szTypeName));
	}

	return pstType->ucTypeCount;
}

#endif

ULONG NSDB_FindData(IN  CHAR *TableName, 
 				    IN  CHAR *szTypeName,
 				    IN  VOID *ulFindData,
 				    OUT GET_DATA_ELE_S *pstGetDataEle )
{
	UINT     uiIndex = 0;
	ULONG    ulRet = ERROR_SUCCESS;
	CHAR    *szData = NULL;
	CHAR    *szTypeNameTemp = NULL;
	ULONG   *pulDataTemp     = NULL;
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	
	pstTable = Table_GetTableByTableName(TableName);
	if (NULL == pstTable)
	{
		return ERROR_DATA_NOT_EXIST;
	}
	pstType  = pstTable->pstTableTypes;

	szData = mem_alloc(pstType->ulSTLong);
	if (NULL == szData)
	{
		return ERROR_NOT_ENOUGH_MEM;
	}
	
	ulRet = Opdata_data_FindData(pstTable, szTypeName, ulFindData, szData);
	if(ERROR_DATA_NOT_EXIST == ulRet)
	{
		return ERROR_DATA_NOT_EXIST;
	}

	for(uiIndex = 0; uiIndex < pstType->ucTypeCount; uiIndex++)
	{
		szTypeNameTemp = pstGetDataEle[uiIndex].szTypeName;
		pulDataTemp = (ULONG *)(pstGetDataEle[uiIndex].pData);
		
		ulRet = Table_type_DataGet(pstType, 
								   szTypeNameTemp, 
								   szData, 
								   pulDataTemp);

	}

	free(szData);

	return ulRet;
}

ULONG NSDB_DelData(IN CHAR *TableName, 
 				   IN CHAR *szTypeName,
 				   IN VOID  *ulFindData)
{
	ULONG ulRet 			= ERROR_FAILE;
	TABLE_S *pstTable 		= NULL;
	
	pstTable = Table_GetTableByTableName(TableName);
	if (NULL == pstTable)
	{
		return ERROR_DATA_NOT_EXIST;
	}
	
	ulRet = Opdata_data_DelData(pstTable, szTypeName, ulFindData);

	return ulRet;
}

ULONG NSDB_AddData(CHAR *TableName, VOID *pAddEle, UINT uiEleNum)
{
	
	UINT     uiTypeI       = 0; 
	ULONG    ulRet 		   = ERROR_FAILE;
	VOID    *pDBDataBuf    = NULL;
	TABLE_S *pstTable 	   = NULL;
	TABLE_TYPES_S *pstType = NULL;
	ADD_DATA_ELE_S *pstAddDataEle = NULL;
	
	pstTable = Table_GetTableByTableName(TableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table[%s] is not exist!", TableName);
		return ERROR_FAILE;
	}
	pstType = pstTable->pstTableTypes;

	if (uiEleNum > pstType->ucTypeCount)
	{
		ERR_PRINTF("This Table Don't Have so many Element!");
		return ERROR_FAILE;
	}

	/* 获取数据元素列的第一个元素的指针 */
	pstAddDataEle = (ADD_DATA_ELE_S *)(ULONG *)pAddEle;

	/* szData为要创建的数据（要保存的数据） */
	pDBDataBuf = mem_alloc(pstType->ulSTLong);
	
	for ( uiTypeI = 0; uiTypeI < uiEleNum; uiTypeI++ )
	{
		ulRet = Table_type_DataCreate(	pDBDataBuf, 
										pstType, 
										pstAddDataEle[uiTypeI].szTypeName,
										pstAddDataEle[uiTypeI].pData);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("table type date Create Failed!");
			return ERROR_FAILE;
		}
		//pstAddDataEle += sizeof(ADD_DATA_ELE_S);
	}
	DBG_PRINT_LOG("pDBDataBuf.bat",pDBDataBuf,pstType->ulSTLong);

	ulRet = Opdata_data_Create(pstTable, (ULONG *)(pDBDataBuf), pstType->ulSTLong);
	
	free(pDBDataBuf);
	pDBDataBuf = NULL;

	return ulRet;
}



ULONG NSDB_UpdateData(IN CHAR *TableName, 
					  IN VOID *pAddEle, 
					  IN UINT  uiEleNum,
					  IN CHAR *szTypeName,
					  IN VOID *ulFindData)
{
	
	ULONG    ulRet 	       = ERROR_FAILE;
	UINT     uiTypeI       = 0; 
	VOID    *pDBDataBuf    = NULL;
	TABLE_S *pstTable      = NULL;
	TABLE_TYPES_S *pstType = NULL;
	ADD_DATA_ELE_S *pstAddDataEle = NULL;
	
	pstTable = Table_GetTableByTableName(TableName);
	if (NULL == pstTable)
	{
		ERR_PRINTF("Table[%s] is not exist!", TableName);
		return ERROR_FAILE;
	}
	pstType = pstTable->pstTableTypes;

	if (uiEleNum > pstType->ucTypeCount)
	{
		ERR_PRINTF("This Table Don't Have so many Element!");
		return ERROR_FAILE;
	}

	/* 获取数据元素列的第一个元素的指针 */
	pstAddDataEle = (ADD_DATA_ELE_S *)(ULONG *)pAddEle;

	/* szData为要创建的数据（要保存的数据） */
	pDBDataBuf = mem_alloc(pstType->ulSTLong);
	
	for ( uiTypeI = 0; uiTypeI < uiEleNum; uiTypeI++ )
	{
		ulRet = Table_type_DataCreate(	pDBDataBuf, 
										pstType, 
										pstAddDataEle[uiTypeI].szTypeName,
										pstAddDataEle[uiTypeI].pData);
		if (ERROR_SUCCESS != ulRet)
		{
			ERR_PRINTF("table type date Create Failed!");
			return ERROR_FAILE;
		}
		//pstAddDataEle += sizeof(ADD_DATA_ELE_S);
	}
	DBG_PRINT_LOG("pDBDataBuf.bat",pDBDataBuf,pstType->ulSTLong);

	ulRet = Opdata_data_UpdateData(pstTable, szTypeName, ulFindData, pDBDataBuf);
	
	free(pDBDataBuf);
	pDBDataBuf = NULL;

	return ulRet;
}


/* 测试  */

#if 0


/* 数据组织结构体，其实也可以数组 */
typedef struct tag_data
{
	ADD_DATA_ELE_S stUserId;
	ADD_DATA_ELE_S stUserName;
	ADD_DATA_ELE_S stUserAge;
	ADD_DATA_ELE_S stUserSex;
	ADD_DATA_ELE_S stUserAddr;
	
}T_DATA_S;

VOID add_data(ULONG ulAddNum)
{
	UCHAR name[32] 			= {};
	int i;

	ULONG ulTableFd 		= TABLE_FD_INVALID;

	T_DATA_S pstTData[1000];

	NSDB_SELECT_COND_S  pstSltCond;
	
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	pstTable = Table_GetTableByTableName((const UCHAR *)TABLE_TEST_NAME);
	pstType = pstTable->pstTableTypes;
	for(i = 0; i < ulAddNum; i++)
	{
	/*
	i < 123时吐核 245时也土河
	*/	

		strcpy(pstTData[i].stUserId.szTypeName, 	"User ID");
		strcpy(pstTData[i].stUserName.szTypeName, 	"User Name");
		strcpy(pstTData[i].stUserAge.szTypeName, 	"User Age");
		strcpy(pstTData[i].stUserSex.szTypeName, 	"User Sex");
		strcpy(pstTData[i].stUserAddr.szTypeName, 	"User Address");
	
		pstTData[i].stUserId.pData 	= (VOID *)(ULONG)(8800 + i);
		sprintf(name,"Liu De Hua %d" , i);
		pstTData[i].stUserName.pData 	= (VOID *)name;
		pstTData[i].stUserAge.pData 	= (VOID *)(ULONG)(25 + i);
		pstTData[i].stUserSex.pData 	= (VOID *)(ULONG)BOOL_TRUE;
		pstTData[i].stUserAddr.pData 	= (VOID *)(ULONG)name;	
	}
	
	for(i = 0; i < ulAddNum; i++)
	{
		NSDB_AddData((UCHAR *)TABLE_TEST_NAME, (VOID *)(&pstTData[i]), 5);
	}
	
	//opdata_dis(pstType);

	mem_set(&pstSltCond, 0, sizeof(NSDB_SELECT_COND_S));
	strcpy(pstSltCond.szEleName, "User Age");
	pstSltCond.ulEleValueMax = 55;
	pstSltCond.ulEleValueMin = 33;

	ULONG ulSDC = 0;
	//ulSDC = Opdata_data_GetSelectDataCount(ulTableFd, &pstSltCond);
	//ERR_PRINTF("ulSDC = %lu" ,ulSDC);

	//_NSDB_SelectData(TABLE_TEST_NAME, &pstSltCond);

	
}

ULONG NSDB_CMD_CreateTable()
{
	ULONG ulRet = ERROR_SUCCESS;
	TABLE_ELE_S *pstTableEle = NULL;
	UCHAR tableName[32];

	ULONG i = 0;
	TABLE_S *pstTable 		= NULL;
	TABLE_TYPES_S *pstType 	= NULL;
	pstTable = Table_GetTableByTableName((const UCHAR *)TABLE_TEST_NAME);
	pstType = pstTable->pstTableTypes;
/*	printf("TableName:");
	scanf("%s",tableName);
	while ( 1 )
	{
		printf("");		
	}
*/	
	ULONG ulTableFd = TABLE_FD_INVALID;
	pstTableEle = mem_alloc(sizeof(TABLE_ELE_S) * 5);
	strcpy(pstTableEle[0].szTypeName, "User ID"); 
	pstTableEle[0].ucEleType = Table_type_GetTypeLen(AT_ULONG);
	pstTableEle[0].ucTypelen = 4;
	
	strcpy(pstTableEle[1].szTypeName, "User Name"); 
	pstTableEle[1].ucEleType = AT_SZ;
	pstTableEle[1].ucTypelen = 32;

	strcpy(pstTableEle[2].szTypeName, "User Age"); 
	pstTableEle[2].ucEleType = AT_CHAR;
	pstTableEle[2].ucTypelen = Table_type_GetTypeLen(AT_CHAR);

	strcpy(pstTableEle[3].szTypeName, "User Sex"); 
	pstTableEle[3].ucEleType = AT_BOOL_T;
	pstTableEle[3].ucTypelen = Table_type_GetTypeLen(AT_BOOL_T);
	
	strcpy(pstTableEle[4].szTypeName, "User Address"); 
	pstTableEle[4].ucEleType = AT_SZ;
	pstTableEle[4].ucTypelen = 32;

	ulRet = NSDB_CreateTable((UCHAR *)TABLE_TEST_NAME, pstTableEle);
	free(pstTableEle);
	pstTableEle = NULL;
	
	if(ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Cant't Create Table!");
	}


	//for(i = 0; i < 1000 ; i++)
	//{
		add_data(100);
		Opdata_data_DelAll(pstTable);
		//ERR_PRINTF("i = %d", i);
	//}
	//Opdata_data_Fini(ulTableFd);

	

}


ULONG NSDB_SQL_EXE(UCHAR * pcStrSql)
{
/*
	create table userData {
		userName,CHAR,20;
		userAge,INT;
		userID,INT;
		userEmail,CHAR,36;
	}
*/
/*
	先把"{}"之外的字符放到一个数组里面， 然后再把{}里面的按照标点(, ; 括号) 进行参数解析
	再调用函数

*/

}
#endif

#endif //__NSDB_C__
