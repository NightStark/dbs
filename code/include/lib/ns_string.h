#ifndef __STRING_C_H__
#define __STRING_C_H__

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define NS_MAC_FORMAT "%02X:%02X:%02X:%02X:%02X:%02X"
#define NS_PRINT_MAC(m) \
    (unsigned int)m[0], \
     (unsigned int)m[1], \
     (unsigned int)m[2], \
     (unsigned int)m[3], \
     (unsigned int)m[4], \
     (unsigned int)m[5]

ULONG mem_set(VOID *p, UCHAR c, ULONG size);
ULONG mem_set0(VOID *p, ULONG size);
#define CLEAR_STRUCT(s) \
	mem_set0((&(s)), sizeof(s))
VOID *mem_alloc(ULONG size);
ULONG str_safe_cpy(char *dest, const char *src, unsigned long len);
INT str_safe_cmp(const CHAR *str1, const CHAR *str2, unsigned long str1Len);
INT mem_safe_cmp(const VOID *p1, const VOID *p2, unsigned long ulPLen);

char * ctohexs(char c);
char * itohexs(unsigned int num);
void mem_print(unsigned int * memptr,unsigned int len);
char is_all_num (char * puc);
int get_char_pos(const char * s, char c, int n);
char *stripwhite (char *string);

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif
#ifndef _isblank
#define _isblank(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n') || ((c) == '\r'))
#endif

#endif //__STRING_C_H__

