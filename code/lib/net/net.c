#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>


ULONG NET_GetHosName(CHAR *szHostName, UINT uiHostNameBufLen)
{
    struct hostent *hostMSg = 0;

	
    hostMSg = gethostent();
    if(NULL == hostMSg)
    	return ERROR_FAILE;
	str_safe_cpy(szHostName, hostMSg->h_name, uiHostNameBufLen);
	
    endhostent();
    return ERROR_SUCCESS;
}

UINT NET_GetLocalIp(NET_IF_DATA_S *pstNetIFData)  
{        
    CHAR *pcLocalip = NULL;  
	INT   iRet      = 0;
    INT   fd        = MSG_SOCKET_INVALID_FD;  
	INT   iIFCnt    = 0;
	INT   iIndex    = 0;
	UINT  uiIPCnt   = 0;
	
    NET_INTERFACE_REQ_S  stNetIFReqbuf[NET_INTERFACES_MAX];    
    NET_INTERFACE_CONF_S stNetIFConf; 

	DBGASSERT(NULL != pstNetIFData);

	mem_set0(&stNetIFReqbuf, NET_INTERFACES_MAX * sizeof(NET_INTERFACE_REQ_S));
	mem_set0(&stNetIFConf, sizeof(NET_INTERFACE_CONF_S));

	/* 创建socket */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (MSG_SOCKET_INVALID_FD == fd)
    {
		return 0;
	}
	
	stNetIFConf.ifc_len = sizeof(stNetIFReqbuf);	
	stNetIFConf.ifc_buf = (caddr_t)stNetIFReqbuf;	

	/* SIOCGIFCONF : 获取所有接口的清单 */
	iRet = ioctl(fd, SIOCGIFCONF, (CHAR *)&stNetIFConf);
	if (0 != iRet)	  
	{
		ERR_PRINTF("ioctl SIOCGIFCONF error!");
		return 0;
	}
	
	iIFCnt = stNetIFConf.ifc_len / sizeof(NET_INTERFACE_REQ_S);	  
    if (iIFCnt >= NET_INTERFACES_MAX) {
        return 0;
    }

	for(iIndex = 0; iIndex < iIFCnt; iIndex++)
	{	 
		/* 获取接口地址 */
		iRet = ioctl (fd, SIOCGIFADDR, (CHAR *) &stNetIFReqbuf[iIndex]);
		if (0 !=iRet)	  
		{
			ERR_PRINTF("ioctl SIOCGIFADDR error!");
			uiIPCnt = 0;
			break;	
		}

		pstNetIFData[iIndex].uiIFAddr = 
		((struct sockaddr_in*)(&stNetIFReqbuf[iIndex].ifr_addr))->sin_addr.s_addr;

		pcLocalip=(inet_ntoa(((struct sockaddr_in*)(&stNetIFReqbuf[iIndex].ifr_addr))->sin_addr)); 
		memcpy(	pstNetIFData[iIndex].szIFName, 
				&stNetIFReqbuf[iIndex].ifr_name, 
				sizeof(pstNetIFData[iIndex].szIFName));
		
		memcpy(	pstNetIFData[iIndex].szIFAddr, 
				pcLocalip, 
				sizeof(pstNetIFData[iIndex].szIFAddr));	
		NET_GetLocalMAC(pstNetIFData[iIndex].szIFName,
						pstNetIFData[iIndex].ucIFMACAddr,
						sizeof(pstNetIFData[iIndex].ucIFMACAddr));
        snprintf((CHAR *)pstNetIFData[iIndex].szIFMACAddr,
                sizeof(pstNetIFData[iIndex].szIFMACAddr), 
                NS_MAC_FORMAT,
                NS_PRINT_MAC(pstNetIFData[iIndex].ucIFMACAddr)
                );
	}  
	uiIPCnt = iIndex;
	
	close (fd);    
	return uiIPCnt;	 
} 

ULONG NET_GetLocalIPByIFName(CHAR *pcIFName, 
							 NET_IF_DATA_S *pstNetIFDat)
{
	ULONG ulRet = 0;
	UINT  uiIndex = 0;
	NET_IF_DATA_S stNetIFData[NET_INTERFACES_MAX];

	DBGASSERT(NULL != pcIFName);
	DBGASSERT(NULL != pstNetIFDat);

	mem_set0(&stNetIFData, sizeof(NET_IF_DATA_S) * NET_INTERFACES_MAX);
	ulRet = NET_GetLocalIp(stNetIFData);
	if(0 == ulRet)
	{
		return ERROR_FAILE;
	}

	for(uiIndex = 0; uiIndex < ulRet; uiIndex++)
	{
		if(0 == str_safe_cmp(stNetIFData[uiIndex].szIFName, 
 							 pcIFName, 
 							 strlen(stNetIFData[uiIndex].szIFName)))
		{
			memcpy(pstNetIFDat, &stNetIFData[uiIndex], sizeof(NET_IF_DATA_S));
			break;
		}
	}

	if (uiIndex == ulRet)
	{
		return ERROR_FAILE;
	}
	else
	{
		return ERROR_SUCCESS;
	}
	
}

