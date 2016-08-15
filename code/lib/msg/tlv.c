/******************************************************************************

  Copyright (C), Night Stark.

 ******************************************************************************
  File Name     : tlv.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2013/11/3
  Last Modified :
  Description   : TLV数据操作
  Function List :
  History       :
  1.Date        : 2013/11/3
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

/*
[   T    |   L    |    V                                                                         |
						C----------------N-----------------T----------------L-----------V
| 总类型 | 总长度 | 有效数据条数 |   数据类型名列表   | 数据类型表(字节) | 长度表(UINT) | 数据区 | 
  1Byte      UINT       UINT-n     STRING[n]('\0'结尾)      UCHAR[n]         ULONG[n]     [data]n
*/

ULONG TLV_UCHAR_Set(IN UCHAR *pSrc, OUT VOID *pDest)
{
	UCHAR *pucDest = (UCHAR *)pDest;
	pucDest = pSrc;
}
ULONG TLV_CHAR_Set()
{
}
ULONG TLV_USHORT_Set()
{
}
ULONG TLV_SHORT_Set()
{
}
ULONG TLV_UINT_Set()
{
}
ULONG TLV_INT_Set()
{
}
ULONG TLV_ULONG_Set()
{
}
ULONG TLV_LONG_Set()
{
}
ULONG TLV_ULONG_Set()
{
}
ULONG TLV_LONG_Set()
{
}
ULONG TLV_ULLONG_Set()
{
}
ULONG TLV_LLONG_Set()
{
}
ULONG TLV_String_Set()
{
}



ULONG TLV_NSDB_Set()
{

}



 

