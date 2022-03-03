#include "rtpepoll.h"
#include "log.h"
#include "cmdSession.h"
#include "cmdSessionManager.h"
#include "bencode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


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
            tracelog("RTP", INFO_LOG,__FILE__, __LINE__, "new cmd session, cookie is [%s]", sk->m_cookie);
        }
        else
        {
            delete sk;
        }
        cs->process_cmd(cookie+2);
    }
    else
    {
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "new cmd session, no cookie");
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

int bencodeCheck(char* cmdstr, char** end)
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
                //ret = parsingInt(p, &begin);
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "not support int type value");
                return FORMAT_ERR;
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


