#include "rtpepoll.h"
#include "log.h"


int tcpRecvBencode(SocketInfo* socket)
{
    char bufer[2048];
    char* cmd = NULL;
    char* cmdnew = NULL;
    int len = 0;
    int ret = 0;
    len = recv(socket->fd, bufer, sizeof(bufer)-1, 0);
    if(len > 0)
    {
        bufer[len] = '\0';
        if(!socket->data)
        {
            cmd = buffer;
        }
        else
        {
            int l = strlen((const char*)socket->data);
            l += len;
            cmdnew = new char[l+1];
            snprintf(cmdnew, l+1, "%s%s",(const char*)socket->data, bufer); 
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd, tcp packet splicing");
            cmd = cmdnew;
        }
    }
    else if(len == 0)
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error, peer side close socket");
        goto errprocess;
    }
    else
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error,  errno is %d", errno);
        goto errprocess;
    }

errprocess:
    close(socket->fd);
    socket->fd = -1;
    socket->fd_tcp_state = CLOSED;
    if(cmdnew)
    {
        delete[] cmdnew;
    }
    return -1;
}

int parseBencodeCmd(char* cmdstr)
{
    
}
