#ifndef __SIPP_UDP_CLIENT_TRANSPORT_H__
#define __SIPP_UDP_CLIENT_TRANSPORT_H__


#include "socket.h"

#include <stdlib.h>


class UdpConnection : public Socket
{
public:
    UdpConnection(const char* local_ip);
    UdpConnection(const char* local_ip, int port);
    virtual ~UdpConnection();
    virtual int send_to(const void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t addrlen=0);
    virtual int recv_from(void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t *addrlen=NULL);
    virtual int add_read_event2EpollLoop(int ep_fd, void* event_data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) ;
    virtual void setnoblock();
    virtual int connect_to(const char* ip, int port)=0;
    virtual void setkeepalive(int interval);
    virtual int getlocalPort();
    virtual int delSocketFromEpollLoop(int ep_fd);

private:
    int bind(const char* local_ip);
    int bind(const char* local_ip, int port);

private:
    int m_port;
    int m_status;
    char m_local_ip[64];

};

#endif
