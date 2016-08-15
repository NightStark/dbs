#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

int t_gethostent(void)
{
    int fd;
    int connfd;
    int i =0;
	struct sockaddr_in servaddr;
    struct hostent *hostMSg = 0;
    unsigned char hostName[INET_ADDRSTRLEN];
    printf("gethostent************************************************\n");
    while(1)
    {
        hostMSg = gethostent();
        if(NULL == hostMSg)
            break;
        printf(" name of host:%s \n", hostMSg->h_name);

        while(NULL != hostMSg->h_aliases[i])
        {
            printf(" aliases name[%d]:%s \n", i, hostMSg->h_aliases[i]);
            i++;
        }
        i = 0;
        printf(" Addr Type:"); 
        switch(hostMSg->h_addrtype)
        {
            case AF_INET:
                printf("AF_INET\n");
                break;
            case AF_INET6:
                printf("AF_INET6\n");
                break;
        }
        printf(" Addr length: %d bytes\n", hostMSg->h_length);
        while(NULL != hostMSg->h_addr_list[i])
        {
            printf(" Address :%s \n", inet_ntoa(*((struct in_addr *)hostMSg->h_addr_list[i])));
            i++;
        }
        printf("******************************************************\n");
    }
    endhostent();
    return 0;
}

int t_getnetent(void)
{
	struct netent *pstNetEnt = NULL;
	int i = 0;
	pstNetEnt = getnetent();
	printf(" net name: %s\n", pstNetEnt->n_name);
	while(NULL != pstNetEnt->n_aliases[i]){
		printf("net n_aliases[%d]: %s\n",i, pstNetEnt->n_aliases[i]);
	}
	
	printf(" net Addr Type:"); 
	switch(pstNetEnt->n_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
			break;
		case AF_INET6:
			printf("AF_INET6\n");
			break;
	}
	
	printf(" net NO.: %lu\n", pstNetEnt->n_net);
	endnetent();
	return 0;
}

int t_getprotoent(void)
{
	struct protoent *pstProEnt = NULL;
	pstProEnt = getprotoent();
	printf(" proto name: %s\n", pstProEnt->p_name);	
	printf(" proto NO.: %d\n",  pstProEnt->p_proto);
	return 0;
}

int t_getserver(void)
{
	struct servent *pstServEnt = NULL;
	int i = 0;
	int iServIndex = 0;

	printf("-------------------------\n");
	while(1){
		iServIndex++;
		printf(" server NO.%d\n", iServIndex);
		pstServEnt = getservent();
		if(NULL == pstServEnt)
		{
			break;
		}
		printf(" server name :%s\n", pstServEnt->s_name);
		i = 0;
		while(NULL != pstServEnt->s_aliases[i])
		{
			printf(" server aliases [%d] : %s\n",i , pstServEnt->s_aliases[i]);
			i++;
		}
		printf(" server port : %d \n", pstServEnt->s_port);
		printf(" server ptoto : %s \n", pstServEnt->s_proto);		
		printf("-------------------------\n");
	}
	endservent();
	
	return 0;
}
int main(void)
{
    char c = 0;
    getchar();
    t_gethostent();
	t_getnetent();
	t_getprotoent();
	t_getserver();
    return 0;
}
