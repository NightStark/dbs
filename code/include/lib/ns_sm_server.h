#ifndef __NS_SM_SERVER_H__
#define __NS_SM_SERVER_H__

typedef enum tag_ServerSmStates
{
    SRV_SM_STATS_INIT = 0,
    SRV_SM_STATS_IDEL,
    SRV_SM_STATS_WAIT_JOIN,
    SRV_SM_STATS_WAIT_CONFIRM,
    SRV_SM_STATS_RUN,
    SRV_SM_STATS_STOP,

    SRV_SM_STATS_MAX,
}SRV_SM_STATS_EN;

typedef enum tag_ServerSMEvents
{
    SRV_SM_EVT_CONNED = 0,
    SRV_SM_EVT_RECV_JOIN,
    SRV_SM_EVT_RECV_CONFIRM,
    SRV_SM_EVT_RECV_CONN_BREAK,

    SRV_SM_EVT_MAX,
}SRV_SM_EVT_EN;

/* 每一个连接，应该对应于一个状态机 */

typedef struct tag_SrvSM
{
    INT iSMID;
    SRV_SM_STATS_EN enSrvSMStats; /* the stats of the sm now */
    VOID *pLink;
}SRV_SM_ST;

#endif //__NS_SM_SERVER_H__
