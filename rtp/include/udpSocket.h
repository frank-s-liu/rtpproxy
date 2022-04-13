#ifndef __SIPP_UDP_CLIENT_TRANSPORT_H__
#define __SIPP_UDP_CLIENT_TRANSPORT_H__


#include "socket.h"
#include "rtpEnum.h"


#include <stdlib.h>


class UdpSrvSocket : public Socket
{
public:
    UdpSrvSocket(const char* local_ip, RTPDirection direction);
    UdpSrvSocket(const char* local_ip, unsigned short port);
    virtual ~UdpSrvSocket();
    virtual int send_to(const void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t addrlen=0);
    virtual int recv_from(void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t *addrlen=NULL);
    virtual int add_read_event2EpollLoop(int ep_fd, void* event_data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) ;
    virtual void setnoblock();
    virtual int connect_to(const char* ip, int port);
    virtual void setkeepalive(int interval);
    virtual unsigned short getlocalPort();
    virtual int getLocalIp(char* buf, int buflen);
    virtual int delSocketFromEpollLoop(int ep_fd);
    virtual int getStatus();
private:
    int bindPort(const char* local_ip,  RTPDirection);
    int bindPort(const char* local_ip, unsigned short port);

private:
    //unsigned short m_port;
    unsigned char  m_status;
};

#endif
