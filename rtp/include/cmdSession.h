#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

#include "rtpepoll.h"

#include <map>
#include <string>
#include <list>

class CmdSessionState;
class PingCheckArgs;
class StateCheckArgs;

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
    SessionKey(const char* cookie);
    SessionKey(const char* key, int key_len);
    virtual ~SessionKey();
    bool operator <(const SessionKey& s) const ;

public:
    char*         m_cookie;
    unsigned long m_cookie_id;
    int           m_cookie_len;
};

class LastCookie
{
public:
    LastCookie(const char* key, int key_len);
    virtual ~LastCookie();

public:
    char*         m_cookie;
    char*         m_resp;
    unsigned long m_cookie_id;
    int           m_cookie_len;
};

typedef std::map<std::string, std::string*> cdmParameters_map;
typedef std::list<std::string*> MsgSend_l;

class Sdp_session;
class CmdSession
{
public:
    CmdSession();
    CmdSession(char* cookie);
    virtual ~CmdSession();
    virtual int setSdp(int type, Sdp_session* sdp);
    virtual int getSdp(int type, Sdp_session**sdp);
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
    virtual int checkState(StateCheckArgs* stateArgs);
    virtual int process_cmd(char* cmdstr);
    virtual int process_cmd(int cmd);
    void setSocketInfo(Epoll_data* data);
    void getCmdValueByStrKey(const char* key_c, std::string** v);
    int doAction2PrepareSend();
    int sendPongResp();
    int sendcmd(const char* cmdmsg);
    int sendcmd(std::string* cmdmsg);
    void rmSocketInfo();
    int flushmsgs();
    int process_cookie(const char* cookie, int cookie_len);

public:
    SessionKey*         m_session_key;
     

private:
    int parsingCmd(char* cmd, int len);
    int getCmd();

private:
    CmdSessionState*          m_css;
    Epoll_data*               m_socket_data;
    LastCookie*               m_last_cookie;
    cdmParameters_map         m_cmdparams;
    MsgSend_l                 m_sendmsgs_l;
};

class CallCmdSession : public CmdSession
{
public:
    CallCmdSession();
    CallCmdSession(char* cookie);
    virtual ~CallCmdSession();
    virtual int setSdp(int type, Sdp_session*sdp);
    virtual int getSdp(int type, Sdp_session**sdp);
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
    virtual int checkState(StateCheckArgs* stateArgs);
public:
    Sdp_session* external_peer_sdp;
    Sdp_session* external_local_sdp;
    Sdp_session* internal_peer_sdp;
    Sdp_session* internal_local_sdp;
};

#endif
