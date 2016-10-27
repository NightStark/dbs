#define DEV_ARCH_MIPS

#ifdef DEV_ARCH_XX86
#include <execinfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tl.h"

#define LEN 128
#define FILENAME "stack" 

static void
__dump_data(unsigned char *ptr, int len, const char *func, int line)
{
    int i;
    printf("\n =============dump=============\n");
    printf("\n dump start:0x%x, len:%d\n", (unsigned int)ptr, len);
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n ==============================\n");
}

#define __DUMP_MEM__(p, len) \
    __dump_data(p, len, __func__, __LINE__)

void fun1();
void fun2();
void fun3();

void print_stacktrace();
int __backtrace(void **array, int size);

#ifdef DEV_ARCH_MIPS
#define _backtrace(array, size) \
	__backtrace((array), (size))
#else
#define _backtrace(array, size) ((int)0);
#endif

int main()
{
    fun3();

    return 0;
}

/********
void *buffer[LEN] = {0};  
void fun1()
{
    int h = 0;
    h = _backtrace(buffer, LEN);
    printf("H:0x%x\n", h);
    while (h-- > 0) {
        printf("0x:%x\n", (unsigned int)(unsigned long)buffer[h]);
    }

}
*/

void fun2()
{
    fun1();
}

void fun3()
{
    fun2();
}


#ifdef DEV_ARCH_XX86
void print_stacktrace()
{
    int size = 16;
    int i;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);
    for (i = 0; i < stack_num; ++i)
    {
        printf("%s\n", stacktrace[i]);
    }
    free(stacktrace);
}
#endif

#ifdef DEV_ARCH_MIPS
//#pragma GCC push_options //for VC++
//#pragma GCC optimize ("O0")
static  unsigned long /*__attribute__((optimize("O0")))*/  __getpc(void) 
{
    unsigned long rtaddr;

    __asm__ volatile ("move %0, $31" : "=r"(rtaddr));

    return rtaddr;
}
//#pragma GCC pop_options

/*
 * 00400920 <__backtrace>:                          
 * 400920:   27bdffb8    addiu   sp,sp,-72
 * 400924:   afbf0044    sw  ra,68(sp)
 * 400928:   afbe0040    sw  s8,64(sp)
 * 40092c:   03a0f021    move    s8,sp
 *
 */
/*
 *
 * 但是像下面这种不会返回的函数，编译器就不会记录其返回地址到堆栈
void fun1()
{
    int h = 0;
    while(1) {
    }
}
其反汇编汇编为
00000720 <fun1>:
 720:	27bdffe8 	addiu	sp,sp,-24
 724:	afbe0014 	sw	s8,20(sp)
 728:	03a0f021 	move	s8,sp
 72c:	afbc0000 	sw	gp,0(sp)
 730:	afc00008 	sw	zero,8(s8)
 734:	1000ffff 	b	734 <fun1+0x14>
 738:	00000000 	nop
 73c:	00000000 	nop
 * */

/*
 *
void fun1()
{
    int h = 0;
    //如果这里有代码，就会有
    //400924:   afbf0044    sw  ra,68(sp)
    //就有返回地址了。
    while(1) {
        break;
    }
}
其反汇编汇编为
 * */

