#ifndef __RTP_CONTROL_PROCESS_H__
#define __RTP_CONTROL_PROCESS_H__

#include "thread.h"
#include "rtpepoll.h"

class ControlProcess : public Thread
{
public:
    ControlProcess();
    virtual ~ControlProcess();
    virtual void doStop();
    virtual void* run();

private:
    int             m_fd_pipe[2];
    int*            m_fd_socket;
    Epoll_data*     m_epoll_socket_data;
    unsigned char   m_fd_socket_num;
};

#endif
