#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

#include "rtpepoll.h"

class CmdSessionState;

typedef enum cmd_type
{
    OFFER_CMD=0,
    ANSWER_CMD,
    DELETE_CMD,
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

class CmdSession
{
public:
    CmdSession();
    CmdSession(char* cookie);
    virtual ~CmdSession();
    void set_client_addr(char* ip, int port);
    int process_cmd(int cmd);
    void setSocketInfo(SocketInfo* info);

public:
    long  m_call_id;
    SessionKey*         m_session_key;
     

private:
    CmdSessionState*    m_css;
    SocketInfo*         m_socketInfo;
    int                 m_direction;
    int                 m_cmd;
    int                 m_client_port;
};

#endif
