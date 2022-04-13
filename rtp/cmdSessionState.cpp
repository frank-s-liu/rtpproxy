#include "cmdSessionState.h"
#include "cmdSession.h"
#include "rtpControlProcess.h"
#include "log.h"
#include "args.h"
#include "task.h"
#include "sdp.h"
#include "rtpLB.h"


#include <stdlib.h>


static const char* StateName[CMDSESSION_MAX_STATE] = {"CMDSESSION_STATE", "CMDSESSION_INIT_STATE", "CMDSESSION_OFFER_PROCESSING_STATE", "CMDSESSION_OFFER_PROCESSED_STATE"};
static const char* CMD_STR[MAX_CONTROL_CMD] = {"OFFER_CMD", "ANSWER_CMD", "DELETE_CMD", "PING_CMD", "PING_CHECK_CMD"};

// timer thread call back
static void processPingCheck(void* args)
{
    PingCheckArgs* pingArg = (PingCheckArgs*)args;
    ControlProcess::getInstance()->add_pipe_event(pingArg);
}

static void processStateCheck(void* args)
{
    StateCheckArgs* stateArgs = (StateCheckArgs*)args;
    ControlProcess::getInstance()->add_pipe_event(stateArgs);
}

CmdSessionState::CmdSessionState(CmdSession* cs)
{
    m_cs = cs;
    m_state = CMDSESSION_STATE;
}

CmdSessionState::~CmdSessionState()
{
    m_cs = NULL;
}

int CmdSessionState::checkPingKeepAlive(PingCheckArgs* pingArg)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"must not process ping in cmd state %s in cmd session %s", StateName[m_state], m_cs->m_session_key->m_cookie);
    return -1;
}

int CmdSessionState::checkState(StateCheckArgs* stateArg)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"must not check state in cmd state %s in cmd session %s", StateName[m_state], m_cs->m_session_key->m_cookie);
    return -1;
}

int CmdSessionState::processSdpResp(Sdp_session* sdp, RTPDirection direction)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"must not processSdpResp in cmd state %s in cmd session %s", StateName[m_state], m_cs->m_session_key->m_cookie);
    return -1;
}

CmdSessionInitState::CmdSessionInitState(CmdSession* cs):CmdSessionState(cs)
{
    m_count = 0;
    m_state = CMDSESSION_INIT_STATE;
}

CmdSessionInitState::~CmdSessionInitState()
{
}

int CmdSessionInitState::checkPingKeepAlive(PingCheckArgs* pingArg)
{
    if(pingArg->ping_recv_count == m_count)
    {
        return -1;
    }
    return 0;
}

