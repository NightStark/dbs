#ifndef __NS_MSG_TYPE_H__
#define __NS_MSG_TYPE_H__

typedef struct tag_msg_mng_join_req
{
    UINT uiClientID;
    UCHAR ucClientMac[6];
}MSG_MNG_JOIN_REQ_ST;

typedef struct tag_msg_mng_join_resp
{
    UINT  uiServerID;
    ULONG ulSessionID;
}MSG_MNG_JOIN_RESP_ST;

typedef struct tag_msg_mng_confirm
{
    UINT uiConfirmID;
}MSG_MNG_CONFIRM_ST;

typedef struct tag_msg_mng_ok
{
    UINT uiOKID;
}MSG_MNG_OK_ST;


typedef struct tag_msg_ctl_attach
{
    UINT uiAttachMode; /* client mode: slave, mange, server. */
    UINT uiFwVer;
    UINT uiCmdVer; 
    UINT uiCmdID;
}MSG_CTL_ATTACH_ST;

typedef struct tag_msg_ctl_attach_resp
{
    UINT uiAttachStatus; /* MSG_ATTACH_RESP_STATUS_EN, 0:sucess */
}MSG_CTL_ATTACH_RESP_ST;

typedef struct tag_msg_ctl_upgrade
{
    UINT uiCmd;
    UCHAR ucFwUrl[256];
    UCHAR ucSHA256[64];
}MSG_CTL_UPGRADE_ST;

#endif //__NS_MSG_TYPE_H__
