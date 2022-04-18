#ifndef _RTP_HEADER_H_
#define _RTP_HEADER_H_

#include <stdint.h>

#include "cstr.h"


/* RTP Fixed Header Fields
 * RFC 3550 chipter 5.1
 */
struct Rtp_Fixed_header 
{
    union
    {
        struct
        {
            unsigned char cc:4;
            unsigned char x:1;
            unsigned char p:1;
            unsigned char v:2;
        }v_p_x_cc_little;
        struct
        {
            unsigned char v:2;
            unsigned char p:1;
            unsigned char x:1;
            unsigned char cc:4;
        }v_p_x_cc_big;
        unsigned char pad_1byte;
    }v_p_x_cc;
    unsigned char m_pt;
    uint16_t seq_num;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[0];  // 0 to 15 items, 32 bits each
} __attribute__ ((packed));

static_assert(sizeof(struct Rtp_Fixed_header) == 12, "must be not align here");

struct Rtp_extension 
{
    uint16_t undefined;
    uint16_t length;
} __attribute__ ((packed));

static_assert(sizeof(struct Rtp_extension) == 4, "must be not align here");

struct RFC_payload
{
    const char* encoding; // "opus"
    const char* encoding_with_params; // "opus/48000"
    unsigned int clock_rate; // 48000
    int payload_type;
    unsigned char channels; // 2
    unsigned char ptime; // default from RFC 20ms
    unsigned char encoding_len;
    unsigned char encoding_with_params_len;
};

struct Rtp_payload 
{
    const struct RFC_payload* pl;
    cstr encoding_with_full_params; // "opus/48000/2"
    unsigned char for_transcoding:1;
    unsigned char accepted:1;
};

#endif
