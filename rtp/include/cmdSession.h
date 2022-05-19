#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

#include "rtpepoll.h"
#include "sessionKey.h"
#include "rtpEnum.h"
#include "cstr.h"

#include <map>
#include <string>
#include <list>

class CmdSessionState;
class PingCheckArgs;
class StateCheckArgs;



class LastCookie
{
public:
    LastCookie(const char* key, int key_len);
    virtual ~LastCookie();

public:
    cstr          m_cookie;
    cstr          m_resp;
    unsigned long m_cookie_id;
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
    virtual int processSdpResp(Sdp_session* sdp, RTPDirection direction);
    virtual void resetCookie(const char* cookie, int len);
    virtual int sendPongResp();
    void setSocketInfo(Epoll_data* data);
    void getCmdValueByStrKey(const char* key_c, std::string** v);
    int doAction2PrepareSend();
    int sendcmd(const char* cmdmsg, int len);
    int sendcmd(std::string* cmdmsg);
    void rmSocketInfo();
    int flushmsgs();
    int process_cookie(const char* cookie, int cookie_len);
    int cache_cookie_resp(const char* cookie, int cookie_len, const char* resp, int resp_len);
    int resp_cookie_cache_with_newcookie(const char* cookie, int cookie_len);
public:
    SessionKey*               m_session_key;
    cstr                      m_cookie;       // cookie in cmd str, but we don't using this as the session key in none call session or call session
    unsigned int              m_state_check_count;
protected:
    CmdSessionState*          m_css;
private:
    int parsingCmd(char* cmd, int len);
    int getCmd();

private:
    Epoll_data*               m_socket_data;
    LastCookie*               m_last_cookie;
    cdmParameters_map         m_cmdparams;
    MsgSend_l                 m_sendmsgs_l;
};

class NoneCallCmdSession : public CmdSession
{
public:
    NoneCallCmdSession();
    virtual ~NoneCallCmdSession();
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
    virtual int sendPongResp();

private:
};

class CallCmdSession : public CmdSession
{
public:
    CallCmdSession();
    CallCmdSession(char* cookie);
    virtual ~CallCmdSession();
    virtual int setSdp(int type, Sdp_session*sdp);
    virtual int getSdp(int type, Sdp_session**sdp);
    virtual int checkState(StateCheckArgs* stateArgs);
    virtual int processSdpResp(Sdp_session* sdp, RTPDirection direction);
public:
    Sdp_session* external_peer_sdp;
    Sdp_session* external_local_sdp;
    Sdp_session* internal_peer_sdp;
    Sdp_session* internal_local_sdp;
};

#endif
