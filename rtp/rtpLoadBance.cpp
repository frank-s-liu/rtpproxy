#include "rtpSendRecvProcs.h"
#include "cmdSession.h"


static RtpProcess* rtpProcess_s = NULL;

int init_rtp_sendRecv_process(int thread_num)
{
    rtpProcess_s = new RtpProcess()[thread_num];
    return 0;
}

int processCmd(int cmd)
{

}


