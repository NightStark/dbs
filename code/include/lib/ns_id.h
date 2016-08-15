/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : ns_id.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/8/30
  Last Modified :
  Description   : ID资源分配
  Function List :
  History       :
  1.Date        : 2014/8/30
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#ifndef __ID_H__
#define __ID_H__

#include "ns_bitmap.h"

LONG CreateIDPool(INT uiIDPoolSize);
VOID DestroyIDPool(LONG lIDPool);
INT  AllocID(LONG lIDPool, UINT uiIDStart);
VOID DeleteID(LONG lIDPool, INT uiID);

#endif //__ID_H__

