#include "rtpHeader.h"
#include "rtpEnum.h"
#include "log.h"
#include "cstr.h"

#include <arpa/inet.h>
#include <stdlib.h>

union Test_little_endian
{
    unsigned char a;
    unsigned int  b;
};

static int is_little_endian()
{
    union Test_little_endian little;
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
    struct Rtp_extension* ext;
    int rtp_header_size = sizeof(struct Rtp_Fixed_header);
    unsigned char cc, x, v;
    //unsigned char padding_num = 0;
    if(s->len < (int)sizeof(struct Rtp_Fixed_header))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package is too short");
        goto error;
    }
    rtp = (struct Rtp_Fixed_header*)s->s;
    //if(s_little_endian == 1) // single byte don't need to check
    //{
        cc = rtp->v_p_x_cc.v_p_x_cc_little.cc;
        x = rtp->v_p_x_cc.v_p_x_cc_little.x;
        v = rtp->v_p_x_cc.v_p_x_cc_little.v;
    //}
    //else
    //{
    //    cc = rtp->v_p_x_cc.v_p_x_cc_big.cc;
    //    x = rtp->v_p_x_cc.v_p_x_cc_big.x;
    //    v = rtp->v_p_x_cc.v_p_x_cc_big.v;
    //}
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
    if(x)
    {
        ext = (struct Rtp_extension*)(s->s + rtp_header_size);
        int length = ntohs(ext->length) *4; // byte
        rtp_header_size += (4+length);
    }
    if(s->len <  rtp_header_size)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtp package cc parsing error extension: %d", x);
        goto error;
    }
       
    *out = rtp;
    if(!payload_out)
    {
        return 0;
    }
    payload_out->s = s->s + rtp_header_size;
    // not parsed here, the last byte of srtp is not the padding number
    //if(p==1)
    //{
    //    padding_num = s->s[s->len-1]; // the last byte is the padding number
    //}
    payload_out->len = s->len - rtp_header_size;// - padding_num;
    return 0;

error:
    return -1;
}

uint32_t packet_index(struct SSRC_CTX* ssrc_ctx, struct Rtp_Fixed_header* rtpHdr)
{
    uint16_t seq = ntohs(rtpHdr->seq_num);
    if(ssrc_ctx->srtp_index == 0)
    {
        ssrc_ctx->srtp_index = seq;
    }
    // rfc 3711 appendix A, 
    uint16_t s_l = (ssrc_ctx->srtp_index & 0x0000ffffULL);
    uint32_t roc = (ssrc_ctx->srtp_index & 0xffff0000ULL) >> 16;
    uint32_t v = 0;

    // to find which value from (rec-1, roc, roc+1) can make  (v<<16+seq) more likely equal to ssrc_ctx->srtp_index
    if (s_l < 0x8000) 
    {
        if(roc > 0 && ((seq - s_l) > 0x8000))
        {
            v = (roc - 1) % 0x10000;
        }
        else
        {
            v = roc;
        }
    } 
    else 
    {
        if((s_l - 0x8000) <= seq)
        {
            v = roc;
        }
        else    
        {
            v = (roc + 1) % 0x10000;
        }
    }
    ssrc_ctx->srtp_index = ((v << 16) | seq);
    return ssrc_ctx->srtp_index;
}

// to_auth_check->s/len  is all the byte used to math authentication tag 
int srtp_payloads(cstr* to_auth_check, cstr* to_decrypt, cstr* auth_tag, int auth_len, cstr* mki,
                  int mki_len, const cstr* raw_rtp_packet, const cstr* payload)
{
   to_auth_check->len = raw_rtp_packet->len;
   to_auth_check->s = raw_rtp_packet->s;
   to_decrypt->len = payload->len;
   to_decrypt->s = payload->s;
   
   if(auth_tag->len)
   {
       delete[] auth_tag->s;
   }
   auth_tag->s = NULL;
   auth_tag->len = 0;
   if(auth_len) 
   {
       if(to_decrypt->len <= auth_len)
       {
           tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "srtp package format error, %d %d", to_decrypt->len, auth_len);
           goto error;
       }
       auth_tag->s = to_decrypt->s + (to_decrypt->len - auth_len);
       auth_tag->len = auth_len;
       to_decrypt->len -= auth_len;
       to_auth_check->len -= auth_len;
   }
   
   if(mki)
   {
       if(mki->len)
       {
           delete[] mki->s;
       }
       mki->s = NULL;
       mki->len = 0;
   }
   if(mki_len) 
   {
       if(to_decrypt->len <= mki_len)
       {
           tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "srtp package format error, %d %d", to_decrypt->len, mki_len);
           goto error;
       }
       if(mki)
       {
           mki->s = to_decrypt->s +(to_decrypt->len - mki_len);
           mki->len =  mki_len;
       }
       to_decrypt->len -= mki_len;
       to_auth_check->len -= mki_len;
   }

   return 0;

error:
    return -1;
}



