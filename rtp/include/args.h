#ifndef _RTPPROXY_ARGS_H__
#define _RTPPROXY_ARGS_H__

#include "cmdSession.h"

typedef struct ping_check_args
{
    unsigned long      ping_recv_count;
    CmdSession*        cs;
}PingCheckArgs;

#endif
