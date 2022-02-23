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
    BENCODE_NOT_COMPLETED,
    BENCODE_SPLICE,
    BENCODE_ERRNO_MAX
}BENCODE_ERRNO;

int parseBencodeCmd(char* cmdstr)
{
    char* start = cmdstr;
    char* cookie = strstr(start, " d");
    if(cookie)
    {
        *cookie = '\0';
        SessionKey* sk = new SessionKey(cookie);
        *cookie = ' ';
        CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(sk);
        if(!cs)
        {
            cs = new CmdSession();
            cs->m_session_key = sk;
            CmdSessionManager::getInstance()->putinCmdSession(cs);
        }
    }
    else
    {
        
    }
    return 0;   
}

int parsingString(char* bencode_str_start, char** bencode_str_end)
{
    char* p = bencode_str_start;
    char* begin = p;
    int cmdlen = 0;
    while(*p)
    {
        if(*p>='0' && *p<='9')
        {
            p++;
        }
        else
        {
            break;
        }
    } 
    if(*p != ':')
    {
        if(*p == '\0')
        {
            return BENCODE_NOT_COMPLETED;
        }
        return FORMAT_ERR;
    }
    else
    {
        *p = '\0';
        int len = atoi(begin);
        if(len <= 0)
        {
            return FORMAT_ERR;
        }
        *p = ':';
        cmdlen = strlen(p);
        if(cmdlen <= len)
        {
            return BENCODE_NOT_COMPLETED;
        }
        p += (len+1);
        *bencode_str_end = p;
    }
    return SUCCESS;
}

int parsingInt(char* bencode_str_start, char** bencode_str_end)
{
    char* p = bencode_str_start;
    if(*p != 'i')
    {
        return FORMAT_ERR;
    }
    else
    {
        p++;
        while(*p)
        {
            if(*p>='0' && *p<='9')
            {
                p++;
            }
            else
            {
                break;
            }
        }
        if(*p != 'e')
        {
            if(*p == '\0')
            {
                return BENCODE_NOT_COMPLETED;
            }
            return FORMAT_ERR;
        }
        else
        {
            *bencode_str_end = p+1;
        }
    }
    return SUCCESS;
}

int tcpBencodeCheck(char* cmdstr, char** end)
{
    char* p = cmdstr;
    char* begin = p;
    char* cookie = strstr(p, " d");
    if(cookie)
    {
        p = cookie+2;
        begin = p;
        while(*p)
        {
            int ret = parsingString(p, &begin); // string type key
            if(SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                p = begin;
            }
            if(*p>='0' && *p<='9')// string type value
            {
                ret = parsingString(p, &begin);
            }
            else if(*p == 'i') // int type value
            {
                ret = parsingInt(p, &begin);
            }
            else
            {
                return FORMAT_ERR;
            }
            if(SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                p = begin;
            }
            if(*p == 'e')
            {
                char tmp = '\0';
                p++;
                if(*p != '\0')
                {
                    tmp = *p;
                    *p = '\0';
                }
                parseBencodeCmd(cmdstr);
                *p = tmp;
                *end = p;
                return SUCCESS;
            }
            else
            {
                continue;
            }
        }
        return BENCODE_NOT_COMPLETED;
    }
    else
    {
        return FORMAT_ERR;
    }
    return SUCCESS;
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
        char* end = NULL;
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
            delete[] socket->cmd_not_completed;
            socket->cmd_not_completed = NULL;
        }
        end = cmd;

        while(end && *end != '\0')
        {
            ret = tcpBencodeCheck(cmd, &end);
            if(ret == SUCCESS)
            {
                cmd = end;
                continue;
            }
            else if(ret == BENCODE_NOT_COMPLETED)
            {
                int len = strlen(cmd);
                socket->cmd_not_completed = new char[len+1];
                snprintf(socket->cmd_not_completed, len+1, "%s", cmd);
                tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "recv bencode cmd not completed");
                break;
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "recv bencode cmd str format error, [%s]", cmd);
                goto errprocess;
            }
        }
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

