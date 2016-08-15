#ifndef __TRIGGER_H__
#define __TRIGGER_H__

typedef  ULONG (* QUEMSG_TRIGGER_FUN)(IN VOID *,
	 								    IN WEB_HTTP_REQMSGINFO_S *, 
	 								    IN WEB_ACTION_RESP_S *);
#include <ns_thread_quemsg.h>	 								    
ULONG Trigger_Handle_Msg(IN INT iSrcThrdId, 
 						 IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg);

#endif //__TRIGGER_H__
