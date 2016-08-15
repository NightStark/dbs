#ifndef __RAND_TABLE_H__
#define __RAND_TABLE_H__

#define RAND_TABLE_FILE_NAME "rand_table.dat"
#define RAND_TABLE_MOD           (0xFFFFFFFF)
#define RAND_TABLE_OFF_MOD       (32 - 10 - 1)
#define RAND_TABLE_TEST_MOD		 (1024)



UINT *RAND_TABLE_Get(VOID);
ULONG RAND_TABLE_Init(VOID);
VOID  RAND_TABLE_Fint(VOID);
VOID  RAND_table_create(IN UINT uiTableLen);
#endif //__RAND_TABLE_H__
