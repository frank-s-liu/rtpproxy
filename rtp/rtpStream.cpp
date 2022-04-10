#include "rtpsession.h"

RtpStream::RtpStream()
{
    m_socket = NULL;
}

RtpStream::~RtpStream()
{
    if(m_socket)
    {
        delete m_socket;
        m_socket = NULL;
    }
}

int RtpStream::set_local_rtp_network(const char* local_ip, int type, RTPDirection direction)
{
    if(type == IPV4)
    {
        if(m_socket)
        {
            delete m_socket;
        }
        m_socket = new UdpSocket(local_ip, direction);
        if(0 == m_socket->getStatus())
        {
            return -1;
        }
        return 0;
    }
    return -1;
}

int RtpStream::set_remote_peer_rtp_network(Network_address* remote_perr_addr)
{
    if(remote_perr_addr)
    {
        m_addr_peer = *remote_perr_addr;
    }
}

int RtpStream::send(const unsigned char* buf, int len)
{
    return 0;
}

int RtpStream::recv(unsigned char* buf, int len)
{
    return 0;
}
