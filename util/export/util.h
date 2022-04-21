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


union little_64
{
    unsigned long src;
    struct
    {
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
        unsigned char b4;
        unsigned char b5;
        unsigned char b6;
        unsigned char b7;
        unsigned char b8;
    };
};

union big_64
{
    unsigned long src;
    struct
    {
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
        unsigned char b4;
        unsigned char b5;
        unsigned char b6;
        unsigned char b7;
        unsigned char b8;
    };
};

static_assert(sizeof(union little_64) == 8, "sizeof(long) is not 8 byte");

static inline unsigned long hton64(unsigned long v)
{
    int little = little_endian();
    if(little)
    {
        union little_64 l;
        l.src = v;
        union big_64 b;
        b.b1 = l.b8;
        b.b2 = l.b7;
        b.b3 = l.b6;
        b.b4 = l.b5;
        b.b5 = l.b4;
        b.b6 = l.b3;
        b.b7 = l.b2;
        b.b8 = l.b1;
        return b.src;
    }
    return v;
}



#ifdef __cplusplus
}
#endif

#endif
