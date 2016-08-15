#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_base.h>

#include <ns_web_server.h>

ULONG WEB_http_ReadReqFile(IN CHAR *pucFilePath, 
 						   IN CHAR *pcFileBuf, 
 						   INOUT ULONG *pulReqFileSize)
{
	INT fd;
	INT len;
	
	/* open file */
	fd = open(pucFilePath, O_RDONLY);
	if(-1 == fd)
	{
		ERR_PRINTF("Open request file :%s failed!", pucFilePath);
		return ERROR_FAILE;
	}

	len = read(fd, pcFileBuf, *pulReqFileSize);
	if(len != *pulReqFileSize)
	{
		ERR_PRINTF("Reqfile is too long than Bufer!");
		return ERROR_FAILE;
	}		
	
	close(fd);

	return ERROR_SUCCESS;

}


