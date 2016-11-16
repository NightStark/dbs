#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <asm/errno.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_client.h>
#include <ns_table_type.h>
#include <ns_nsdb.h>
#include <ns_client_fsm.h>
#include <ns_sm.h>
#include <ns_server.h>
#include <ns_msg_client_link.h>

ULONG MSG_clinet_ctl_recv_attach (MSG_CLT_LINK_ST *pstCltLink, VOID *pMsg)
{
    NS_MSG_ST *pstMsg;
    MSG_CTL_ATTACH_ST stCtlAttach;
    ULONG ulRet = ERROR_FAILE;

    DBGASSERT(NULL != pMsg);

    pstMsg = (NS_MSG_ST *)pMsg;
    //if ()
    ulRet = MSG_GetData(pstMsg, pstMsg->usSubType, &stCtlAttach, sizeof(stCtlAttach));
    if (ulRet != ERROR_SUCCESS) {
        ERR_PRINTF("Get sub type[%d] failed.", pstMsg->usSubType);
    }

    MSG_PRINTF("Attach Mode = %d", stCtlAttach.uiAttachMode);

    return ERROR_SUCCESS;
}
