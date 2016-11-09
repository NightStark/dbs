#include <stdio.h>
#include "tl.h"

#define _backtrace(array, size) \
	__backtrace((array), (size))
#define LEN 128
void *buffer[LEN] = {0};  
void fun1()
{
    int h = 0;
    unsigned long int sp = 0;

    __asm__ volatile (  
            "move  %0, $29\t\n" //sp -> *p
            :"=r" (sp)     /* output register */
            );

    
    printf("%s %d sp=0x%x.\n", __func__, __LINE__, (unsigned int)sp);

    while(1) {
        //sleep(1000);
        h = h + 1;
    }
    /*
    h = _backtrace(buffer, LEN);
    printf("H:0x%x\n", h);
    while (h-- > 0) {
        printf("0x:%x\n", (unsigned int)(unsigned long)buffer[h]);
    }
    */
    return;
}
