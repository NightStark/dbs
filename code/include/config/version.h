#ifndef __VERSION_H__
#define __VERSION_H__

#define NS_VER_MAIN (0)
#define NS_VER_SUB  (0)
#define NS_VER_STEP (1)

/* define in a int type 
 * 0x00_AA_BB_CC :
 *      AA: NS_VER_MAIN
 *      BB: NS_VER_SUB
 *      CC: NS_VER_STEP
 * */
#define NS_SERVER_VER (((NS_VER_MAIN) << 16) || (NS_VER_SUB << 8) || (NS_VER_STEP))

#endif //__VERSION_H__