int CmdSessionInitState::processCMD(int cmd, CmdSessionState** nextState)
{
    int ret = 0;
    Sdp_session* sdp = NULL;
    SDPArgs* sdpArg = NULL;
    switch (cmd)
    {
        case OFFER_CMD:
        {
           std::string* v = NULL;
           std::string* direction = NULL;
           m_cs->getCmdValueByStrKey("sdp", &v);
           m_cs->getCmdValueByStrKey("direction", &direction);
           if(v && direction)
           {
               RTPDirection dir = MAX_DIRECTION;
               sdp = new Sdp_session();
               sdp->parse(v->c_str(), v->length());
               std::string dir1("8:external8:internal");
               std::string dir2("8:internal:8:external");
               if(*direction == dir1)
               {
                   dir = EXTERNAL_PEER;
               }
               else if(*direction == dir2)
               {
                   dir = INTERNAL_PEER;
               }
               else
               {
                   tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "err direction %s for cmd session: %s to process cmd %s", 
                                                                   direction->c_str(), m_cs->m_session_key->m_cookie, CMD_STR[OFFER_CMD]);
                   goto err_ret;
               }
               sdpArg = new SDPArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
               sdpArg->sdp = sdp;
               sdpArg->direction = dir;
               if(0 != processSdpArgs(sdpArg))
               {
                   tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"process cmd %s error because of sdp args error in cmd session %s ", 
                                                                   CMD_STR[OFFER_CMD], m_cs->m_session_key->m_cookie);
                   goto err_ret;
               }
           }
           else
           {
               tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no sdp or no direction in cmd session %s, to process cmd of OFFER_CMD", m_cs->m_session_key->m_cookie);
               goto err_ret;
           }
           *nextState = new CmdSessionOfferProcessingState(m_cs);
           StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
           args->state = CMDSESSION_OFFER_PROCESSING_STATE;
           if(0 != add_task(1600, processStateCheck, args))
           {
               delete args;
               tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
           }
           ret = 0;
           break;
        }
        case ANSWER_CMD:
        case DELETE_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s must not process cmd %s in CmdSessionInitState", m_cs->m_session_key->m_cookie, CMD_STR[cmd]);
            goto err_ret;
        }
        case PING_CMD:
        {
            m_count++;
            PingCheckArgs* args = new PingCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            args->ping_recv_count = m_count;
            //args->cmdtype = PING_CHECK_CMD;
            if(0 != add_task(8000, processPingCheck, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s when processing cmd of PING_CMD", 
                                                                 m_cs->m_session_key->m_cookie);
            }
            ret = m_cs->sendPongResp();
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s process unknown cmd %d in CmdSessionInitState", m_cs->m_session_key->m_cookie, cmd);
            goto err_ret;
        }
    }
    return ret;

err_ret:
    if(sdp)
    {
        delete sdp;
    }
    if(sdpArg)
    {
        delete sdpArg;
    }
    ret = -1;
    *nextState = NULL;
    return ret;
}

CmdSessionOfferProcessingState::CmdSessionOfferProcessingState(CmdSession* cs):CmdSessionState(cs)
{
    m_state = CMDSESSION_OFFER_PROCESSING_STATE;
}

CmdSessionOfferProcessingState::~CmdSessionOfferProcessingState()
{
}

int CmdSessionOfferProcessingState::processCMD(int cmd, CmdSessionState** nextState)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"currently not process cmd in state of %s, call id %s", StateName[m_state], m_cs->m_session_key->m_cookie);
    return -1;
}

int CmdSessionOfferProcessingState::checkState(StateCheckArgs* stateArg)
{
    if(stateArg->state >= m_state)
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"cmd session %s check state failed in cmd session state of %s", m_cs->m_session_key->m_cookie, StateName[m_state]);
        return -1;
    }
    return 0;
}

// in msg replied to SIP proxy, it doesn't need to add direction info, So don't use direction parameter
int CmdSessionOfferProcessingState::processSdpResp(Sdp_session* sdp, RTPDirection direction)
{
    char resp[2048];
    int len = 0;
    int ret = 0;
    int max_sdp_len_reserver = 5; // example 1234:v=0, "1234:" is the max_sdp_len_reserver
    int len_reserve = m_cs->m_session_key->m_cookie_len + strlen(" d3:sdp")+max_sdp_len_reserver;
    int real_reserver = 0;
    int delta = 0;
    ret = sdp->serialize(&resp[len_reserve], &len);
    if(0 != ret)
    {
        return -1;
    }
    if(len>=1000)
    {
        max_sdp_len_reserver = 5;
    }
    else if(len >= 100)
    {
        max_sdp_len_reserver = 4;
    }
    else if(len >= 10)
    {
        max_sdp_len_reserver = 3;
    }
    else
    {
        max_sdp_len_reserver = 2;
    }
    real_reserver = m_cs->m_session_key->m_cookie_len + strlen(" d3:sdp")+max_sdp_len_reserver;
    delta = len_reserve - real_reserver;
    len = snprintf(&resp[delta], real_reserver, "%s d3:sdp%d", m_cs->m_session_key->m_cookie, len);
    resp[delta+len]=':';
    tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"sdp resp msg [%s] from direction of %d", &resp[delta], direction);
    return m_cs->sendcmd(&resp[delta]);
}





