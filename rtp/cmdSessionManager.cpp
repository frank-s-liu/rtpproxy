#include "cmdSessionManager.h"
#include "cmdSession.h"
#include "hash.h"

#include <errno.h>
#include <map>

typedef std::map<long, CmdSession*> cdm_session_map;

static long CMD_PING = BKDRHash("ping", 4);
static long CMD_OFFER = BKDRHash("offer",5);
static long CMD_ANSWER = BKDRHash("answer",6);
static long CMD_DELETE = BKDRHash("delete",6);
static long CMD_QUERY = BKDRHash("query",5);

static cdm_session_map s_sessions_callid;
//static cdm_session_map s_sessions_fromtag;

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
        bencode_item_t* dict;
        bencode_item_t* call_id_item;
        long cmd_case = 0;
        long call_id = 0;
        CmdSession* cs = NULL;
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
        cmd_case = BKDRHash(cmd_item->iov[1].iov_base, cmd_item->iov[1].iov_len);
        switch (cmd_case)
        {
            case CMD_PING:
            {
                char result_str[256];
                snprintf(result_str, sizeof(result_str), "%s d6:result4:ponge", cookie);
                m_connection->sendto(result_str, strlen(result_str), 0, (struct sockaddr*)&client_addr, &socket_len);
                break;
            }
            case CMD_OFFER:
            case CMD_ANSWER:
            case CMD_DELETE:
            {
                call_id_item = bencode_dictionary_get(dict, "call-id");
                if (!call_id_item || call_id_item->type != BENCODE_STRING)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "don't have call_id parameters");
                    ret = 1;
                    goto retpoint;
                }
                call_id = BKDRHash(call_id_item->iov[1].iov_base, call_id_item->iov[1].iov_len);
                cdm_session_map::iterator ite = s_sessions_callid.find(call_id);
                if(ite != s_sessions_callid.end())
                {
                    cs = s_sessions_callid[call_id];
                }
                break;
            }
            case CMD_QUERY:
            default:
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unsupport command:%s", cmd_item->iov[1].iov_base);
                ret = 1;
                goto retpoint;
            }
        }
        
        switch (cmd_case)
        {
            case CMD_OFFER:
            {
                if(!cs)
                {
                    cs = new CmdSession();
                    s_sessions_callid[call_id] = cs;
                }
                ret = cs->process_cmd(OFFER_CMD);
                break;
            }
            case CMD_ANSWER:
            {
                if(!cs)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "don't find call id:[%s] when process ANSWER cmd session", call_id_item->iov[1].iov_base);
                    ret = 1;
                    goto retpoint;
                }
                ret = cs->process_cmd(ANSWER_CMD);
                break;
            }
            case CMD_DELETE:
            {
                if(!cs)
                {
                    tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "don't find call id:[%s] when process delete cmd session", call_id_item->iov[1].iov_base);
                    ret = 1;
                    goto retpoint;
                }
                ret = cs->process_cmd(DELETE_CMD);
                break;
            }
            case CMD_QUERY:
            default:
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "unknon issue, should not here");
                ret = 1;
                goto retpoint;
            }
        }

        //bencode_dictionary_add_string(resp, "result", resultstr);
        //cmd_resp = bencode_collapse(resp, &response_len);

        //m_connection->sendto(cmd_str, sizeof(cmd_str), 0, (struct sockaddr*)&client_addr, &socket_len);
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
