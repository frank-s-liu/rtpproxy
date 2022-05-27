#include "rtpEnum.h"
#include "rtpConstStr.h"


const char* g_crypto_suite_str[MAX_CRYPTO_SUIT] = {
                                                   "AEAD_AES_256_GCM"
                                                  };

const char* g_RTPDirection_str[MAX_DIRECTION] = {
                                                "EXTERNAL_PEER",
                                                "INTERNAL_PEER",
                                                };

const char* CMD_STR[MAX_CONTROL_CMD] = {"OFFER_CMD", "ANSWER_CMD", "DELETE_CMD", "PING_CMD", "PING_CHECK_CMD"};

const char* StateName[CMDSESSION_MAX_STATE] = {"CMDSESSION_STATE",                 "CMDSESSION_INIT_STATE",               "CMDSESSION_OFFER_PROCESSING_STATE", 
                                               "CMDSESSION_OFFER_PROCESSED_STATE", "CMDSESSION_ANSWER_PROCESSING_STATE",  "CMDSESSION_ANSWER_PROCESSED_STATE",
                                               "CMDSESSION_DELETE_STATE"};
