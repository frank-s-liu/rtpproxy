#ifndef SOCKET_H_
#define SOCKET_H_

#ifdef _WIN32
    #include <winsock2.h>
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>

#ifdef __linux__
    typedef int r_socket;
#else 
    typedef SOCKET r_socket;
#endif


#ifdef __linux__
    #ifdef INVALID_SOCKET
        #undef INVALID_SOCKET
        #define INVALID_SOCKET -1
    #else
        #define INVALID_SOCKET -1
    #endif 
#endif // __linux__

enum IpVersion
{
    IPV4 = 0,
    IPV6
};


class Socket
{
public:
    Socket(int type, int ipver);
    virtual ~Socket();
    virtual int send_to(const void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t addrlen=0);
    virtual int recv_from(void* buf, size_t buflen, int flag=0, struct sockaddr *src_addr=NULL, socklen_t *addrlen=NULL);
    virtual int add_read_event2EpollLoop(int ep_fd, void* event_data);
    virtual int modify_write_event2Epoll(int ep_fd, void* event_data);
    virtual int modify_read_event2Epoll(int ep_fd, void* event_data) = 0;
    virtual int delSocketFromEpollLoop(int ep_fd)=0;
    virtual void setnoblock() = 0;
    virtual void setkeepalive(int interval) = 0;
    virtual int connect_to(const char* ip, int port);
    virtual unsigned short getlocalPort() = 0;
    
private:
    Socket& operator = (const Socket& other);
    Socket(const Socket& socket);

protected:
    r_socket m_socket;
    int m_type;
    int m_IPver;
};


#endif

