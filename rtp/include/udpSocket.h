#ifndef __SIPP_UDP_CLIENT_TRANSPORT_H__
#define __SIPP_UDP_CLIENT_TRANSPORT_H__


#include "socket.h"

#include <stdlib.h>


class UdpSocket : public Socket
{
public:
    UdpSocket(const char* local_ip);
    UdpSocket(const char* local_ip, int port);
    virtual ~UdpSocket();
    virtual int send_to(const void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t addrlen=0);
    virtual int recv_from(void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t *addrlen=NULL);
    virtual int add_read_event2EpollLoop(int ep_fd, void* event_data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) ;
    virtual void setnoblock();
    virtual int connect_to(const char* ip, int port);
    virtual void setkeepalive(int interval);
    virtual int getlocalPort();
    virtual int delSocketFromEpollLoop(int ep_fd);

private:
    int bindPort(const char* local_ip);
    int bindPort(const char* local_ip, int port);

private:
    unsigned short m_port;
    unsigned char  m_status;
    
};

#endif
