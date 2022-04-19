#ifndef _BASE_64_H_
#define _BASE_64_H_

char base64EncodeTable[65]={
                            'A','B','C','D','E','F','G','H',
                            'I','J','K','L','M','N','O','P',
                            'Q','R','S','T','U','V','W','X',
                            'Y','Z','a','b','c','d','e','f',
                            'g','h','i','j','k','l','m','n',
                            'o','p','q','r','s','t','u','v',
                            'w','x','y','z','0','1','2','3',
                            '4','5','6','7','8','9','+','/','\0'
                           };


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

int base64Encode(const char* src, int in_size, char* out, int out_size);
int base64Decode(const char* src, int in_size, char* out, int out_size);

#endif
