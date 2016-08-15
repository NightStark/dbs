#ifndef __BITMAP_H__
#define __BITMAP_H__

#define BIT_MAP_NUM_MAX (256)

#define BIT_MAP_INVALID_ID (-1)

LONG CreateBitMap(UINT uiMapSize);
VOID DestroyBitMap(LONG lBitMapID);
ULONG BitMap_SetBit(LONG lBitMapID, UINT uiStartPos, UINT uiEndPos);
ULONG BitMap_SetBitALL(LONG lBitMapID);
ULONG BitMap_ClearBit(LONG lBitMapID,UINT uiStartPos,UINT uiEndPos);
ULONG BitMap_ClearBitALL(LONG lBitMapID);
BIT_T BitMap_GetBitVal(LONG lBitMapID, UINT uiPos);
UINT  BitMap_GetSize(LONG lBitMapID);
/*
	获取bitV(0 or 1)在 uiStartPos 到 uiEndPos 第一次出现的位置
*/
INT  BitMap_GetPos(LONG lBitMapID, UINT uiStartPos, UINT uiEndPos, BIT_T bitV);
VOID BitMap_Init(void);

#endif //__BITMAP_H__
