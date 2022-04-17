#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string.h>  // memcpy
#include <assert.h>

#include "crypto.h"
#include "rtpEnum.h"

static int aes_gcm_session_key_init(class Crypto_context *c);
static int evp_session_key_cleanup(class Crypto_context *c);

struct crypto_suite s_crypto_suites[MAX_CRYPTO_SUIT] = 
{
    {
        .suit_srtp_name              = "AEAD_AES_256_GCM",
        .master_key_len              = 32,   //byte
        .master_salt_len             = 12,
        .session_key_len             = 32,
        .session_salt_len            = 12,
        .srtp_auth_tag_len           = 0,
        .srtcp_auth_tag_len          = 0,
        .srtp_auth_key_len           = 0,
        .srtcp_auth_key_len          = 0,
        .srtp_lifetime               = 1ULL << 48,
        .srtcp_lifetime              = 1ULL << 31,
        //.kernel_cipher               = REC_AEAD_AES_GCM_256,
        //.kernel_hmac                 = REH_NULL,
        //.encrypt_rtp                 = aes_gcm_encrypt_rtp,
        //.decrypt_rtp                 = aes_gcm_decrypt_rtp,
        //.encrypt_rtcp                = aes_gcm_encrypt_rtcp,
        //.decrypt_rtcp                = aes_gcm_decrypt_rtcp,
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

Crypto_context::Crypto_context()
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    init_crypto_param();
    m_session_key_ctx[0] = NULL;
    m_session_key_ctx[1] = NULL;
    m_session_key[0] = '\0';
    m_session_salt[0] = '\0';
    m_session_auth_key[0] = '\0';
    m_have_session_key = 0;
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

void Crypto_context::init_crypto_param()
{
    m_params.mki = NULL;
    m_params.crypto_suite = NULL;
    m_params.master_key[0] = '\0';
    m_params.master_salt[0] = '\0';
    m_params.mki_len = 0;
    m_params.session_params.unencrypted_srtcp = 0;
    m_params.session_params.unencrypted_srtp = 0;
    m_params.session_params.unauthenticated_srtp = 0;
}
