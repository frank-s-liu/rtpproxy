#include "rtpSendRecvProcs.h"
#include "cmdSession.h"
#include "hash.h"
#include "args.h"

static RtpProcess* rtpProcess_s = NULL;
static int rtpProcess_num_s = 0;

int init_rtp_sendRecv_process(int thread_num)
{
    rtpProcess_num_s = thread_num;
    rtpProcess_s = new RtpProcess[thread_num]();
    return 0;
}

int processSdpArgs(SDPArgs* args, unsigned long hash_key)
{
    int index = hash_key % rtpProcess_num_s;
    return rtpProcess_s[index].add_pipe_event(args);
}


