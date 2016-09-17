#ifndef __STRING_C__
#define __STRING_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

ULONG mem_set(VOID *p, UCHAR c, ULONG size)
{
	ULONG index = 0;
	UCHAR *puc = (UCHAR *)p;
	for(index = 0; index < size; index++, puc++)
	{
		if(NULL == puc)
		{
			ERR_PRINTF("point is a (NULL) value!");
			return ERROR_PONIT_IS_NULL;
		}
		*puc = c;
	}
	return ERROR_SUCCESS;
}

ULONG inline mem_set0(VOID *p, ULONG size)
{
	return mem_set(p, 0, size);
}

/* alloc mem set 0 */
/* 改成宏，使得调用者可以退出当前函数，并报错 */
#ifdef mem_alloc
undef mem_alloc
#endif

VOID * mem_alloc(ULONG size)
{
 	VOID *p = NULL;

	p = malloc(size);
	
	if(NULL == p)
	{
		ERR_PRINTF("Memory is not enough!");
		return NULL;
	}
	
	memset(p, 0, size);
	
	return p;
}

ULONG str_safe_cpy(char *dest, const char *src, unsigned long len)
{
	INT i = 0;

	DBGASSERT(NULL != dest);
	DBGASSERT(NULL != src);
	while(i < len)
	{
		if(0 == src[i])
		{
			break;
		}
		dest[i] = src[i];
		i++;
	}
	return i;
}


/*
	str1Len 要带 '\0'
	str1     xxxxxx   xxxxxx   xxxxxx
	str2     xxxxxx	  xxxxxxx  xxxx
	cmp		 =        <        >
	
*/
INT str_safe_cmp(const CHAR *str1, const CHAR *str2, unsigned long str1Len)
{
	INT i = 0;
	DBGASSERT(NULL != str1);
	DBGASSERT(NULL != str2);
	for(i = 0; i < str1Len; i++)
	{
		if(str1[i] == str2[i] && str1[i] != 0 && str2[i] != 0)
		{
			continue;
		}
		else
		{
			return str1[i] - str2[i];
		}
	}
	return 0;
}

INT mem_safe_cmp(const VOID *p1, const VOID *p2, unsigned long ulPLen)
{
	INT i = 0;
	CHAR *pc1 = NULL; 
	CHAR *pc2 = NULL;
		
	if(NULL == p1)
	{
		ERR_PRINTF("First parameter p1 is NULL!");
		return -1;
	}
	if(NULL == p2)
	{
		ERR_PRINTF("First parameter p2 is NULL!");
		return -1;
	}

	pc1 = (CHAR *)p1;
	pc2 = (CHAR *)p2;
	
	for(i = 0; i < ulPLen; i++)
	{
		if(pc1[i] == pc2[i])
		{
			continue;
		}
		else
		{
			return pc1[i] - pc2[i];
		}
	}
	
	return 0;
}



//数字转字符串10进制
#define NUM_S_LEN 12
char num_s[NUM_S_LEN];
char * itos(int num){
    mem_set(num_s,0x0,NUM_S_LEN);
    int i = 0;
    num_s[NUM_S_LEN - 1] = '\0';
    if(num == 0){
        num_s[NUM_S_LEN - 2] = '0';
    }
    while(num){
        num_s[NUM_S_LEN - i - 2] = num % 10 + '0';
        num /= 10;
        i++;
    } 
    return num_s + (NUM_S_LEN - 1 - i);
}

//问题，字符串存在全局变量中容易，多个同时使用会被有查掉的危险
//需要修该
//数字转字符串16进制
#define NUM_S_HEX_LEN 11 
char num_s_hex[NUM_S_HEX_LEN];
char * itohexs(unsigned int num){
    mem_set(num_s_hex,0x0,NUM_S_HEX_LEN);
    num_s_hex[0] = '0';
    num_s_hex[1] = 'x';
    num_s_hex[NUM_S_HEX_LEN - 1] = '\0';

    char n_b = 0;
    int i = 0;
    while(i < 8){
        n_b = (num & 0xF);
        if(n_b <= 9 ){
            num_s_hex[NUM_S_HEX_LEN - i - 2] = n_b + '0';
        }else{
            num_s_hex[NUM_S_HEX_LEN - i - 2] = (n_b - 10) + 'A';
        }
        num = num >> 4;
        i++;
    } 
    return num_s_hex;
}

