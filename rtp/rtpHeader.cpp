#include "rtpHeader.h"
#include "rtpEnum.h"
#include "log.h"
#include "cstr.h"


union Test_little_endian
{
    unsigned char a;
    unsigned int  b;
};

static int is_little_endian()
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

static const int s_little_endian = is_little_endian();

//RFC 3551 payload type info
const struct RFC_payload rfc_rtp_payload_types[MAX_PAYLOAD_TYPE_INDEX] = 
{
    {
        .encoding = "PCMU",
        .encoding_with_params = "PCMU/8000",
        .clock_rate = 8000,
        .payload_type = 0,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "GSM",
        .encoding_with_params = "GSM/8000",
        .clock_rate = 8000,
        .payload_type = 3,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 8,
    },
    {
        .encoding = "G723",
        .encoding_with_params = "G723/8000",
        .clock_rate = 8000,
        .payload_type = 4,
        .channels = 1,
        .ptime = 30,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "DVI4",
        .encoding_with_params = "DVI4/8000",
        .clock_rate = 8000,
        .payload_type = 5,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "DVI4",
        .encoding_with_params = "DVI4/16000",
        .clock_rate = 16000,
        .payload_type = 6,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "LPC",
        .encoding_with_params = "LPC/8000",
        .clock_rate = 8000,
        .payload_type = 7,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 8,
    },
    {
        .encoding = "PCMA",
        .encoding_with_params = "PCMA/8000",
        .clock_rate = 8000,
        .payload_type = 8,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "G722",
        .encoding_with_params = "G722/8000",
        .clock_rate = 8000,
        .payload_type = 9,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "L16",
        .encoding_with_params = "L16/44100",
        .clock_rate = 44100,
        .payload_type = 10,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "L16",
        .encoding_with_params = "L16/44100",
        .clock_rate = 44100,
        .payload_type = 11,
        .channels = 2,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "QCELP",
        .encoding_with_params = "QCELP/8000",
        .clock_rate = 8000,
        .payload_type = 12,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 5,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "CN",
        .encoding_with_params = "CN/8000",
        .clock_rate = 8000,
        .payload_type = 13,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 2,
        .encoding_with_params_len = 7,
    },
    {
        .encoding = "MPA",
        .encoding_with_params = "MPA/90000",
        .clock_rate = 90000,
        .payload_type = 14,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "G728",
        .encoding_with_params = "G728/8000",
        .clock_rate = 8000,
        .payload_type = 15,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "DVI4",
        .encoding_with_params = "DVI4/11025",
        .clock_rate = 11025,
        .payload_type = 16,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "DVI4",
        .encoding_with_params = "DVI4/22050",
        .clock_rate = 22050,
        .payload_type = 17,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "G729",
        .encoding_with_params = "G729/8000",
        .clock_rate = 8000,
        .payload_type = 18,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "CelB",
        .encoding_with_params = "CelB/90000",
        .clock_rate = 90000,
        .payload_type = 25,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "JPEG",
        .encoding_with_params = "JPEG/90000",
        .clock_rate = 90000,
        .payload_type = 26,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "nv",
        .encoding_with_params = "nv/90000",
        .clock_rate = 90000,
        .payload_type = 28,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 2,
        .encoding_with_params_len = 8,
    },
    {
        .encoding = "H261",
        .encoding_with_params = "H261/90000",
        .clock_rate = 90000,
        .payload_type = 31,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "MPV",
        .encoding_with_params = "MPV/90000",
        .clock_rate = 90000,
        .payload_type = 32,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 3,
        .encoding_with_params_len = 9,
    },
    {
        .encoding = "MP2T",
        .encoding_with_params = "MP2T/90000",
        .clock_rate = 90000,
        .payload_type = 33,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
    {
        .encoding = "H263",
        .encoding_with_params = "H263/90000",
        .clock_rate = 90000,
        .payload_type = 34,
        .channels = 1,
        .ptime = 20,
        .encoding_len = 4,
        .encoding_with_params_len = 10,
    },
};

int rtp_payload(struct Rtp_Fixed_header** out, cstr* payload_out, const cstr* s) 
{
    struct Rtp_Fixed_header* rtp;
    //struct Rtp_extension* ext;
    unsigned int rtp_header_size = sizeof(struct Rtp_Fixed_header);
    unsigned char cc, x, v, p;
    unsigned char padding_num = 0;
    if(s->len < sizeof(struct Rtp_Fixed_header))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package is too short");
        goto error;
    }
    rtp = (Rtp_Fixed_header*)s->s;
    if(s_little_endian == 1)
    {
        cc = rtp->v_p_x_cc.v_p_x_cc_little.cc;
        x = rtp->v_p_x_cc.v_p_x_cc_little.x;
        v = rtp->v_p_x_cc.v_p_x_cc_little.v;
        p = rtp->v_p_x_cc.v_p_x_cc_little.p;
    }
    else
    {
        cc = rtp->v_p_x_cc.v_p_x_cc_big.cc;
        x = rtp->v_p_x_cc.v_p_x_cc_big.x;
        v = rtp->v_p_x_cc.v_p_x_cc_big.v;
        p = rtp->v_p_x_cc.v_p_x_cc_big.p;
    }
    if(v != 2)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package version error %d", v);
        goto error;
    }
    rtp_header_size += cc*4; // each cc has 4 byte
    if(s->len < rtp_header_size) // each cc has 4 byte 
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package cc parsing error cc: %d", cc);
        goto error;
    }
    rtp_header_size += x * sizeof(struct Rtp_extension);
    if(s->len <  rtp_header_size)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package cc parsing error extension: %d", x);
        goto error;
    }
       
    *out = rtp;
    if (!payload_out)
    {
        return 0;
    }
    payload_out->s = s->s + rtp_header_size;
    if(p==1)
    {
        padding_num = s->s[s->len-1]; // the last byte is the padding number
    }
    payload_out->len = s->len - rtp_header_size - padding_num;
    return 0;

error:
    return -1;
}

