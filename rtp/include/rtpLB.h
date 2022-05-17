#ifndef RTP_LOAD_BALANCE_H_
#define RTP_LOAD_BALANCE_H_

int init_rtp_sendRecv_process(int thread_num);
int processRTPArgs(rtpSendRecvThreadArgs* args, unsigned long hash_key);
int processArgs(Args* args, unsigned long hash_key);

#endif
