/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : opdata.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/7
  Last Modified :
  Description   : Table_Type.c的头文件
  Function List :
              Table_type_Destroy
              Table_DestroyALL
              Table_Type_AllocMem
              Table_Type_Creat
              Table_type_DataCreate
              Table_type_DataGet
              Table_type_GetTypeIndex
              Table_type_GetTypeLen
  History       :
  1.Date        : 2013/11/7
    Author      : langyanjun
    Modification: Created file

******************************************************************************/ 

#ifndef __TABLE_TYPE_H__
#define __TABLE_TYPE_H__

 /* 自定义类型集合 */
 typedef struct st_table_tag
 {
	 UINT			 uiTableID;
	 CHAR			 szTableName[TABLE_NAME_LEN_MAX];
	 TABLE_TYPES_S	*pstTableTypes;
	 ULONG			 ulTableLen;	 /* 该表的当前数据的个数 */
	 DCL_HEAD_S 	 stTableHead;	 /* 该类型的数据挂接到的头节点 */
 
	 DCL_NODE_S stNode;
 }TABLE_S;

 typedef struct tag_table_element
 {
	 UCHAR ucEleType;
	 CHAR szTypeName[TABLE_ELEMENT_NAME_LEM_MAX];
	 UCHAR ucTypelen;
 }TABLE_ELE_S;
#define TABLE_TEST_NAME "Table_Test"
 
 /*  在添加数据时，作为一条数据的一个元素的临时元素 */
 typedef struct tag_AddDataEle
 {
	 CHAR szTypeName[TABLE_ELEMENT_NAME_LEM_MAX];
	 VOID  *pData;
 }ADD_DATA_ELE_S;
 
 typedef struct tag_GetDataEle
 {
	 CHAR szTypeName[TABLE_ELEMENT_NAME_LEM_MAX];
	 UCHAR ucEleType;
	 VOID  *pData;
 }GET_DATA_ELE_S;

 /* 申请类型结构体空间 */
 TABLE_TYPES_S * Table_Type_AllocMem(void);
 
 /* 创建数据类型 
    其实就是创建类型结构体( TABLE_TYPES_S )并赋值
 */
 TABLE_TYPES_S * Table_Type_Creat(IN TABLE_ELE_S *pstTableEle,
								  IN ULONG ulNum);
 
 ULONG Table_type_GetTypeIndex(IN TABLE_TYPES_S * pstType,
							   IN const CHAR *szTypeName);
 
 
 ULONG Table_type_DataCreate(IN VOID *pDBBuf,
							 IN TABLE_TYPES_S * pstTypes, 
							 IN const CHAR *szTypeName,
							 IN VOID *pData);
									
 /* 根据类型查找数据, 从数据区分离出指定类型的数据 */
 ULONG Table_type_DataGet( IN TABLE_TYPES_S * pstType,
 						   IN const CHAR *szTypeName,
 						   IN VOID  *pDBBuf,
 						   IN VOID  *ulGetData);
 /* 释放与类型相关的内存 */
 VOID Table_type_Destroy(TABLE_TYPES_S	*pstTableTypes);
 ULONG Table_type_GetTypeLen(UCHAR ucType);

 #endif //__TABLE_TYPE_H__

