#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>
#include <ns_bitmap.h>
#include <ns_progressbar.h>

STATIC DCL_HEAD_S g_astPrgrsBarHead = {};

typedef struct tag_PrgrsBar
{
	LONG  lPrgrsBarID;			
	UCHAR szPrgrsBarName[PRGRS_BAR_NAME_LEN];	
	DCL_NODE_S stNode;
}PRGRS_BAR_S;

