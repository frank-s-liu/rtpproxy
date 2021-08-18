
#include <stdlib.h>

long BKDRHash(const char* str, int len)
{
    if(NULL == str || len > 65535)
    {
        return -1;
    }
    //unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    long hash = 0;
    while (str && len >0)
    {
        hash = (hash << 7) + (hash << 1) + hash + (*str);
        str++;
        //hash = hash * seed + (*str++); //
        len--;
        hash &= 0x7FFFFFFFFFFFFFFF;
    }
    return hash;
}
