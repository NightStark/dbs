/* Append Buffer */
#ifndef __AB_H__
#define __AB_H__

#include <ns_lilist.h>

typedef struct tag_ABInfo AB_INFO_S;

typedef ULONG (* AB_AEEPENDSTR_PF)(AB_INFO_S *, const CHAR *, ...);

struct tag_ABInfo
{
	DCL_NODE_S stNode;

	VOID  *pAB;
	ULONG ulABLen;
	ULONG ulABCurrentEnd;
	AB_AEEPENDSTR_PF *pfAB_AppendString; /* [幻想着某天可以如同面向对象一样] ULONG AB_AEEPENDSTR(AB_INFO_S *, const CHAR *, ...) */
};

ULONG AB_Init(VOID);
ULONG AB_AppendString(AB_INFO_S *pstAB, const CHAR *pcFmt, ...);
AB_INFO_S *AB_Create(ULONG ulNABlen, VOID *pNAB);
VOID AB_Destroy(AB_INFO_S *pstAB);

#endif //__AB_H__
