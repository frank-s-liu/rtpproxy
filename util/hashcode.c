
#include <stdlib.h>

unsigned long BKDRHash(const char* str, int len)
{
    unsigned long hash = 0;
    if(NULL == str || len > 65535)
    {
        return -1;
    }
    //unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    while (str && len >0)
    {
        hash = (hash << 7) + (hash << 1) + hash + (*str);
        str++;
        //hash = hash * seed + (*str++); //
        len--;
    }
    return hash;
}
