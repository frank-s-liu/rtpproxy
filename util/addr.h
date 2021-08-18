#ifndef __SOCKET_ADDR_H__
#ifndef __SOCKET_ADDR_H__ 

#ifdef __linux__
    #include <netinet/in.h>
#endif

//#define MAX_SOCKADDR_SIZE 28

union Socket_Addr
{
       sockaddr mSockaddr;
       sockaddr_in m_anonv4;
       sockaddr_in6 m_anonv6;
       //char pad[MAX_SOCKADDR_SIZE]; //< this make union same size if v6 is in or out
};


#endif