int __backtrace(void **array, int size)  
{  
    int i = 0;  
    int ii = 0;
    short slen = 0;
    int stack_len = 0;
    int high = 0;
    int stack_max = 4096;
    unsigned long int ra = 0;   //=eip
    unsigned long int *p = &ra;
    unsigned long int sp = 0;
    unsigned long int pc = 0;
    unsigned long int _instruct = 0;
    //char asm_cmd[128];
    unsigned long int reg = 0;

    if (array == NULL || size <= 0) {
        printf("invalid parameters.\n");
        return -1;
    }

    high ++;
    array[high] = __backtrace;

    __asm__ volatile (  
            "lw  $8, %0\t\n"
            "sw  $31,($8)\t\n" // ra -> *p
            :  /* output register */
            :"m" (p)     /* output register */
            :"memory"   /* cloberred register */
            );

    printf("ra:[%x]\n", 
            (unsigned int)ra);

#if 0
    __asm__ volatile (  
            "lw  $8, %0\t\n"
            "sw  $29,($8)\t\n" //sp -> *p
            :  /* output register */
            :"m" (p)     /* output register */
            :"memory"   /* cloberred register */
            );
#endif
    //snprintf(asm_cmd, sizeof(asm_cmd), "move  %0, $%d\t\n", 29);

    /////////////////////////////////////////////////////////////
    __asm__ volatile (  
            "move  %0, $0\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg0:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $1\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg1:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $2\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg2:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $3\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg3:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $4\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg4:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $5\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg5:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $6\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg6:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $7\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg7:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $8\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg8:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $9\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg9:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $10\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg10:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $11\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg11:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $12\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg12:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $13\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg13:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $14\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg14:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $15\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg15:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $16\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg16:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $17\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg17:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $18\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg18:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $19\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg19:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $20\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg20:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $21\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg21:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $22\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg22:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $23\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg23:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $24\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg24:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $25\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg25:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $26\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg26:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $17\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg27:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $28\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg28:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $29\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg29:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $30\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg30:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    __asm__ volatile (  
            "move  %0, $31\t\n" 
            :"=r" (reg)     /* output register */
            );
    printf("%s [%d] reg31:%x\n", __func__, __LINE__,
            (unsigned int)reg);
    
    

    unsigned long input_d = 0xa5a50110;
    __asm__ volatile (  
            "lw  $15, %1\t\n" 
            //"move  %0, $15\t\n" 
            :"=r" (reg)     /* output register */
            :"m" (input_d)
            );
    __asm__ volatile (  
            "move  %0, $15\t\n" 
            :"=r" (reg)     /* output register */
            );
    
    printf("%s [%d] reg15:%x\n", __func__, __LINE__,
            (unsigned int)reg);

    /////////////////////////////////////////////////////////////

    __asm__ volatile (  
            "move  %0, $29\t\n" //sp -> *p
            :"=r" (sp)     /* output register */
            );
    pc = (unsigned long)__getpc();
    printf("%s [%x] sp:%x\n", __func__,
            (unsigned int)pc, (unsigned int)sp);

    __DUMP_MEM__((void *)(sp - 16), 16);



    for (i = 0; i < size; i++) {
        /* find instruct like "400920:   27bdffb8    addiu   sp,sp,-72"
         * "27bdffb8"中后16位为sp偏移的值, 栈都是从高地址向低地址扩展，因此为负值。
         * */
        ii = 0;
        while(1) {
            _instruct = (*(unsigned long *)pc);
            /*
               printf("ii = %x, pc = %x, instruct:%x\n", 
               (unsigned int)(ii * 4), (unsigned int)pc, (unsigned int)_instruct);
               */
            if ((( _instruct & 0xFFFF0000) == 0x27bd0000) || (ii++ > stack_max)) { //FIXME:1024 maybe a good value
                break;
            }
            pc -= 4;
        }

        if (ii >= stack_max) {
            //FIXME: all exit in here;?
            printf("invalid instruct or stack.\n");
            goto exit;
        }


        /* get stack len of this function */
        slen = (_instruct & 0xFFFF); /* a negative num, so use short can get sign */
        stack_len = (slen < 0) ? (-slen) : slen;

#if 0
        printf("ii*4 = %x, pc = %x, %x, stack len=%d\n", 
                (unsigned int)(ii * 4), (unsigned int)pc, (unsigned int)_instruct, stack_len);
#endif

        /* point to prev stack button */
        sp += stack_len;  
        /* the first instruct is : save ra to sp, and save ra to the top of this function stack
         * so: ra = *(sp + stack_len - 4)
         * */

        /* get (ra), return address, 
         * pc porint to the return address , return to the caller function */
        ra = sp - 4;
        pc = (*(unsigned long *)(unsigned long)(ra)); 

        printf("sp1:[%x]\n", (unsigned int)((unsigned long)(ra))); 
        printf("pc:[%x]\n", (unsigned int)(pc)); 

        if (pc == 0 || stack_len == 0) {
            break;
        }

        high++;
        if (high < size) {
            array[high] = (void *)pc;
        } else {
            printf("size is too small.\n");
            break;
        }
    }

    int tval = 0;
    tval = input_d;
    printf("%s %d tval addr :0x%x., tval = 0x%x\n", __func__, __LINE__, (unsigned int)(&tval), (unsigned int)tval);

exit:
    while (1) {
        tval = input_d;

        /*
        __asm__ volatile (  
                "lw  $15, %1\t\n" 
                :"=r" (reg)     
                :"m" (input_d)
                );
                */
    }
    

    return high;  
}  
#endif

