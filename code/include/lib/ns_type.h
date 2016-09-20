#ifndef __TYPE_H__
#define __TYPE_H__
/*

int和long的区别
int : 4 bytes
long : =  sizeof(void *)
*/

typedef char				CHAR; 			/* 1 */
typedef unsigned char 		UCHAR; 			/* 1 */
typedef	short 				SHORT;			/* 2 */
typedef	unsigned short		USHORT; 		/* 2 */
typedef int					INT;			/* 4 */	 
typedef unsigned int		UINT;			/* 4 */
typedef long int 			LONG;			/* x */
typedef unsigned long int	ULONG;			/* x */
typedef long long 			LLONG;			/* 8 */
typedef unsigned long long	ULLONG;			/* 8 */
typedef float				FLOAT;			/* 4 */
typedef double				DOUBLE;			/* 8 */

#if __WORDSIZE == 64
typedef unsigned long int	UINT64;        /* 8 */
#else
__extension__   /* 与标准不符时编译器不报警号什么 */
typedef unsigned long long int	UINT64;
#endif

typedef UCHAR			    BOOL_T;			/* 1 */

typedef CHAR                BIT_T;			/* 1 */

#ifndef __TIME_T
#define __TIME_T     		 /* 避免重复定义 time_t */
typedef long     TIME_T;    /* 时间值time_t 为长整型的别名*/
#endif

/* sparc 32 bit */

typedef UINT         	    SIZE_T;
typedef INT                 SSIZE_T;

#define BOOL_TRUE			(1)
#define BOOL_FALSE			(0)

#define BIT_0				(0)
#define BIT_1				(1)


/* 特殊类型 */
typedef ULONG				TYPEID;			/* 自定数据类型id */
typedef UCHAR				STTYPED;


#ifndef	TABLE_NAME_LEN_MAX
#define TABLE_NAME_LEN_MAX 32
#endif //TABLE_NAME_LEN_MAX

#ifndef TABLE_ELEMENT_NAME_LEM_MAX
#define TABLE_ELEMENT_NAME_LEM_MAX 32
#endif //TABLE_ELEMENT_NAME_LEM_MAX

#ifndef	TABLE_ELEMENT_NUM_MAX
#define TABLE_ELEMENT_NUM_MAX 32
#endif //TABLE_ELEMENT_NUM_MAX

#ifndef TABLE_NUM_MAX
#define TABLE_NUM_MAX (1000 * 1000)  /* 100W个表，100W个链表 */
#endif //TABLE_NUM_MAX

#ifndef TYPE_ID_BASE
#define TYPE_ID_BASE ((0xFF) << 8)
#endif //TYPE_ID_BASE

#ifndef TABLE_FD_INVALID
#define TABLE_FD_INVALID (TABLE_NUM_MAX + 0xFF)
#endif //TABLE_FD_INVALID

/* 没有 offsetof container_of 会挂掉(这个在内核里里面都有定义的) */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#ifndef container_of
#define container_of(ptr, type, member) ({ \
const typeof( ((type *)0)->member ) *__mptr = (ptr); \
(type *)( (char *)__mptr - offsetof(type,member) );})
#endif


/* 自定义类型拆分与获得用的 */
typedef struct st_table_types_tag
{
	ULONG 	    ulSTLong;		/* 数据总长度(Bytes) */

	UCHAR		ucTypeCount;    /* 元素个数 */
	UCHAR	   *pucTypeMap;		/* 元素类型 */
	ULONG	   *pucTypeOffset;	/* 元素偏移，便于快速查找 */
	UCHAR	  **pucTypeName;	/* 元素名称 */
	ULONG	   *pulTypeLen;		/* 元素长度(Bytes),便于在复制时去掉类型 */
	
}TABLE_TYPES_S;



typedef enum any_type{
	AT_NONE = 0,
	AT_CHAR,
	AT_UCHAR,
	AT_SZ,
	AT_SHORT,
	AT_USHORT,
	AT_INT,
	AT_UINT,
	AT_LONG,
	AT_ULONG,
	AT_LLONG,
	AT_ULLONG,
	AT_FLOAT,
	AT_DOUBLE,
	AT_BOOL_T
}ANY_TYPE_EN;

#endif /* __TYPE_H__ */
