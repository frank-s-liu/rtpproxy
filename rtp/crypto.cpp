#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string.h>  // memcpy
#include <assert.h>
#include <arpa/inet.h>


#include "crypto.h"
#include "rtpEnum.h"
#include "rtpHeader.h"
#include "base64.h"
#include "util.h"
#include "log.h"

static int aes_gcm_session_key_init(class Crypto_context *c);
static int evp_session_key_cleanup(class Crypto_context *c);
static int aes_gcm_encrypt_rtp(Crypto_context *c, struct Rtp_Fixed_header* r, cstr* s, uint64_t idx);
static int aes_gcm_decrypt_rtp(Crypto_context* c, struct Rtp_Fixed_header* r, cstr* s, uint64_t idx);

union aes_gcm_rtp_iv 
{
    unsigned char bytes[12];
    struct 
    {
        uint16_t zeros;
        uint32_t ssrc;
        uint32_t roq; 
        uint16_t seq;
    } __attribute__((__packed__));
} __attribute__((__packed__));
    
static_assert(sizeof(union aes_gcm_rtp_iv) == 12, "union aes_gcm_rtp_iv not packed");


struct crypto_suite s_crypto_suites[MAX_CRYPTO_SUIT] = 
{
    {
        .suit_srtp_name              = "AEAD_AES_256_GCM",
        .master_key_len              = 32,   //byte
        .master_salt_len             = 12,
        .session_key_len             = 32,
        .session_salt_len            = 12,
        .srtp_auth_tag_len           = 0,  // no srtp authentication tag and no srtcp authentication tag in AEAD_AES_256_GCM 
        .srtcp_auth_tag_len          = 0,
        .srtp_auth_key_len           = 0,
        .srtcp_auth_key_len          = 0,
        .srtp_lifetime               = 1ULL << 48,
        .srtcp_lifetime              = 1ULL << 31,
        //.kernel_cipher               = REC_AEAD_AES_GCM_256,
        //.kernel_hmac                 = REH_NULL,
        .encrypt_rtp                 = aes_gcm_encrypt_rtp,
        .decrypt_rtp                 = aes_gcm_decrypt_rtp,
        //.encrypt_rtcp                = aes_gcm_encrypt_rtcp,
        //.decrypt_rtcp                = aes_gcm_decrypt_rtcp,
        .hash_rtp                    = NULL,
        .hash_rtcp                   = NULL,
        .session_key_init            = aes_gcm_session_key_init,
        .session_key_cleanup         = evp_session_key_cleanup,
        .aes_evp                     = NULL,
        .aead_evp                    = EVP_aes_256_gcm,
    },
};

void crypto_suit_init() 
{
    struct crypto_suite *cs;
    for (unsigned int i = 0; i < MAX_CRYPTO_SUIT; i++) 
    {
        cs = &s_crypto_suites[i];
        switch(cs->master_key_len) 
        {
            case 16:
            {
                cs->aes_evp = EVP_aes_128_ecb();
                break;
            }
            case 24:
            {
                cs->aes_evp = EVP_aes_192_ecb();
                break;
            }
            case 32:
            {
                cs->aes_evp = EVP_aes_256_ecb();
                break;
            }
        }
    }
}

static int evp_session_key_cleanup(Crypto_context *c) 
{
    unsigned char block[16];
    int len, i;
    int size = sizeof(c->m_session_key_ctx)/sizeof(void*);
    for (i = 0; i < size; i++) 
    {
        if (!c->m_session_key_ctx[i])
        {
            continue;
        }
        EVP_EncryptFinal_ex(c->m_session_key_ctx[i], block, &len);
        EVP_CIPHER_CTX_free(c->m_session_key_ctx[i]);
        c->m_session_key_ctx[i] = NULL;
    }
    return 0;
}

static int aes_gcm_session_key_init(Crypto_context *c) 
{
    evp_session_key_cleanup(c);
    c->m_session_key_ctx[0] = EVP_CIPHER_CTX_new();
    return 0;
}

/* rfc 3711 section 4.1 and 4.1.1
 * "in" and "out" MAY point to the same buffer 
 * */
static void aes_ctr(unsigned char *out, cstr* in, EVP_CIPHER_CTX* ecc, const unsigned char*iv) 
{
    unsigned char ivx[16];
    unsigned char key_block[16];
    unsigned int left;
    int outlen, i;
    uint64_t *pi, *qi, *ki;
    uintptr_t pi_addr, qi_addr;
    if (!ecc)
    {
        return;
    }
    memcpy(ivx, iv, 16);
    pi = (uint64_t*) in->s;
    qi = (uint64_t*) out;
    ki = (uint64_t*) key_block;
    left = in->len;

    pi_addr = (uintptr_t)pi;
    qi_addr = (uintptr_t)qi;
    assert(pi_addr % sizeof(uint64_t) == 0);
    assert(qi_addr % sizeof(uint64_t) == 0);
    while (left) 
    {
        EVP_EncryptUpdate(ecc, key_block, &outlen, ivx, 16);
        assert(outlen == 16);

        qi[0] = pi[0] ^ ki[0];
        qi[1] = pi[1] ^ ki[1];
        left -= 16;
        qi += 2;
        pi += 2;

        for (i = 15; i >= 0; i--) 
        {
            ivx[i]++;
            if(ivx[i])
            {
                break;
            }
        }
    }
 
    return;
}