int __backtrace_run_proc(void **array, int size, 
        unsigned long int sp,
        unsigned long int pc)  
{  
    int i = 0;  
    int ii = 0;
    short slen = 0;
    int stack_len = 0;
    int high = 0;
    int stack_max = 4096;
    unsigned long int ra = 0;   //=eip
    unsigned long int *p = &ra;
    //unsigned long int sp = 0;
    //unsigned long int pc = 0;
    unsigned long int _instruct = 0;

    if (array == NULL || size <= 0) {
        printf("invalid parameters.\n");
        return -1;
    }

    high ++;
    array[high] = __backtrace;

    __asm__ volatile (  
            "lw  $8, %0\t\n"
            "sw  $31,($8)\t\n" // ra -> *p
            :  /* output register */
            :"m" (p)     /* output register */
            :"memory"   /* cloberred register */
            );

    printf("ra:[%x]\n", 
            (unsigned int)ra);

    //pc = (unsigned long)__getpc();
    printf("%s pc:[%x] sp:%x\n", __func__,
            (unsigned int)pc, (unsigned int)sp);

    for (i = 0; i < size; i++) {
        /* find instruct like "400920:   27bdffb8    addiu   sp,sp,-72"
         * "27bdffb8"中后16位为sp偏移的值, 栈都是从高地址向低地址扩展，因此为负值。
         * */
        ii = 0;
        while(1) {
            _instruct = (*(unsigned long *)pc);
            /*
               printf("ii = %x, pc = %x, instruct:%x\n", 
               (unsigned int)(ii * 4), (unsigned int)pc, (unsigned int)_instruct);
               */
            if ((( _instruct & 0xFFFF0000) == 0x27bd0000) || (ii++ > stack_max)) { //FIXME:1024 maybe a good value
                break;
            }
            pc -= 4;
        }

        if (ii >= stack_max) {
            //FIXME: all exit in here;?
            printf("invalid instruct or stack.\n");
            return high;
        }


        /* get stack len of this function */
        slen = (_instruct & 0xFFFF); /* a negative num, so use short can get sign */
        stack_len = (slen < 0) ? (-slen) : slen;

#if 0
        printf("ii*4 = %x, pc = %x, %x, stack len=%d\n", 
                (unsigned int)(ii * 4), (unsigned int)pc, (unsigned int)_instruct, stack_len);
#endif

        /* point to prev stack button */
        sp += stack_len;  
        /* the first instruct is : save ra to sp, and save ra to the top of this function stack
         * so: ra = *(sp + stack_len - 4)
         * */

        /* get (ra), return address, 
         * pc porint to the return address , return to the caller function */
        ra = sp - 4;
        pc = (*(unsigned long *)(unsigned long)(ra)); 

        printf("sp1:[%x]\n", (unsigned int)((unsigned long)(ra))); 
        printf("pc:[%x]\n", (unsigned int)(pc)); 


        if (pc == 0 || stack_len == 0) {
            break;
        }

        high++;
        if (high < size) {
            array[high] = (void *)pc;
        } else {
            printf("size is too small.\n");
            break;
        }
    }

    return high;  
}  
