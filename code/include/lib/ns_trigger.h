#ifndef __TRIGGER_H__
#define __TRIGGER_H__

typedef  ULONG (* QUEMSG_TRIGGER_FUN)(IN VOID *,
	 								    IN WEB_HTTP_REQMSGINFO_S *, 
	 								    IN WEB_ACTION_RESP_S *);
typedef struct web_action_trigger_obj
{
	UINT uiTrrEvtID;					/* event id which can trigger this */
	CHAR acTrrName[64];					/* name of this event trigger */
	QUEMSG_TRIGGER_FUN pfQueMsgTrr;     /* funtion call back of this event trigger */
}WEB_ACT_TRR_OBJ_ST;

#include <ns_thread_quemsg.h>	 								    
ULONG Trigger_Handle_Msg(IN INT iSrcThrdId, 
 						 IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg);

#endif //__TRIGGER_H__
