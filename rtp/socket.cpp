#include "socket.h"
#include "transport.h"
#include "log.h"

//std include
#ifdef __linux__
    #include <sys/types.h>  
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    //#include <> // INVALID_SOCKET
    #include <unistd.h> // close()
    #include <errno.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>



Socket::Socket(int type, int ipVer)
{
    m_type = type;
    m_IPver = ipVer;
    switch (type)
    {
       case UDP:
       {
           m_socket = ::socket(ipVer == IPV4 ? PF_INET : PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
           break;
       }
       case TCP:
       case TLS:
       {
           m_socket = ::socket(ipVer == IPV4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
           break;
       }
       default:
       {
         assert(NULL);
       }
   }

   if (m_socket == INVALID_SOCKET)
   {
      assert(0);
   }

#ifdef __linux__
   int on = 1;
   if (ipVer == IPV6)
   {
      if (::setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) )
      {
          exit(errno);
      }
   }
#endif

}

Socket::~Socket()
{
    if(m_socket != INVALID_SOCKET)
    {
        //tracelog("TRANSPORT", WARNING_LOG, __FILE__, __LINE__, "close socket ");
        close(m_socket);
        m_socket = INVALID_SOCKET;
    }
}


int Socket::recv_from(void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t *addrlen)
{
    return 0;
}

int Socket::send_to(const void* buf, size_t buflen, int flag, struct sockaddr *src_addr, socklen_t addrlen)
{
    return 0;
}


int Socket::add_read_event2EpollLoop(int ep_fd, void* eventdata)
{
    return 0;
}

int Socket::modify_write_event2Epoll(int ep_fd, void* eventdata)
{
    return 0;
}
int Socket::connect_to(const char* ip, int port)
{
    return 0;
}