UCHAR *NET_GetLocalMAC(IN CHAR *pcIFName, INOUT UCHAR *pucIFMAC, IN UINT bufLen)
{
	INT   iRet      = 0;
	INT sockfd      = MSG_SOCKET_INVALID_FD;
	NET_INTERFACE_REQ_S stNetIFReq;

	DBGASSERT(NULL != pcIFName);
	DBGASSERT(NULL != pucIFMAC);
			
	if( strlen(pcIFName) >= IFNAMSIZ)
		printf("device name is error.\n"), exit(0);
		
	strcpy(stNetIFReq.ifr_name, pcIFName);
		
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(MSG_SOCKET_INVALID_FD == sockfd)
	{
		return NULL;
	}

	//get HWaddr 
	iRet = ioctl(sockfd, SIOCGIFHWADDR, &stNetIFReq);
	if(-1 ==  iRet)
	{
		ERR_PRINTF("get HWaddr	hwaddr error");
		return NULL;
	}

	memcpy(pucIFMAC, stNetIFReq.ifr_hwaddr.sa_data, bufLen);
		
	return pucIFMAC;
}

ULONG NET_GetLocalIFNameList(IN CHAR *pcBuf, IN INT iBufLen)
{
    INT i      = 0;
    INT iP     = 0;
    INT sockfd = -1;;
    UCHAR buf[512];
    struct ifconf ifconf;
    struct ifreq *ifreq;

    DBGASSERT(NULL != pcBuf);
    DBGASSERT(0    >= iBufLen);

    /* 初始化ifconf */
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
    {
        perror("socket");
        ERR_PRINTF("socket failed");
        goto err;
    }  

    //获取所有接口信息
    if (ioctl(sockfd, SIOCGIFCONF, &ifconf) < 0) {
        ERR_PRINTF("ioctl failed");
        goto err;
    }

    //接下来一个一个的获取IP地址
    ifreq = (struct ifreq*)buf;  
    for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i>0; i--)
    {
        if(ifreq->ifr_flags == AF_INET){            //for ipv4
            MSG_PRINTF("name = [%s]\n", ifreq->ifr_name);
            iP += snprintf(pcBuf + iP, iBufLen - iP, "%s,", ifreq->ifr_name);
            ifreq++;
        }
    }

    return ERROR_SUCCESS;

err:
    if (sockfd > 0) {
        close(sockfd);
        sockfd = -1;
    }
    return ERROR_FAILE;
}
	
#if 0
int t_main(int argc,CHAR *argv[])
{
	int i = 0;
	UINT  uiIPCnt   = 0;
	u_int8_t hd[6];

	NET_IF_DATA_S stNetIFData[NET_INTERFACES_MAX];
	mem_set0(stNetIFData, NET_INTERFACES_MAX * sizeof(NET_IF_DATA_S));
	uiIPCnt = NET_GetLocalIp(stNetIFData);

	while(uiIPCnt-- > 0)
	{
		printf("name: %s\n", stNetIFData[i].szIFName);
		printf("ip: %s\n", stNetIFData[i].szIFAddr);
		i++;
	}


	//NET_GetLocalMAC(stNetIFData[i].szIFName, hd, sizeof(hd));
	NET_GetLocalMAC(stNetIFData[0].szIFName, hd, sizeof(hd));
	printf("HWaddr: %02X:%02X:%02X:%02X:%02X:%02X\n", hd[0], hd[1], hd[2], hd[3], hd[4], hd[5]);
	/**/
    exit(0);
}
#endif

/* @return:need free by caller,USE API:NET_DestroyIFInfo */
NET_IP_INFO_S *NET_GetIFInfo(VOID)
{
	//INT i = 0;
	INT iBufLen  = 0;
	//INT iDataLen = 0;
	//INT iSendLen = 0;
	UINT uiIPCnt = 0;
	//CHAR cIpInfoBuf[1024] = {0};
	
	NET_IP_INFO_S *pstNetIpInfo = NULL;
	iBufLen = sizeof(NET_IP_INFO_S) + sizeof(NET_IF_DATA_S) * NET_INTERFACES_MAX;
	pstNetIpInfo = malloc(iBufLen);
	mem_set0(pstNetIpInfo, iBufLen);
	uiIPCnt = NET_GetLocalIp(pstNetIpInfo->pstNetIFData);
	if (uiIPCnt == 0){
		free(pstNetIpInfo);
		return NULL;
	}

	pstNetIpInfo->iCnt = uiIPCnt;

    /*
	while(uiIPCnt-- > 0)
	{
		printf("name: %s\n", pstNetIpInfo->pstNetIFData[i].szIFName);
		printf("ip: %s\n", pstNetIpInfo->pstNetIFData[i].szIFAddr);
		iDataLen += snprintf(cIpInfoBuf + iDataLen, sizeof(cIpInfoBuf),"name:%s\nIP:%s\n", 
			pstNetIpInfo->pstNetIFData[i].szIFName,
			pstNetIpInfo->pstNetIFData[i].szIFAddr 
		);

		i++;
	}

	free(pstNetIpInfo);
    pstNetIpInfo = NULL;
    */

	return pstNetIpInfo;
}
VOID NET_DestroyIFInfo(NET_IP_INFO_S *pstIpInfo)
{
    free(pstIpInfo);
    pstIpInfo = NULL;

    return;
}


ULONG TimerCB_SentIPinfo(INT iTimerId, VOID *arg)
{
	INT    iConnFd = -1;
	ULONG *pulPara = NULL;

	pulPara = (ULONG *)arg;
	iConnFd = pulPara[0];
	//SentIPInfo(iConnFd);

	return ERROR_SUCCESS;
}

