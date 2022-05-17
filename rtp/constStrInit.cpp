#include "rtpEnum.h"
#include "rtpConstStr.h"


const char* g_crypto_suite_str[MAX_CRYPTO_SUIT] = {
                                                   "AEAD_AES_256_GCM"
                                                  };

const char* g_RTPDirection_str[MAX_DIRECTION] = {
                                                "EXTERNAL_PEER",
                                                "EXTERNAL_LOCAL",
                                                "INTERNAL_PEER",
                                                "INTERNAL_LOCAL"
                                                };
