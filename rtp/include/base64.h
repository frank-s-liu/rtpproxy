#ifndef _BASE_64_H_
#define _BASE_64_H_


union Base64
{
    unsigned char src[3];
    struct
    {
        unsigned char c2_1:2;
        unsigned char c1:6;
        unsigned char c3_1:4;
        unsigned char c2_2:4;
        unsigned char c4:6;
        unsigned char c3_2:2;
    } __attribute__ ((packed));
} __attribute__ ((packed));

static_assert(sizeof(union Base64) == 3, "must be not align here");

int base64Encode(const unsigned char* src, int in_size, unsigned char* out, int out_size);
int base64Decode(const char* src, int in_size, unsigned char* out, int out_size);

#endif