char char_hex[2];
char * ctohexs(char c){
    //0x00000088;
    return itohexs((unsigned int)c) + 8;
}

//字符串转换成整数
//只对大于零的有效
int stoi(char *strptr){
    int num = 0;
    while(*strptr == ' ')strptr++;
    while(*strptr != '\0'){
        if(*strptr > (9 + '0')){
            printf("Errrr in function [stoi]\n");
            return -1;
        }
        num *= 10;
        num += *strptr - '0';
        strptr++;
    }
    return num;
}

//hex字符转整数
unsigned int shextoi(char *strptr){
    unsigned int num = 0;
    int len = 0;
    char c = 0;
    if((*strptr == '0') && ((*(strptr + 1) == 'x') || (*(strptr + 1)) == 'X')){
        strptr += 2;
        while(len < 8){
            if(*strptr == '\0'){
                num = num >> ((8 - len) * 4); 
                break;
            }
            if(*strptr >= '0' && *strptr <= '9'){
                c = *strptr - '0';
            }else if(*strptr >= 'A' && *strptr <= 'F'){
                c = *strptr - 'A' + 0xA;
            }else if(*strptr >= 'a' && *strptr <= 'f'){
                c = *strptr - 'a' + 0xA;
            }else{
                printf("ERROR:can't trans string hex to int!\n");
                return 0;
            }
            num = num | ((c & 0xFF) << (4 * (8 - len - 1)));
            strptr++;
            len++;
        } 
        return num;
    }else{
        printf("ERROR:Format is wrong!\n");
        return 0;
    }
}

//内存打印 16进至
void mem_print(unsigned int * memptr,unsigned int len){
    int i = 0;
    char buf[512];
    //转为字符指针 
    unsigned char * c_memptr = (unsigned char *)memptr;
    printf("------------------------------------------------------------------------------\n");
    printf("  offset | 00 01 02 03 04 05 06 07  08 09 10 11 12 13 14 15  ........ ........\n");
    printf("------------------------------------------------------------------------------\n");
    printf("0x00000000 ");
    while(i < len){
    	memset(buf, 0, sizeof(buf));
        do{
			sprintf(buf + i, "%s", ctohexs(*(c_memptr + i)));
			printf(ctohexs(*(c_memptr++)));
			sprintf(buf + 50 + i, "%s", ctohexs(*(c_memptr + i)));
			printf(" ");
			
        	if (i % 7 == 0 && i != 0){
				sprintf(buf + i, "%s", " ");
				printf(" ");
        	}
        }while((i++ % 15) != 0 && i < len);
        
        /*
        printf(" ");
        if(i % 8 == 0){
            printf(" ");
        }
        */
        if(i != 1){
            printf("\n");
            printf(itohexs(i));
            printf(" ");
			i++;
        }
    }
    printf("\n------------------------------------------------------------------------------\n");
    
	printf(buf);
}

int str_pois_find(CHAR *cBuf, const CHAR *cFind, int n)
{
	int iCmpRet = 0;
	CHAR *pBuf =NULL;
	int iFindInex = 1;
	
	pBuf = cBuf;

	while(1)
	{ 
		iCmpRet = str_safe_cmp(pBuf,cFind,strlen(cFind));
		
		if (0 == iCmpRet)
		{
			if (iFindInex == n)
			{
				return (pBuf - cBuf);	
			}
			iFindInex++;
		}

		pBuf++;
		
		if ((pBuf - cBuf) > strlen(cBuf))
		{
			return -1;
		}
	}
	
	return -1;
}

