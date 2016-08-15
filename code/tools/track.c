#define DEV_ARCH_MIPS

#ifdef DEV_ARCH_XX86
#include <execinfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN 128
#define FILENAME "stack" 

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

void *buffer[LEN] = {0};  
unsigned long g_sp = 0;
void fun1()
{
    int h = 0;
    h = _backtrace(buffer, LEN);
    printf("H:0x%x\n", h);
    while (h-- > 0) {
        printf("0x:%x\n", (unsigned int)(unsigned long)buffer[h]);
    }

}

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
            :"m" (p)     /* input  register */
            :"memory"   /* cloberred register */
            );

    printf("ra:[%x]\n", 
            (unsigned int)ra);

#if 0
    __asm__ volatile (  
            "lw  $8, %0\t\n"
            "sw  $29,($8)\t\n" //sp -> *p
            :  /* output register */
            :"m" (p)     /* input  register */
            :"memory"   /* cloberred register */
            );
#endif
    __asm__ volatile (  
            "move  %0, $29\t\n" //sp -> *p
            :"=r" (sp)     /* input  register */
            );
    pc = (unsigned long)__getpc();
    printf("%s [%x] sp:%x\n", __func__,
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
#endif