/* rfc 3711 section 4.3.1 
 * label = 0x0 srtp session key
 * label = 0x1 srtp auth key
 * label = 0x2 srtp salt key
 * r = index/key_derivation_rate;
 *   if key_derivation_rate==0, r=0   //   ==  zero(6byte )
 * key_id = <label> || r;    //key_id  7byte
 *
 * x = key_id XOR master salt;
 *
 *C= A || B means connect A and B using network order, and A in the high order
 */
int crypto_gen_session_key(Crypto_context *c, cstr *out, unsigned char label, int index_len) 
{
    unsigned char key_id[7] = {0};
    unsigned char x[14];
    int i;
    unsigned char iv[16] = {0};
    union
    {
        uint64_t align;
        char in[32];
    }union_in;
    union
    {
        uint64_t align;
        unsigned char o[32];
    }union_out;
    memset(union_in.in, 0, sizeof(union_in.in));
    memset(union_out.o, 0, sizeof(union_out.o));
    unsigned char block[16];
    EVP_CIPHER_CTX *ctx;
    cstr c_in;
    int len;
    if (!out->len)
    {
        // has generated
	return 0;
    }
    c_in.s = union_in.in;
    c_in.len = out->len > 16 ? 32 : 16;
    key_id[0] = label;
    assert(sizeof(x) >= c->m_params.crypto_suite->master_salt_len);

    memcpy(x, c->m_params.master_salt, c->m_params.crypto_suite->master_salt_len);
    // AEAD uses 12 bytes master salt; pad on the right to get 14
    if (c->m_params.crypto_suite->master_salt_len == 12)
    {
        x[12] = x[13] = '\0';
    }
    for (i=(13-index_len); i<14; i++) // x = key_id XOR master salt
    {
        x[i] = key_id[i - (13 - index_len)] ^ x[i];
    }
    
    memcpy(iv, x, sizeof(x));
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, c->m_params.crypto_suite->aes_evp, NULL, c->m_params.master_key, NULL);
    aes_ctr(union_out.o, &c_in, ctx, iv);
    EVP_EncryptFinal_ex(ctx, block, &len);
    EVP_CIPHER_CTX_free(ctx);

    memcpy(out->s, union_out.o, out->len);

    return 0;
}

/**
 * RFC 3711 chipter 3.2.1 
 *    index = 2^16 * ROC + SEQ.
 **/
static int aes_gcm_encrypt_rtp(Crypto_context *c, struct Rtp_Fixed_header* r, cstr* s, uint64_t idx) 
{
    union aes_gcm_rtp_iv iv;
    int len, ciphertext_len;

    memcpy(iv.bytes, c->m_session_salt, 12);

    iv.ssrc ^= r->ssrc;
    iv.roq ^= htonl((idx & 0x00ffffffff0000ULL) >> 16);
    iv.seq ^= htons(idx & 0x00ffffULL);

    EVP_EncryptInit_ex(c->m_session_key_ctx[0], c->m_params.crypto_suite->aead_evp(), NULL,
                    (const unsigned char *) c->m_session_key, iv.bytes);

    // nominally 12 bytes of AAD
    EVP_EncryptUpdate(c->m_session_key_ctx[0], NULL, &len, (const unsigned char*)r, s->s - (char*)r);

    EVP_EncryptUpdate(c->m_session_key_ctx[0], (unsigned char*) s->s, &len,
                    (const unsigned char*) s->s, s->len);
    ciphertext_len = len;
    if (!EVP_EncryptFinal_ex(c->m_session_key_ctx[0], (unsigned char *) s->s+len, &len))
            return 1;
    ciphertext_len += len;
    // append the tag to the str buffer
    EVP_CIPHER_CTX_ctrl(c->m_session_key_ctx[0], EVP_CTRL_GCM_GET_TAG, 16, s->s+ciphertext_len);
    s->len = ciphertext_len + 16;

    return 0;
}

