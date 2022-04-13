#ifndef  RTP_ENUM_H__
#define  RTP_ENUM_H__

enum RTPDirection
{
    EXTERNAL_PEER = 0,
    EXTERNAL_LOCAL,
    INTERNAL_PEER,
    INTERNAL_LOCAL,
    MAX_DIRECTION
};

typedef enum cmd_type
{
    OFFER_CMD=0,
    ANSWER_CMD,
    DELETE_CMD,
    PING_CMD,

//  internal cmd
    PING_CHECK_CMD,
    MAX_CONTROL_CMD
}CONTROL_CMD;

enum CmdState
{
    CMDSESSION_STATE = 0,
    CMDSESSION_INIT_STATE,
    CMDSESSION_OFFER_PROCESSING_STATE,
    CMDSESSION_OFFER_PROCESSED_STATE,
    CMDSESSION_MAX_STATE
};

#endif
