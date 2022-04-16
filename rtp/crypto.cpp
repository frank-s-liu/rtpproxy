

#include "crypto.h"

struct crypto_suite __crypto_suites[] = {
  45         {
  46                 .name                   = "AEAD_AES_256_GCM",
  47                 .dtls_name              = "SRTP_AEAD_AES_256_GCM",
  48                 .master_key_len         = 32,
  49                 .master_salt_len        = 12,
  50                 .session_key_len        = 32,
  51                 .session_salt_len       = 12,
  52                 .srtp_lifetime          = 1ULL << 48,
  53                 .srtcp_lifetime         = 1ULL << 31,
  54                 .kernel_cipher          = REC_AEAD_AES_GCM_256,
  55                 .kernel_hmac            = REH_NULL,
  56                 .srtp_auth_tag          = 0,
  57                 .srtcp_auth_tag         = 0,
  58                 .srtp_auth_key_len      = 0,
  59                 .srtcp_auth_key_len     = 0,
  60                 .encrypt_rtp            = aes_gcm_encrypt_rtp,
  61                 .decrypt_rtp            = aes_gcm_decrypt_rtp,
  62                 .encrypt_rtcp           = aes_gcm_encrypt_rtcp,
  63                 .decrypt_rtcp           = aes_gcm_decrypt_rtcp,
  64                 .session_key_init       = aes_gcm_session_key_init,
  65                 .session_key_cleanup    = evp_session_key_cleanup,
  66                 .aead_evp               = EVP_aes_256_gcm,
  67         },
