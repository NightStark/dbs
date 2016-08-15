#ifndef __NSTEST_H__
#define __NSTEST_H__

typedef void (*pfNSTestEle)(VOID *);

#define NSTEST(T_FuncName)			\
ULONG ns_TEST_##T_FuncName(VOID *arg)			

VOID  ns_test_printf(ULONG ulPV);
VOID* ns_test_RegTList(VOID);
VOID  ns_test_UnRegTList(VOID *pDLT);
VOID  ns_test_getTList(VOID);



#endif //__NSTEST_H__
