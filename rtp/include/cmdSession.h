#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

#include "rtpepoll.h"

#include <map>
#include <string>

class CmdSessionState;
class PingCheckArgs;

typedef enum cmd_type
{
    OFFER_CMD=0,
    ANSWER_CMD,
    DELETE_CMD,
    PING_CMD,

//  internal cmd
    PING_CHECK_CMD,
    MAX_CONTROL_CMD
}CONTROL_CMD;

class SessionKey
{
public:
    SessionKey(char* cookie);
    virtual ~SessionKey();
    bool operator <(const SessionKey& s) const ;

public:
    char*         m_cookie;
    unsigned long m_cookie_id;
    int           m_cookie_len;
};

typedef std::map<std::string, std::string*> cdmParameters_map;

class CmdSession
{
public:
    CmdSession();
    CmdSession(char* cookie);
    virtual ~CmdSession();
    int process_cmd(char* cmdstr);
    int process_cmd(int cmd);
    void setSocketInfo(Epoll_data* data);
    int getCmdValueByStrKey(const char* key);
    int checkPingKeepAlive(PingCheckArgs* pingArg);    

public:
    SessionKey*         m_session_key;
     

private:
    int parsingCmd(char* cmd, int len);
    int getCmd();

private:
    CmdSessionState*    m_css;
    Epoll_data*         m_socket_data;
    cdmParameters_map   m_cmdparams;
};

#endif
