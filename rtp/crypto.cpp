#include <openssl/evp.h>
#include <openssl/hmac.h>



#include "crypto.h"



struct crypto_suite __crypto_suites[MAX_CRYPTO_SUIT] = 
{
         {
                 .suit_srtp_name              = "AEAD_AES_256_GCM",
                 .master_key_len              = 32,   //byte
                 .master_salt_len             = 12,
                 .session_key_len             = 32,
                 .session_salt_len            = 12,
                 .srtp_lifetime               = 1ULL << 48,
                 .srtcp_lifetime              = 1ULL << 31,
                 .kernel_cipher               = REC_AEAD_AES_GCM_256,
                 .kernel_hmac                 = REH_NULL,
                 .srtp_auth_tag_len           = 0,
                 .srtcp_auth_tag_len          = 0,
                 .srtp_auth_key_len           = 0,
                 .srtcp_auth_key_len          = 0,
                 .encrypt_rtp                 = aes_gcm_encrypt_rtp,
                 .decrypt_rtp                 = aes_gcm_decrypt_rtp,
                 .encrypt_rtcp                = aes_gcm_encrypt_rtcp,
                 .decrypt_rtcp                = aes_gcm_decrypt_rtcp,
                 .session_key_init            = aes_gcm_session_key_init,
                 .session_key_cleanup         = evp_session_key_cleanup,
                 .aead_evp                    = EVP_aes_256_gcm,
         }
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
    unsigned char block[16];
    int len = 0;
    if(m_session_key_ctx[0])
    {
        EVP_EncryptFinal_ex(m_session_key_ctx[0], block, &len);
        EVP_CIPHER_CTX_free(m_session_key_ctx[0]);
        m_session_key_ctx[0] = NULL;
    }
    if(m_session_key_ctx[1])
    {
        EVP_EncryptFinal_ex(m_session_key_ctx[1], block, &len);
        EVP_CIPHER_CTX_free(m_session_key_ctx[1]);
        m_session_key_ctx[1] = NULL;
    }  
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