static int aes_gcm_decrypt_rtp(Crypto_context* c, struct Rtp_Fixed_header* r_hdr, cstr* srtp_load, uint64_t idx) 
{
    union aes_gcm_rtp_iv iv;
    int len, plaintext_len;

    if(srtp_load->len < 16)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "srtp payload size is too short %d", srtp_load->len);
        return -1;
    }
    memcpy(iv.bytes, c->m_session_salt, 12);

    iv.ssrc ^= r_hdr->ssrc;
    iv.roq ^= htonl((idx & 0x00ffffffff0000ULL) >> 16);
    iv.seq ^= htons(idx & 0x00ffffULL);

    EVP_DecryptInit_ex(c->m_session_key_ctx[0], c->m_params.crypto_suite->aead_evp(), NULL,
                    (const unsigned char*) c->m_session_key, iv.bytes);

    // nominally 12 bytes of AAD
    if(!EVP_DecryptUpdate(c->m_session_key_ctx[0], NULL, &len, (const unsigned char*)r_hdr, srtp_load->s - (char*)r_hdr))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "EVP_DecryptUpdate failed, index [%lu]", idx);
    }
    // decrypt partial buffer - the last 16 bytes are the tag
    if(!EVP_DecryptUpdate(c->m_session_key_ctx[0], (unsigned char *) srtp_load->s, &len,
                    (const unsigned char*) srtp_load->s, srtp_load->len-16))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "EVP_DecryptUpdate failed, index [%lu]", idx);
    }
    //tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"srtp lad size %d, rtp load size %d ", srtp_load->len-16, len);

    plaintext_len = len;
    EVP_CIPHER_CTX_ctrl(c->m_session_key_ctx[0], EVP_CTRL_GCM_SET_TAG, 16, srtp_load->s + (srtp_load->len-16));
    if(!EVP_DecryptFinal_ex(c->m_session_key_ctx[0], (unsigned char*) srtp_load->s+len, &len))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "EVP_DecryptFinal_ex failed, index [%lu]", idx);
        return -1;
    }
    plaintext_len += len;
    srtp_load->len = plaintext_len;

    return 0;
}


Crypto_context::Crypto_context(Crypto_Suite cry_suit)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    init_crypto_param(cry_suit);
    m_session_key_ctx[0] = NULL;
    m_session_key_ctx[1] = NULL;
    m_session_key[0] = '\0';
    m_session_salt[0] = '\0';
    m_session_auth_key[0] = '\0';
    m_have_session_key = 0;
    m_params.crypto_suite->session_key_init(this);
#else
    assert(0);
#endif
}

Crypto_context::~Crypto_context()
{
    evp_session_key_cleanup(this);
    deinit_crypto_param();
}

void Crypto_context::deinit_crypto_param()
{
    if(m_params.mki)
    {
        delete[] m_params.mki;
        m_params.mki = NULL;
    }
    m_params.crypto_suite = NULL;
}

void Crypto_context::init_crypto_param(Crypto_Suite cry_suit)
{
    m_params.mki = NULL;
    m_params.crypto_suite = &s_crypto_suites[cry_suit];
    m_params.master_key[0] = '\0';
    m_params.master_salt[0] = '\0';
    m_params.mki_len = 0;
    m_params.session_params.unencrypted_srtcp = 0;
    m_params.session_params.unencrypted_srtp = 0;
    m_params.session_params.unauthenticated_srtp = 0;
}

int Crypto_context::set_crypto_param(Attr_crypto* a)
{
    int ret = 0;
    unsigned char b64decode[256];
    cstr key;
    if(a->mki_len)
    {
        m_params.mki = new unsigned char[a->mki_len+1]; // add mki_len byte into srtp package, the value is a->mki_v in network order
        m_params.mki[a->mki_len] = '\0';
        if(a->mki_len == sizeof(char))
        {
            memcpy(m_params.mki, &a->mki_v, a->mki_len);
        }
        else if(a->mki_len == sizeof(short))
        {
            unsigned short v = a->mki_v;
            v = htons(v);
            memcpy(m_params.mki, &v, a->mki_len);
        }
        else if(a->mki_len == sizeof(int))
        {
            unsigned int v = a->mki_v;
            v = htonl(v);
            memcpy(m_params.mki, &v, a->mki_len);
        }
        else if(a->mki_len == sizeof(long))
        {
            unsigned long v = a->mki_v;
            v = hton64(v);
            memcpy(m_params.mki, &v, a->mki_len);
        }
        m_params.mki_len = a->mki_len;
    }
    ret = base64Decode(a->key_params.s, a->key_params.len, b64decode, sizeof(b64decode));
    if(ret != 0)
    {
        return ret;
    }
    memcpy(m_params.master_key, b64decode, m_params.crypto_suite->master_key_len);
    memcpy(m_params.master_salt, &b64decode[m_params.crypto_suite->master_key_len], m_params.crypto_suite->master_salt_len);

    key.s = m_session_key;
    key.len = m_params.crypto_suite->session_key_len;
    if(crypto_gen_session_key(this, &key, 0x00, 6))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto_gen_session_key session key failed");
        return -1;
    }
    key.s = m_session_auth_key;
    key.len = m_params.crypto_suite->srtp_auth_key_len;
    if(crypto_gen_session_key(this, &key, 0x01, 6))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto_gen_session_key auth key failed");
        return -1;
    }
    key.s = m_session_salt;
    key.len = m_params.crypto_suite->session_salt_len;
    if(crypto_gen_session_key(this, &key, 0x02, 6))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto_gen_session_key salt key failed");
        return -1;
    }
    return 0;
}
