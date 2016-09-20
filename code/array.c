
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/sha.h> 
#include <openssl/crypto.h>  // OPENSSL_cleanse

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>
#include <ns_lilist.h>
#include <ns_bitmap.h>

#include <ns_base.h>
#include <ns_web_event.h>

#ifdef __cplusplus
}
#endif


int main(void)
{
    int a = 18;

    BitMap_Init();
    ERR_PRINTF("hello");
    HTTPREQ_EvtDir_Init();
    return 0;
}

