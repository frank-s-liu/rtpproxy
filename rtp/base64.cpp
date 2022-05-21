#include "stdlib.h"

#include "base64.h"
#include "log.h"

static 
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


int base64Encode(const unsigned char* src, int in_size, unsigned char* out, int out_size)
{
    int padding_size = in_size%3;
    int need_padding = padding_size==0?0:1;
    char padding[3] = {0};
    int outindex = 0;
    union Base64* encode = NULL;
    if(need_padding && padding_size==1)
    {
        padding[0] = src[in_size-1];
        padding[1] = '\0';
        padding[2] = '\0';
    }
    else if(need_padding && padding_size==2)
    {
        padding[0] = src[in_size-2];
        padding[1] = src[in_size-1];
        padding[2] = '\0';
    }   
    if(out_size <= (in_size/3 * 4 + need_padding*4))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "not enough buf to save base64 encoded data");
        return -1;
    }
    in_size -= padding_size;
    for(int index=0; index<in_size;)
    {
        encode = (union Base64*) &src[index];
        out[outindex++] = base64EncodeTable[encode->c1];
        out[outindex++] = base64EncodeTable[encode->c2_1<<4 | encode->c2_2];
        out[outindex++] = base64EncodeTable[encode->c3_1<<2 | encode->c3_2];
        out[outindex++] = base64EncodeTable[encode->c4];
        index += 3;
    }
    if(need_padding)
    {
        encode = (union Base64*)padding;
        out[outindex++] = base64EncodeTable[encode->c1];
        out[outindex++] = (base64EncodeTable[encode->c2_1<<4 | encode->c2_2] == 'A')? '=':base64EncodeTable[encode->c2_1<<4 | encode->c2_2];
        out[outindex++] = (base64EncodeTable[encode->c3_1<<2 | encode->c3_2] == 'A')? '=':base64EncodeTable[encode->c3_1<<2 | encode->c3_2];
        out[outindex++] = (base64EncodeTable[encode->c4] == 'A')? '=':base64EncodeTable[encode->c4];
    }
    out[outindex] = '\0';
    return 0;
}


int base64Decode(const char* src, int in_size, unsigned char* out, int out_size)
{
    int outindex = 0;
    union Base64* encode = NULL;
    if(in_size%4 != 0 || out_size <= (in_size/4 *3))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "not enough buf to save base64 Decoded data");
        return -1;
    }
    for(int index=0; index<in_size; )
    {
        encode = (union Base64*)&out[outindex];
        if(src[index]>= 65 && src[index]<=90)
        {
            encode->c1 = src[index]-65;
        }
        else if(src[index]>=97 && src[index]<=122)
        {
            encode->c1 = src[index]-71;
        }
        else if(src[index]>=48 && src[index]<=57)
        {
            encode->c1 = src[index]+4;
        }
        else if(src[index] == 43)
        {
            encode->c1 = 62;
        }
        else if(src[index] == 47)
        {
            encode->c1 = 63;
        }
        else if(src[index] == 61)
        {
            break;
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "base64 Decoded error, unknow base64 char %c ", src[index]);
            return -1;
        }
        index++;

        if(src[index]>= 65 && src[index]<=90)
        {
            encode->c2_1 = ((src[index]-65) >> 4) & 0x03;
            encode->c2_2 = (src[index]-65) & 0x0F;
        }
        else if(src[index]>=97 && src[index]<=122)
        {
            encode->c2_1 = ((src[index]-71) >> 4) & 0x03;
            encode->c2_2 = (src[index]-71) & 0x0F;
        }
        else if(src[index]>=48 && src[index]<=57)
        {
            encode->c2_1 = ((src[index]+4)>>4) & 0x03;
            encode->c2_2 = (src[index]+4) & 0x0F;
        }
        else if(src[index] == 43)
        {
            encode->c2_1 = 3;
            encode->c2_2 = 14;
        }
        else if(src[index] == 47)
        {
            encode->c2_1 = 3;
            encode->c2_2 = 15;
        }
        else if(src[index] == 61)
        {
            break;
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "base64 Decoded error, unknow base64 char %c ", src[index]);
            return -1;
        }
        index++;
        if(src[index]>= 65 && src[index]<=90)
        {
            encode->c3_1 = ((src[index]-65) >>2) &0x0F;
            encode->c3_2 = (src[index]-65)&0x03;
        }
        else if(src[index]>=97 && src[index]<=122)
        {
            encode->c3_1 = ((src[index]-71)>>2) &0x0F;
            encode->c3_2 = (src[index]-71)&0x03;
        }
        else if(src[index]>=48 && src[index]<=57)
        {
            encode->c3_1 = ((src[index]+4)>>2)&0x0F;
            encode->c3_2 = (src[index]+4)&0x03;
        }
        else if(src[index] == 43)
        {
            encode->c3_1 = 15;
            encode->c3_2 = 2;
        }
        else if(src[index] == 47)
        {
            encode->c3_1 = 15;
            encode->c3_2 = 3;
        }
        else if(src[index] == 61)
        {
            outindex += 1;
            break;
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "base64 Decoded error, unknow base64 char %c ", src[index]);
            return -1;
        }
        index++;
        if(src[index]>= 65 && src[index]<=90)
        {
            encode->c4 = src[index]-65;
        }
        else if(src[index]>=97 && src[index]<=122)
        {
            encode->c4 = src[index]-71;
        }
        else if(src[index]>=48 && src[index]<=57)
        {
            encode->c4 = src[index]+4;
        }
        else if(src[index] == 43)
        {
            encode->c4 = 62;
        }
        else if(src[index] == 47)
        {
            encode->c4 = 63;
        }
        else if(src[index] == 61)
        {
            outindex +=2;
            break;
        }
        else
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "base64 Decoded error, unknow base64 char %c ", src[index]);
            return -1;
        }
        index++;
        outindex += 3;
     }
    out[outindex] = '\0';
    return 0;
}

