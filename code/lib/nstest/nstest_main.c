#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>

#include <ns_test.h>

/*
*/	




/* RUN before main() */
__attribute((constructor)) void NSTEST_TestFuncReg()
{

	//printf("%s\n",__FUNCTION__);
}

/* RUN after main() */
__attribute((destructor)) void NSTEST_TestFuncUnReg()
{
	//printf("%s\n",__FUNCTION__);
}

INT main(VOID)
{
	VOID *pDLT = NULL; 
	
	NSTEST_Init();

	/* 从动态库读取TEST函数符号放到全局变量 */
	ns_test_getTList();

	/* 根据函数符号表，获取函数指针并加入链表 */
	pDLT = ns_test_RegTList();
	if (NULL == pDLT)
	{ 
		return;
	}

	MSG_PRINTF("Test start ..");

	/* 运行测试函数表 */
	NSTEST_Run();

	/* 卸载动态库 */
	ns_test_UnRegTList(pDLT);
	
	/* 释放链表数据 */
	NSTEST_ReleaseAll();

	NSTEST_Fint();
	return 0;
}

