/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : Table_Type.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/10/26
  Last Modified :
  Description   : 与表相关的[数据类型]的操作
  Function List :
  History       :
  1.Date        : 2013/10/26
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_lilist.h>
#include <ns_table.h>
 
 /* 申请类型结构体空间 */
 TABLE_TYPES_S * Table_Type_AllocMem(void)
 {
	 TABLE_TYPES_S *pstType;
	 pstType = (TABLE_TYPES_S *)mem_alloc(sizeof(TABLE_TYPES_S));
	 if(NULL == pstType)
	 {
		 ERR_PRINTF("malloc failed!");
		 
	 }
 
	 return pstType; 
 }
 
 /* 创建数据类型 
    其实就是创建类型结构体( TABLE_TYPES_S )并赋值
 */
 TABLE_TYPES_S * Table_Type_Creat(IN TABLE_ELE_S *pstTableEle,
								  IN ULONG ulNum)
 {
	 UINT           uiTCounter = 0;
	 TABLE_TYPES_S *pstType;
 
	 pstType = Table_Type_AllocMem();
	 if (NULL == pstType)
	 {
		goto err_Type_AllocMem;
	 }
 
	 //*******
	 pstType->ucTypeCount = ulNum;
	 
 
	 /* 根据类型元素个数分配各种空间 */
	 pstType->pucTypeName	 = (UCHAR **)mem_alloc(ulNum * sizeof(UCHAR *));
	 if (NULL == pstType->pucTypeName) 	 goto err_mem_alloc_pucTypeName;
	 pstType->pulTypeLen	 = (ULONG *)mem_alloc(ulNum * sizeof(ULONG));
	 if (NULL == pstType->pulTypeLen) 	 goto err_mem_alloc_pulTypeLen;
	 pstType->pucTypeMap	 = (UCHAR *)mem_alloc(ulNum * sizeof(UCHAR));
	 if (NULL == pstType->pucTypeMap) 	 goto err_mem_alloc_pucTypeMap;
	 pstType->pucTypeOffset  = (ULONG *)mem_alloc(ulNum * sizeof(ULONG));
 	 if (NULL == pstType->pucTypeOffset) goto err_mem_alloc_pucTypeOffset;
 
	 /* 各种必要元素赋值 */
	 for(uiTCounter = 0; uiTCounter < ulNum; uiTCounter++)
	 {
		 pstType->pucTypeName[uiTCounter] = (UCHAR *)mem_alloc(strlen(pstTableEle[uiTCounter].szTypeName) + 1);
		 if(NULL == pstType->pucTypeName[uiTCounter])
		 {
			 goto err_mem_alloc_TypeName;
		 }
		 
		 //*******
		 strcpy((CHAR *)pstType->pucTypeName[uiTCounter], pstTableEle[uiTCounter].szTypeName);
		 //*******
		 pstType->pulTypeLen[uiTCounter] = pstTableEle[uiTCounter].ucTypelen;
		 //*******
		 pstType->pucTypeMap[uiTCounter] = pstTableEle[uiTCounter].ucEleType;
		 //*******
		 pstType->pucTypeOffset[uiTCounter] = pstType->ulSTLong;
		 //*******
		 pstType->ulSTLong += pstTableEle[uiTCounter].ucTypelen;
	 }

return pstType;

err_mem_alloc_TypeName:
	for(uiTCounter--; uiTCounter >= 0; uiTCounter--)
	{
		free(pstType->pucTypeName[uiTCounter]);
	}

	free(pstType->pucTypeOffset);
err_mem_alloc_pucTypeOffset:
	free(pstType->pucTypeMap);
err_mem_alloc_pucTypeMap:
	free(pstType->pulTypeLen);
err_mem_alloc_pulTypeLen:
	free(pstType->pucTypeName);
err_mem_alloc_pucTypeName:
	free(pstType);
err_Type_AllocMem:
	
	 return NULL;
 }
 
 ULONG Table_type_GetTypeIndex(IN TABLE_TYPES_S * pstType,
							   IN const CHAR *szTypeName)
 {
	 
	 ULONG ulTCounter = 0;
 
	 for(ulTCounter = 0; ulTCounter < pstType->ucTypeCount; ulTCounter++)
	 {
		 if(0 == strcmp((CHAR *)pstType->pucTypeName[ulTCounter],szTypeName))
		 {
			 return ulTCounter;;
		 }
	 }
 
	 return ERROR_INVALID_TYPE_NAME;
 }
 
 
 /* 根据类型创建数据创建数据 */
