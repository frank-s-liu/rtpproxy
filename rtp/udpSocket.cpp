#include "udpSocket.h"
#include "transport.h" 
#include "log.h"


#include "string.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <assert.h>
#include <netinet/tcp.h>

extern unsigned short getExternalPort();
extern void releaseExternalPort(unsigned short port);
extern unsigned short  getInternalPort();
extern void releaseInternalPort(unsigned short port);

UdpSrvSocket::UdpSrvSocket(const char* local_ip, RTPDirection direction):Socket(UDP, IPV4)
{
    m_status = 0;
    bindPort(local_ip, direction);
}

UdpSrvSocket::UdpSrvSocket(const char* local_ip, unsigned short port):Socket(UDP, IPV4)
{
    m_status = 0;
    bindPort(local_ip, port);
}

UdpSrvSocket::~UdpSrvSocket()
{
    close(m_socket);
    m_socket = INVALID_SOCKET;
}

int UdpSrvSocket::connect_to(const char* host, int port)
{
    return 0;
}

int UdpSrvSocket::bindPort(const char* local_ip, RTPDirection direction)
{
    unsigned short port = 0;
    struct sockaddr_in addr;
    int ret = 1;
    int loop = 0;
    if(m_socket != INVALID_SOCKET)
    {
        close(m_socket);
        m_socket = INVALID_SOCKET;
    }
    while(ret)
    {
        if(direction == EXTERNAL_PEER)
        {
            port = getExternalPort();
        }
        else if(direction == INTERNAL_PEER)
        {
            port = getInternalPort();
        }
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(local_ip);
        addr.sin_port = htons(port);
        ret = ::bind(m_socket, (struct sockaddr *) &addr, sizeof(addr));
        if(0 == ret)
        {
            //struct sockaddr_in sockaddr;
            //int len = sizeof(sockaddr);
            //getsockname(m_socket, (struct sockaddr *) &sockaddr, (socklen_t *) &len); 
            //port = ntohs(sockaddr.sin_port); 
            m_status = 1;
            break;
        }
        loop++;
        if((loop&15) == 0)
        {
            tracelog("TRANSPORT", WARNING_LOG, __FILE__, __LINE__, "bind rtp port %d to socket, error %d", port, errno);
        }
        if(loop > 128)
        {
            tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not bind one rtp port to socket, error %d", errno);
            //usleep(5000);
            //assert(0);
            m_status = 0;
            ret = -1;
            break;
        }
    }
    
    return ret;
}

int UdpSrvSocket::bindPort(const char* local_ip, unsigned short port)
{
    struct sockaddr_in addr;
    int ret = 1;
    if(m_socket != INVALID_SOCKET)
    {
        close(m_socket);
        m_socket = INVALID_SOCKET;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(local_ip);
    addr.sin_port = htons(port);
    ret = ::bind(m_socket, (struct sockaddr *) &addr, sizeof(addr));
    if(0 == ret)
    {
        //struct sockaddr_in sockaddr;
        //int len = sizeof(sockaddr);
        //getsockname(m_socket, (struct sockaddr *) &sockaddr, (socklen_t *) &len); 
        //port = ntohs(sockaddr.sin_port); 
        m_status = 1;
    }
    else
    {
        m_status = 0;
    }
    return ret;
}

int UdpSrvSocket::recv_from(void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t *addrlen)
{
    return ::recvfrom(m_socket, buf, buflen, flag, src_addr, addrlen);
}

int UdpSrvSocket::send_to(const void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t addrlen)
{
    return ::sendto(m_socket, buf, buflen, flag, src_addr, addrlen);
}

void UdpSrvSocket::setnoblock()
{
    int old_option = fcntl(m_socket, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(m_socket, F_SETFL, new_option);
}

void UdpSrvSocket::setkeepalive( int interval)
{
    tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "udp can not set keep alive ");
}

int UdpSrvSocket::add_read_event2EpollLoop(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not add tcp socket to epool loop, reason %d ", errno);
    }
    return ret;
}

int UdpSrvSocket::modify_write_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop, reason %d ", errno);
    }
    return ret;
}

int UdpSrvSocket::modify_read_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop, reason %d ", errno);
    }
    return ret;
} 

int UdpSrvSocket::delSocketFromEpollLoop(int ep_fd)
{
    int ret = 0;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_DEL, m_socket, NULL);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify udp socket to epool loop, reason %d ", errno);
    }
    return ret;
    
}

unsigned short UdpSrvSocket::getlocalPort()
{
    unsigned short port = 0;
    if(m_status==1)
    {
        struct sockaddr_in sockaddr;
        int len = sizeof(sockaddr);
        getsockname(m_socket, (struct sockaddr *) &sockaddr, (socklen_t *) &len); 
        port = ntohs(sockaddr.sin_port);
    }
    return port;
}

int UdpSrvSocket::getLocalIp(char* buf, int buflen)
{
    int ret = -1;
    if(m_status == 1)
    {
        struct sockaddr_in sockaddr;
        int len = sizeof(sockaddr);
        getsockname(m_socket, (struct sockaddr *) &sockaddr, (socklen_t *) &len); 
        const char* res = inet_ntop(AF_INET, &sockaddr.sin_addr, buf, buflen);
        if(!res)
        {
            ret = -1;
            tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "usp server get local ip error, reason %d ", errno);
        }
    }
    return ret;
}

int UdpSrvSocket::getStatus()
{
    return m_status;
}