int mem_pois_find( VOID *pBuf, 
						int iBufLen,
					    VOID *pFind, 
						int iFindLen,
						int n)
{
	int iCmpRet = 0;
	CHAR *pcBufTmp =NULL;
	int iFindInex = 1;
	
	pcBufTmp = (CHAR *)pBuf;

	if (iFindLen > iBufLen)
	{
		return -1;
	}

	while((pcBufTmp - (CHAR *)pBuf) <= iBufLen)
	{
		iCmpRet = mem_safe_cmp(pcBufTmp,pFind,iFindLen);
		
		if (0 == iCmpRet)
		{
			if (iFindInex == n)
			{
				return (pcBufTmp - (CHAR *)pBuf);	
			}
			iFindInex++;
		}

		pcBufTmp++;
	}
	
	return -1;
}

char is_all_num (char * puc)
{
	while ('\0' != *puc)
	{
		if(*puc < '0' || *puc > '9')
			return 0;
		puc++;
	}
	
	return 1;
}

/*
	@ s : the string
	@ c : then char want to get pos
	@ n :  != 0 , 限制查找的长度
		   == 0 , 对字符串查找没有长度限制，但要保证有'\0'结尾符号。
*/
int get_char_pos(const char * s, char c, int n)
{
	const char *p = NULL;

	DBGASSERT(s != NULL);

	p = s;
	do{
		if (*p == c)
		{
			return p - s;
			break;
		}
		p++;
	}while(*p != '\0' || (n > 0 ? (p - s) <= n : 0));

	return -1;
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
 *    into STRING. */
char *
stripwhite (char *string)
{
	register char *s, *t;

	DBGASSERT(NULL != string);

	for (s = string; _isblank (*s); s++)
		;

	if (*s == 0)
		return (s);

	t = s + strlen (s) - 1;
	while (t > s && _isblank (*t))
		t--;
	*++t = '\0';

	return s;
}
/* 在@src中查找@d
 * @src可能不是字符串(没有'\0'), 但@d一定要是个字符串(以'\0'结尾)
 * @max 指定@src的长度。
 * */
char *ns_strstr2(const char *src, const char *d, int max)
{
    int l2;
    char *s = (char*)src;
    int i = 0;

    l2 = strlen(d);
    if (!l2)
        return (char *)s;
    while(*s && (s - src) <= (max - l2)){
        for(i = 0; i < l2; i ++){
            if(s[i] != d[i])
                break;
        }
        if(i == l2)
            return s;
        s ++;
    }
    return NULL;
}

/* copy @tt中@a之后@b之前的数据到@buf中，返回@b后的地址
 * 如果@a == NULL:则copy从@tt到@b
 * */
const char * ns_get_msg_of_A2B(const char *tt, int tt_len,
                                        char *buf, int buf_len,
                                        const char *a,
                                        const char *b)
{
    int len = 0;
    int str_len = 0;
    const char *p = NULL, *p1 = NULL;
    int skip_a_len = 0;
    int skip_b_len = 0;

    if (NULL == tt  || tt_len  <= 0 ||
        NULL == buf || buf_len <= 0 ||
        NULL == b) {
        return NULL;
    }

    if (NULL != a) {
        skip_a_len = strlen(a);
        p = ns_strstr2(tt, a, tt_len);
    } else {
        a = tt;
        p = tt;
    }
    skip_b_len = strlen(b);

    //p = ns_strstr2(tt, a, tt_len);
    if (NULL != p) {
        p  += skip_a_len; /* skip "<" */
        len = tt_len - skip_a_len;
        p1 = p;
        p = ns_strstr2(p1, b, len);
        if (NULL != p) {
            str_len = ((p - p1) >= buf_len) ? (buf_len - 1) : (p - p1);
            strncpy(buf, p1, str_len);
            buf[str_len + 1] = 0;
            return p + skip_b_len; /* skip '>'*/
        }
    }
    return NULL;
}

/* copy @tt中@tt之后@b之前的数据到@buf中，返回@b后的地址  */
const char * ns_get_msg_of_P2B(const char *tt, int tt_len,
                                        char *buf, int buf_len,
                                        const char *b)
{
    return ns_get_msg_of_A2B(tt, tt_len, buf, buf_len, NULL, b);
}

const char * ns_get_msg_of_ltgt(const char *tt, int tt_len, char *buf, int buf_len)
{
    return ns_get_msg_of_A2B(tt, tt_len, buf, buf_len, "<", ">");
}
#endif //__STRING_C__

