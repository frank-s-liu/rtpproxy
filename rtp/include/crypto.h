#ifndef RTPPROXY_RTPCRYPTO_H_
#define RTPPROXY_RTPCRYPTO_H_


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
    unsigned char                         srtp_auth_tag;
    unsigned char                         srtcp_auth_tag;
    unsigned char                         srtp_auth_key_len;
    unsigned char                         srtcp_auth_key_len;
    unsigned long long                    srtp_lifetime;
    unsigned long long                    srtcp_lifetime;       
    int                                   kernel_cipher;
    int                                   kernel_hmac;
    crypto_rtp_cb                         encrypt_rtp;
    crypto_rtp_cb                         decrypt_rtp;
    crypto_rtcp_cb                        encrypt_rtcp;
    crypto_rtcp_cb                        decrypt_rtcp;
    hash_func_rtp                         hash_rtp;
    hash_func_rtcp                        hash_rtcp;
    session_key_init_func                 session_key_init;
    session_key_cleanup_func              session_key_cleanup;
    const EVP_CIPHER*                     aes_evp;
    unsigned int                          idx;
    cstr                                  name_str;
    const EVP_CIPHER*                     (*aead_evp)(void);
};

65 struct crypto_session_params {
 66         unsigned int unencrypted_srtcp:1,
 67                      unencrypted_srtp:1,
 68                      unauthenticated_srtp:1;
 69 };

struct crypto_params 
{
    const struct crypto_suite*           crypto_suite;
    unsigned char                        master_key[SRTP_MAX_MASTER_KEY_LEN];
    unsigned char                        master_salt[SRTP_MAX_MASTER_SALT_LEN];
    unsigned char*                       mki;
    unsigned int                         mki_len;
    struct crypto_session_params         session_params;
};



#endif
