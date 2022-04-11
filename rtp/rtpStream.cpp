#include "rtpsession.h"
#include "rtpSendRecvProcs.h"

RtpStream::RtpStream(RtpSession* rtp_session)
{
    m_socket = NULL;
    m_rtpSession = rtp_session;
    m_bridged = 0;
}

RtpStream::~RtpStream()
{
    if(m_socket)
    {
        delete m_socket;
        m_socket = NULL;
    }
    m_rtpSession = NULL;
}

unsigned short RtpStream::getLocalPort()
{
    return m_socket->getlocalPort();
}

int RtpStream::set_local_rtp_network(const char* local_ip, int type, RTPDirection direction)
{
    if(type == IPV4)
    {
        int epoll_fd = m_rtpSession->m_rtp_sendrecv_process->getEpoll_fd();
        if(m_socket)
        {
            delete m_socket;
        }
        m_socket = new UdpSocket(local_ip, direction);
        if(0 == m_socket->getStatus())
        {
            return -1;
        }
        m_direction = direction;
        m_socket->add_read_event2EpollLoop(epoll_fd, this);
        return 0;
    }
    return -1;
}

int RtpStream::set_remote_peer_rtp_network(Network_address* remote_perr_addr)
{
    if(remote_perr_addr)
    {
        m_addr_peer = *remote_perr_addr;
        return 0;
    }
    return -1;
}

int RtpStream::send(const unsigned char* buf, int len)
{
    return 0;
}

int RtpStream::recv(unsigned char* buf, int len)
{
    return 0;
}
