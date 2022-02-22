#include "rtpepoll.h"
#include "log.h"
#include "cmdSession.h"
#include "cmdSessionManager.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef enum bencode_error
{
    SUCCESS = 0,
    FORMAT_ERR,
    TOO_LONG_STR,
    BENCODE_ERRNO_MAX
}BENCODE_ERRNO;

int parseBencodeCmd(char* cmdstr)
{
    char* start = cmdstr;
    char* cookie = strstr(start, " d");
    if(cookie)
    {
        *cookie = '\0';
        //SessionKey* sk = new SessionKey(cookie);
        //CmdSession* cs = new CmdSession(cookie);
        *cookie = ' ';
        //CmdSessionManager* CmdSessionManager::getInstance()->
    }
    else
    {
        
    }
    return 0;   
}


int tcpRecvBencode(Epoll_data* data)
{
    char buffer[2048];
    char* cmd = NULL;
    char* cmdnew = NULL;
    int len = 0;
    int ret = 0;
    SocketInfo* socket = (SocketInfo*)data->data;
    len = recv(socket->fd, buffer, sizeof(buffer)-1, 0);
    if(len > 0)
    {
        buffer[len] = '\0';
        if(!socket->cmd_not_completed)
        {
            cmd = buffer;
        }
        else
        {
            int l = strlen(socket->cmd_not_completed);
            l += len;
            cmdnew = new char[l+1];
            snprintf(cmdnew, l+1, "%s%s",socket->cmd_not_completed, buffer); 
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd, tcp packet splicing");
            cmd = cmdnew;
        }
        parseBencodeCmd(cmd);
        goto retprocess;
    }
    else if(len == 0)
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error, peer side close socket");
        ret = -1;
        goto errprocess;
    }
    else
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "recv bencode cmd error,  errno is %d", errno);
        ret = -1;
        goto errprocess;
    }

errprocess:
    close(socket->fd);
    socket->fd = -1;
    //socket->fd_tcp_state = CLOSED;
retprocess:
    if(cmdnew)
    {
        delete[] cmdnew;
        cmdnew = NULL;
    }
    return ret;
}

