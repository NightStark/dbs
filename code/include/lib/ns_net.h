#ifndef __NET_H__
#define __NET_H__

#define NET_INTERFACES_MAX   			(16)
#define NET_INTERFACE_NAME_LEN_MAX 		(16)
#define NET_INTERFACE_ADDR_LEN_MAX  	(16)
#define NET_INTERFACE_MACADDR_LEN_MAX  	(6)


typedef struct ifreq 	NET_INTERFACE_REQ_S;	 
typedef struct ifconf   NET_INTERFACE_CONF_S;	 

typedef struct tag_InterfaceData
{
	CHAR szIFName[NET_INTERFACE_NAME_LEN_MAX];
	UCHAR szIFAddr[NET_INTERFACE_ADDR_LEN_MAX];
    UINT  uiIFAddr;
	UCHAR szIFMACAddr[32];
	UCHAR ucIFMACAddr[NET_INTERFACE_MACADDR_LEN_MAX];
	
}NET_IF_DATA_S;

typedef struct tag_IpInfo
{
	INT iCnt;
	NET_IF_DATA_S pstNetIFData[0]; //
}NET_IP_INFO_S;

UINT   NET_GetLocalIp(NET_IF_DATA_S *pstNetIFData);  
UCHAR *NET_GetLocalMAC(IN CHAR *pcIFName, INOUT UCHAR *pucIFMAC, IN UINT bufLen);
ULONG  NET_GetHosName(CHAR *szHostName, UINT uiHostNameBufLen);
ULONG  NET_GetLocalIPByIFName(CHAR *pcIFName, 
							  NET_IF_DATA_S *pstNetIFDat);
NET_IP_INFO_S *NET_GetIFInfo(VOID);
VOID NET_DestroyIFInfo(NET_IP_INFO_S *pstIpInfo);

ULONG TimerCB_SentIPinfo(INT iThreadId, VOID *arg);

#endif //__NET_H__
