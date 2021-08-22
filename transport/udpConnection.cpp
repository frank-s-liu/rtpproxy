#include "udpConnection.h"
#include "connection.h" 
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
#include <stdio.h>

UdpConnection::UdpConnection(const char* local_ip):Socket(UDP, IPV4)
{
    m_port = -1;
    m_status = -1;
    snprintf(m_local_ip, sizeof(m_local_ip), "%s", local_ip);
}

UdpConnection::UdpConnection(const char* local_ip, int port):Socket(UDP, IPV4)
{
    m_port = port;
    m_status = -1;
    snprintf(m_local_ip, sizeof(m_local_ip), "%s", local_ip);
}

UdpConnection::~UdpConnection()
{
        
}


int UdpConnection::bind(const char* local_ip)
{
    struct sockaddr_in addr;
    int ret = 1;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(local_ip);
    addr.sin_port = htons(0);
    ret = ::bind(m_socket, (struct sockaddr *) &addr, sizeof(addr));
    if(0 == ret)
    {
        struct sockaddr_in sockaddr;
        int len = sizeof(sockaddr);
        getsockname(m_socket, (struct sockaddr *) &sockaddr, (socklen_t *) &len); 
        m_port = ntohs(sockaddr.sin_port); 
        m_status = 1;
    }
    else
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not bind one port to socket, error %d", errno);
    }
    return ret;
}

int UdpConnection::bind(const char* local_ip, int port)
{
    struct sockaddr_in addr;
    int ret = 1;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(local_ip);
    addr.sin_port = htons(port);
    ret = ::bind(m_socket, (struct sockaddr *) &addr, sizeof(addr));
    if(0 == ret)
    {
        m_port = port;
        m_status = 1;
    }
    else
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not bind port %d to socket, error %d", port, errno);
    }
    return ret;
}


int UdpConnection::recv_from(void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t *addrlen)
{
    return ::recvfrom(m_socket, buf, buflen, flag, src_addr, addrlen);
}

int UdpConnection::send_to(const void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t addrlen)
{
    return ::sendto(m_socket, buf, buflen, flag, src_addr, addrlen);
}

void UdpConnection::setnoblock()
{
    int old_option = fcntl(m_socket, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(m_socket, F_SETFL, new_option);
}

void UdpConnection::setkeepalive( int interval)
{
}

int UdpConnection::add_read_event2EpollLoop(int ep_fd, void* event_data)
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

int UdpConnection::modify_write_event2Epoll(int ep_fd, void* event_data)
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

int UdpConnection::modify_read_event2Epoll(int ep_fd, void* event_data)
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

int UdpConnection::delSocketFromEpollLoop(int ep_fd)
{
    int ret = 0;
    ret = epoll_ctl(ep_fd, EPOLL_CTL_DEL, m_socket, NULL);
    if(ret < 0) 
    {
        tracelog("TRANSPORT", ERROR_LOG, __FILE__, __LINE__, "can not modify udp socket to epool loop, local port is %d, reason %d ", m_port, errno);
    }
    return ret;
    
}

int UdpConnection::getlocalPort()
{
    return m_port;
}
