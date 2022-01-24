#ifndef __UTIL_H_
#define __UTIL_H_ 

#ifdef __cplusplus
extern "C"
{
#endif

union Test_little_endian
{
    unsigned char a;
    unsigned int  b;
};
	
__attribute((always_inline)) 
static inline int little_endian()
{
    Test_little_endian little;
    little.b = 1;
    if(1 == little.a)
    {
	return 1;
    }
    else
    {
        return 0;
    }		
}




#ifdef __cplusplus
}
#endif

#endif
