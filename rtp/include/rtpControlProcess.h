#ifndef __RTP_CONTROL_PROCESS_H__
#define __RTP_CONTROL_PROCESS_H__

#include "thread.h"
#include "rtpepoll.h"
#include "lockfreequeue_mutipush_one_pop.h"
#include "args.h"


typedef memqueue_s timerEvents;

class ControlProcess : public Thread
{
public:
    ControlProcess();
    virtual ~ControlProcess();
    virtual void doStop();
    virtual void* run();
    int add_pipe_timer_event(Args* args);
    static ControlProcess* getInstance();

private:
    int                           m_fd_pipe[2];
    SocketInfo*                   m_fd_socketInfo;
    Epoll_data*                   m_epoll_socket_data;
    timerEvents*                  m_pipe_events;
    static ControlProcess*        s_instance;
    unsigned char                 m_fd_socket_num;
};

#endif