/*****************************************************************************
 Prototype    : Table_type_DataCreate
 Description  : 创建数据
 Input        : IN VOID *pDBBuf    		要创建的数据，一块内存         
                IN TABLE_TYPES_S * pstTable  该表的类型约束
                IN const UCHAR *szTypeName   本次要在数内存中创建的数据类型的名称
                IN ULONG ulData              本次要在数据内存中创建的数据的数据的值
 Output       : None
 Return Value : ULONG
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2013/10/29
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
 ULONG Table_type_DataCreate(IN VOID *pDBBuf,
							 IN TABLE_TYPES_S * pstTypes, 
							 IN const CHAR *szTypeName,
							 IN VOID *pData)
 {
	 ULONG ulTIndex = 0;
	 VOID  *pDesc = NULL;
	 VOID  *pSrc  = NULL;
	 ULONG	ulTypeLen = 0;
	 
	 ulTIndex = Table_type_GetTypeIndex(pstTypes, szTypeName);
 
	 if(ERROR_INVALID_TYPE_NAME == ulTIndex)
	 {
		 return ERROR_INVALID_TYPE_NAME;
	 }
 
	 pDesc = (pDBBuf + (pstTypes->pucTypeOffset[ulTIndex]));
	 ulTypeLen = pstTypes->pulTypeLen[ulTIndex];
	 
	 pSrc	= ((VOID *)(ULONG *)(pData));
	 
	 memcpy(pDesc, pSrc, ulTypeLen);
 
	 return ERROR_SUCCESS;
	 
 }
 
 /* 根据类型查找数据, 从数据区分离出指定类型的数据 */
 ULONG Table_type_DataGet( IN TABLE_TYPES_S * pstType,
 						   IN const CHAR *szTypeName,
 						   IN VOID  *pDBBuf,
 						   IN VOID  *ulGetData)
 {
	 ULONG ulTIndex = 0;
	 
	 VOID *pDest = NULL;
	 VOID *pSrc  = NULL;
	 ULONG ulTypeLen = 0;
	 
	 DBGASSERT(NULL != ulGetData);
	 
	 ulTIndex = Table_type_GetTypeIndex(pstType, szTypeName);
	 if(ERROR_INVALID_TYPE_NAME == ulTIndex)
	 {
		 return ERROR_INVALID_TYPE_NAME;
	 }
 
	 ulTypeLen = pstType->pulTypeLen[ulTIndex];  
	 pSrc= (pDBBuf + pstType->pucTypeOffset[ulTIndex]);
	 pDest= ((VOID *)(ULONG *)(ulGetData));
 
	 memcpy(pDest, pSrc, ulTypeLen);
 
	 return ERROR_SUCCESS;
 }
 
 /* 释放与类型相关的内存 */
 VOID Table_type_Destroy(TABLE_TYPES_S	*pstTableTypes)
 {
	 ULONG	  ulTNIndex	 = 0;
	 
	 /* 释放 TABLE_TYPES_S */
	 free(pstTableTypes->pucTypeMap);
	 
	 for(ulTNIndex = 0; 
		 ulTNIndex < pstTableTypes->ucTypeCount; 
		 ulTNIndex++)
	 {
		 free(pstTableTypes->pucTypeName[ulTNIndex]);
	 }
	 
	 free(pstTableTypes->pucTypeName);
	 free(pstTableTypes->pucTypeOffset);
	 free(pstTableTypes->pulTypeLen);
 
	 /* 释放 TABLE_TYPES_S*/
	 free(pstTableTypes);
	 pstTableTypes = NULL;

	 return;
 }

 
 ULONG Table_type_GetTypeLen(UCHAR ucType)
 {
 	ULONG ulTypeLen = 0;
	 switch(ucType)
	 {
		case AT_CHAR:
			ulTypeLen = 1;
			break;
		case AT_UCHAR:
			ulTypeLen = 1;
			break;
		case AT_SZ:
			ulTypeLen = 4;
			break;
		case AT_SHORT:
			ulTypeLen = 2;
			break;
		case AT_USHORT:
			ulTypeLen = 2;
			break;
		case AT_INT:
			ulTypeLen = 4;
			break;
		case AT_UINT:
			ulTypeLen = 4;
			break;
		case AT_LONG:
			ulTypeLen = 4;
			break;
		case AT_ULONG:
			ulTypeLen = 4;
			break;
		case AT_LLONG:
			ulTypeLen = 8;
			break;
		case AT_ULLONG:
			ulTypeLen = 8;
			break;
		case AT_FLOAT:
			ulTypeLen = 4;
			break;
		case AT_DOUBLE:
			ulTypeLen = 8;
			break;
		case AT_BOOL_T:
			ulTypeLen = 1;
			break;
		default:
			ulTypeLen = 0;
			break;
	 }
	 return ulTypeLen;
 }

