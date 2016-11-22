#ifndef __NS_SM_H__
#define __NS_SM_H__

#include <ns_sm_server.h>
#include <ns_sm_client.h>

/*********** Server *************************************************/
typedef INT (*SRV_SM_EVT_PF)(SRV_SM_ST *, SRV_SM_EVT_EN, VOID *);

typedef struct tag_SrvSmEvtMap
{
    SRV_SM_EVT_EN enSrvSMEvt;
    SRV_SM_EVT_PF pfSrvSMEvt;
}SRV_SM_EVT_MAP_ST;


SRV_SM_STATS_EN SRV_SM_STATUS_GET(SRV_SM_ST *pstSrvSM);
SRV_SM_ST *SRV_sm_CreateAndStart(VOID);
INT  SRV_sm_Stop(SRV_SM_ST *pstSrvSm);
VOID SRV_sm_Destroy(SRV_SM_ST *pstSrvSm);
INT SRV_SM_STATS_CHANGE(SRV_SM_ST *pstSrvSM, SRV_SM_STATS_EN enSrvSMStatNow);
INT SRV_SM_EVT_HANDLE(IN SRV_SM_ST *pstSrvSM, IN SRV_SM_EVT_EN enSrvSMEvt, IN VOID * args);
VOID SRV_sm_Init(VOID);

/*********** Client *************************************************/
typedef INT (*CLT_SM_EVT_PF)(CLT_SM_ST*, CLT_SM_EVT_EN, VOID *);

typedef struct tag_CLtSmEvtMap
{
    CLT_SM_EVT_EN enCltSMEvt;
    CLT_SM_EVT_PF pfCltSMEvt;
}CLT_SM_EVT_MAP_ST;


CLT_SM_ST * CLT_sm_CreateAndStart(VOID);
INT CLT_sm_Stop(CLT_SM_ST *pstCltSm);
VOID CLT_sm_Destroy(IN CLT_SM_ST *pstCltSm);
INT CLT_SM_STATS_CHANGE(CLT_SM_ST *pstCltSM, CLT_SM_STATS_EN enCLtSMStatNow, VOID *args);
CLT_SM_STATS_EN CLT_SM_STATUS_GET(CLT_SM_ST *pstCltSM);
INT CLT_SM_EVT_HANDLE(IN CLT_SM_ST *pstCltSM, IN CLT_SM_EVT_EN enCltSmEvt, IN VOID * args);
INT CLT_sm_init(void);

#endif //__NS_SM_H__
