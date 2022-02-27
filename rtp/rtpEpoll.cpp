#include "rtpepoll.h"
#include "log.h"


#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

Epoll_data::~Epoll_data()
{
    if(data)
    {
        rm_fd_from_epoll();
        close(data->fd);
        delete data;
        data = NULL;
        fd_state = CLOSED;
    }
}

int Epoll_data::rm_fd_from_epoll()
{
    int ret = 0;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
    if(ret < 0)  
    {
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "can not rm socket from epool loop, errno is %d", errno);
    }
    return ret;
}

