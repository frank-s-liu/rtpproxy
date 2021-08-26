#include "cmdSessionManager.h"
#include "cmdSession.h"
#include "hash.h"

#include <errno.h>
#include <map>

typedef std::map<long, CmdSession*> cdm_session_map;

static int CMD_PING = BKDRHash("ping", 4);
static int CMD_OFFER = BKDRHash("offer",5);
static int CMD_ANSWER = BKDRHash("answer",6);
static int CMD_DELETE = BKDRHash("delete",6);
static int CMD_QUERY = BKDRHash("query",5);


CmdSessionManager::CmdSessionManager(const char* localip, int localPort)
{
    m_connection = new udpConnection(localip, localPort);
}

CmdSessionManager::~CmdSessionManager()
{
    delete m_connection;
    m_connection = NULL;
}

int CmdSessionManager::processCmd()
{
    char cmd_str[4096];
    struct sockaddr_in client_addr;
    socklen_t socket_len = sizeof(client_addr); 
    int len = m_connection->recv_from(cmd_str, sizeof(cmd_str), 0, (struct sockaddr*)&client_addr, &socket_len);
    int ret = 0;
    bencode_buffer_t buffer;
    if(len > 0 && len < sizeof(cmd_str))
    {
        char* cookie = cmd_str;
        char* cmd = strchr(cmd_str, ' ');
        const char* result_str = "OK";
        bencode_item_t* dict;
        if(!cmd)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "invalid cmd str [%s]", cmd_str);
            ret = 1;
            goto retpoint;
        }
        else
        {
            if(*cookie == ' ')
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "invalid cmd str, no cookie. [%s]", cmd_str);
                ret = 1;
                goto retpoint;
            }
            *cmd = '\0';
            if( strlen(cookie) == (len-1) )
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "invalid cmd str, no bencode str. [%s]", cmd_str);
                ret = 1;
                goto retpoint;
            }
            cmd++;
        }
        ret = bencode_buffer_init(&buffer);
        if(ret != 0)
        {
            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "bencode_buffer_init error %d", ret);
            goto retpoint;
        }
        bencode_item_t* resp = bencode_dictionary(&buffer);
        if(!resp)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "bencode_dictionary error");
            ret = 1;
            goto retpoint;
        }
        dict = bencode_decode_expect(&buffer, cmd, strlen(cmd), BENCODE_DICTIONARY);
        if(!dict)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "bencode_decode_expect_str error");
            ret = 1;
            goto retpoint;
        }
        
        bencode_item_t* cmd_item;
        cmd_item = bencode_dictionary_get(dict, "command");
        if (!cmd_item || cmd_item->type != BENCODE_STRING)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "don't have command parameters");
            ret = 1;
            goto retpoint;
        }
        long cmd_case = BKDRHash(cmd_item->iov[1].iov_base, cmd_item->iov[1].iov_len);
        switch (cmd_case)
        {
            case CMD_PING:
            {
                result_str = "pong";
                break;
            }
            case CMD_OFFER:
            {
                break;
            }
            case CMD_ANSWER:
            {
                break;
            }
            case CMD_DELETE:
            {
                break;
            }
            case CMD_QUERY:
            {
                break;
            }
            default:
            {
              tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unsupport command:%s", cmd_item->iov[1].iov_base);
              goto retpoint;
            }
        }
        
        bencode_dictionary_add_string(resp, "result", resultstr);
        ret = 0;
        goto retpoint;
    }
    else if(len == sizeof(cmd_str))
    {
        ret = 2;
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, " cmd is constains too many characters, max size is %d, cmd[%s]", sizeof(cmd_str), cmd_str);
    }
    else if(len <= 0 && errno != EAGAIN)
    {
        ret = 3;
        tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, " CmdSessionM recv error, %d", errno);
    }

retpoint:
    bencode_buffer_free(&buffer);
    return ret;
}

int CmdSessionManager::sendCmdResp()
{

}
