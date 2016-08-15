#include "stdio.h"

#include "stdlib.h"
#include "string.h"

#include "net/if.h"
#include "arpa/inet.h"
#include "linux/sockios.h"
#include <sys/ioctl.h>

char* GetLocalIp()  
{        
    int MAXINTERFACES=16;  
    char *ip = NULL;  
    int fd, intrface, retn = 0;    
    struct ifreq buf[MAXINTERFACES];    
    struct ifconf ifc;    
	u_int8_t hd[6];

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)    
    {    
        ifc.ifc_len = sizeof(buf);    
        ifc.ifc_buf = (caddr_t)buf;    
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))    
        {    
            intrface = ifc.ifc_len / sizeof(struct ifreq);    

            while (intrface-- > 0)    
            {    
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))    
                {    
                    ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr)); 
					printf("name: %s\n", &buf[intrface].ifr_name);
					printf("ip: %s\n", ip);
                    break;  
                }                        
            }  
        }    
        close (fd);    
        return ip;    
    }  
} 

int main(int argc,char *argv[])
{
    struct sockaddr_in *addr;
    struct ifreq ifr;
    char*address;
    int sockfd;

	GetLocalIp();

printf("******************************\na");

    char *name = "p3p1";
    if( strlen(name) >= IFNAMSIZ)
        printf("device name is error.\n"), exit(0);
        
    strcpy( ifr.ifr_name, name);
        
    sockfd = socket(AF_INET,SOCK_DGRAM,0);

    //get inet addr
    if( ioctl( sockfd, SIOCGIFADDR, &ifr) == -1)
        printf("get inet addr ioctl error.\n"), exit(0);

    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    address = inet_ntoa(addr->sin_addr);

    printf("inet addr: %s\n",address);

    //get Mask
    if( ioctl( sockfd, SIOCGIFNETMASK, &ifr) == -1)
        printf("get Mask ioctl error.\n"), exit(0);

    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    address = inet_ntoa(addr->sin_addr);

    printf("Mask: %s\n",address);

    //get HWaddr 
    u_int8_t hd[6];
    if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1)
        printf("get HWaddr  hwaddr error.\n"), exit(0);

    memcpy( hd, ifr.ifr_hwaddr.sa_data, sizeof(hd));
    printf("HWaddr: %02X:%02X:%02X:%02X:%02X:%02X\n", hd[0], hd[1], hd[2], hd[3], hd[4], hd[5]);
    
    exit(0);
}

