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

UdpSocket::UdpSocket(const char* local_ip, RTPDirection direction):Socket(UDP, IPV4)
{
    m_port = -1;
    m_status = 0;
    bindPort(local_ip, direction);
}

UdpSocket::UdpSocket(const char* local_ip, int port):Socket(UDP, IPV4)
{
    m_port = -1;
    m_status = 0;
    bindPort(local_ip, port);
}

UdpSocket::~UdpSocket()
{
    close(m_socket);
    m_socket = INVALID_SOCKET;
}

int UdpSocket::connect_to(const char* host, int port)
{
    return 0;
}


int UdpSocket::bindPort(const char* local_ip, RTPDirection direction)
{
    unsigned short port = 0;
    struct sockaddr_in addr;
    int ret = 1;
    int loop = 0;
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
            m_port = port;
            m_status = 1;
            break;
        }
        loop++;
        if((loop&15) == 0)
        {
            tracelog("TRANSPORT", WARNING_LOG, __FILE__, __LINE__, "bind rtp port %d to socket, error %d", port, errno);
        }
        if(loop > 1000)
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

int UdpSocket::bindPort(const char* local_ip, int port)
{
    struct sockaddr_in addr;
    int ret = 1;
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
        m_port = port;
        m_status = 1;
    }
    else
    {
        m_status = 0;
    }
    return ret;
}

int UdpSocket::recv_from(void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t *addrlen)
{
    return ::recvfrom(m_socket, buf, buflen, flag, src_addr, addrlen);
}

int UdpSocket::send_to(const void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t addrlen)
{
    return ::sendto(m_socket, buf, buflen, flag, src_addr, addrlen);
}

void UdpSocket::setnoblock()
{
    int old_option = fcntl(m_socket, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(m_socket, F_SETFL, new_option);
}

void UdpSocket::setkeepalive( int interval)
{
}

int UdpSocket::add_read_event2EpollLoop(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not add tcp socket to epool loop, local port is %d, reason %d ", m_port, errno);
    }
    return ret;
}

int UdpSocket::modify_write_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop, local port is %d, reason %d ", m_port, errno);
    }
    return ret;
}

int UdpSocket::modify_read_event2Epoll(int ep_fd, void* event_data)
{
    int ret = 0;
    struct epoll_event event;
 
    memset(&event, 0, sizeof(event));
    event.data.ptr = event_data;
    event.events = EPOLLIN | EPOLLRDHUP;
 
    ret = epoll_ctl(ep_fd, EPOLL_CTL_MOD, m_socket, &event);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify tcp socket to epool loop, local port is %d, reason %d ", m_port, errno);
    }
    return ret;
} 

int UdpSocket::delSocketFromEpollLoop(int ep_fd)
{
    int ret = 0;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_DEL, m_socket, NULL);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify udp socket to epool loop, local port is %d, reason %d ", m_port, errno);
    }
    return ret;
    
}

int UdpSocket::getlocalPort()
{
    return m_port;
}
