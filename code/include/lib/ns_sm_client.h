#ifndef __NS_SM_CLIENT_H__
#define __NS_SM_CLIENT_H__

/*****************************
C                 S

-----join req---->
<----join resp---
-----confirm----->
<----ok----------
*****************************/

typedef enum tag_ClientSmStates
{
    CLT_SM_STATS_INIT = 0,
    CLT_SM_STATS_IDEL,
    CLT_SM_STATS_WAIT_JOIN_RESP,
    CLT_SM_STATS_WAIT_CONFIRM,
    CLT_SM_STATS_WAIT_OK,
    CLT_SM_STATS_RUN,
    CLT_SM_STATS_STOP,

    CLT_SM_STATS_MAX,
}CLT_SM_STATS_EN;

typedef enum tag_ClientSMEvents
{
    CLT_SM_EVT_CONNED = 0,
    CLT_SM_EVT_RECV_JOIN_RESP,
    CLT_SM_EVT_RECV_OK,
    CLT_SM_EVT_RECV_CONN_BREAK,

    CLT_SM_EVT_MAX,
}CLT_SM_EVT_EN;

typedef struct tag_ClientSM
{
    INT iSMID;
    CLT_SM_STATS_EN enCltSMStats;
    VOID *pLinkInfo;
}CLT_SM_ST;

#endif //__NS_SM_CLIENT_H__


