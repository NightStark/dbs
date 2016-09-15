#ifndef _BASE64_HH 
#define _BASE64_HH


int base64_encode(unsigned char *t, const unsigned char *f, int dlen);   
int base64_decode(unsigned char *t, const unsigned char *f, int n);

#endif

