#include <linux/string.h>
#include "string.h"

/* 在@src中查找@d 
 * @src可能不是字符串(没有'\0'), 但@d一定要是个字符串(以'\0'结尾)  
 * @max 指定@src的长度。
 * */
char *__strstr2(const char *src, const char *d, int max)
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
const char * __get_msg_of_A2B(const char *tt, int tt_len, 
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
        p = __strstr2(tt, a, tt_len);
    } else {
        a = tt;
        p = tt;
    }
    skip_b_len = strlen(b);

    //p = __strstr2(tt, a, tt_len);
    if (NULL != p) {
        p  += skip_a_len; /* skip "<" */
        len = tt_len - skip_a_len;
        p1 = p;
        p = __strstr2(p1, b, len);
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
const char * __get_msg_of_P2B(const char *tt, int tt_len, 
                                        char *buf, int buf_len, 
                                        const char *b)
{
    return __get_msg_of_A2B(tt, tt_len, buf, buf_len, NULL, b);
}

const char * __get_msg_of_ltgt(const char *tt, int tt_len, char *buf, int buf_len)
{
    return __get_msg_of_A2B(tt, tt_len, buf, buf_len, "<", ">");
}


/*
 * */

int __get_str_pos_of_A2B(const char *str, int strlen, STR_A2B_INFO_ST *pstStrA2BInfo)
{
    const char *posA = NULL;
    const char *posB = NULL;

    posA = __strstr2(str, pstStrA2BInfo->strA, strlen);
    if (posA == NULL) {
        return -1;
    }

    posB = __strstr2(posA, pstStrA2BInfo->strB, strlen);
    if (posB == NULL) {
        return -1;
    }

    pstStrA2BInfo->posA = posA;
    pstStrA2BInfo->posB = posB;

    pstStrA2BInfo->A2BLen = posB - posA;
    UP_MSG_PRINTF("A:%x,B:%x, len:%d", (unsigned int)posA,
            (unsigned int)posB, (posB-posA));

    return 0;
};

/*
 * @hdr_filed : name of http header-field
 * @pstA2B: output, struct of header-field str
 * */
int _get_http_header_filed(const char *str, 
                                         int strlen, 
                                         const char *hdr_filed, 
                                         STR_A2B_INFO_ST *pstA2B)
{
    pstA2B->strA = hdr_filed;
    pstA2B->strB = "\r\n";

    if (__get_str_pos_of_A2B(str, strlen, pstA2B) < 0) {

        return -1;
    }

    return 0;
}

/* find @str in pstA2B
 * */
char * __A2B_strstr(STR_A2B_INFO_ST *pstA2B, const char *str)
{
    char *p = NULL;

    p  = __strstr2(pstA2B->posA, str, pstA2B->A2BLen);

    return p;
}
