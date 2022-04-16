#ifndef RTPPROXY_RTPCRYPTO_H_
#define RTPPROXY_RTPCRYPTO_H_

#include <openssl/evp.h>

static const unsigned char SRTP_MAX_MASTER_KEY_LEN = 32;
static const unsigned char SRTP_MAX_MASTER_SALT_LEN = 14;
static const unsigned char SRTP_MAX_SESSION_KEY_LEN = 32;
static const unsigned char SRTP_MAX_SESSION_SALT_LEN = 14;
static const unsigned char SRTP_MAX_SESSION_AUTH_LEN = 20;

struct crypto_suite 
{
    const char*                           suit_srtp_name;
    unsigned char                         master_key_len;
    unsigned char                         master_salt_len;
    unsigned char                         session_key_len;
    unsigned char                         session_salt_len;
    unsigned char                         srtp_auth_tag_len;
    unsigned char                         srtcp_auth_tag_len;
    unsigned char                         srtp_auth_key_len;
    unsigned char                         srtcp_auth_key_len;
    unsigned long long                    srtp_lifetime;
    unsigned long long                    srtcp_lifetime;       
    int                                   kernel_cipher;
    int                                   kernel_hmac;
    //crypto_rtp_cb                         encrypt_rtp;
    //crypto_rtp_cb                         decrypt_rtp;
    //crypto_rtcp_cb                        encrypt_rtcp;
    //crypto_rtcp_cb                        decrypt_rtcp;
    //hash_func_rtp                         hash_rtp;
    //hash_func_rtcp                        hash_rtcp;
    const EVP_CIPHER*                     aes_evp;
    unsigned int                          idx;
    cstr                                  name_str;
    const EVP_CIPHER*                     (*aead_evp)(void);
};

struct crypto_session_params 
{
    unsigned char    unencrypted_srtcp:1;
    unsigned char    unencrypted_srtp:1;
    unsigned char    unauthenticated_srtp:1;
};

struct crypto_params 
{
    struct crypto_session_params         session_params;
    const struct crypto_suite*           crypto_suite;
    unsigned char*                       mki;    // Master Key Identifier, The MKI MAY be used by key management for the purposes of re-keying
    unsigned char                        master_key[SRTP_MAX_MASTER_KEY_LEN];
    unsigned char                        master_salt[SRTP_MAX_MASTER_SALT_LEN];
    unsigned int                         mki_len;
};

class Crypto_context 
{
public:
    Crypto_context();
    virtual ~Crypto_context();
private:
    void init_crypto_param();
    void deinit_crypto_param();
public:
    struct crypto_params                m_params;
    void*                               m_session_key_ctx[2];
    char                                m_session_key[SRTP_MAX_SESSION_KEY_LEN];
    char                                m_session_salt[SRTP_MAX_SESSION_SALT_LEN];
    char                                m_session_auth_key[SRTP_MAX_SESSION_AUTH_LEN];
    unsigned char                       m_have_session_key:1;  // has generate session key or not
};

#endif
