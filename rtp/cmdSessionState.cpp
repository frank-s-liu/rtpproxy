#include "cmdSessionState.h"
#include "cmdSession.h"
#include "rtpControlProcess.h"
#include "log.h"
#include "args.h"
#include "task.h"
#include "sdp.h"
#include "rtpLB.h"
#include "rtpConstStr.h"


#include <stdlib.h>


//static const char* CMD_STR[MAX_CONTROL_CMD] = {"OFFER_CMD", "ANSWER_CMD", "DELETE_CMD", "PING_CMD", "PING_CHECK_CMD"};

// timer thread call back
static void fireArgs2controlProcess_s(void* args)
{
    Args* arg = (Args*)args;
    if(0 != ControlProcess::getInstance()->add_pipe_event(arg))
    {
        delete arg;
    }
}

static int processSdpResp_s(Sdp_session* sdp, const char* cookie, int cookie_len, char* resp, int resp_size, int* resp_offset)
{
    int len = 0;
    int ret = 0;
    int max_sdp_len_reserver = 5; // example 1234:v=0, "1234:" is the max_sdp_len_reserver
    int len_reserve = cookie_len + strlen(" d3:sdp")+max_sdp_len_reserver;
    int real_reserver = 0;
    int delta = 0;
    if(len_reserve >= resp_size)
    {
        return -1;
    }
    len = resp_size-len_reserve;
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
    real_reserver = cookie_len + strlen(" d3:sdp")+max_sdp_len_reserver;
    delta = len_reserve - real_reserver;
    len = snprintf(&resp[delta], real_reserver, "%s d3:sdp%d", cookie, len);
    resp[delta+len]=':';
    *resp_offset = delta;
    len = strlen(&resp[delta]);
    if(resp_size > (delta+len))
    {
        snprintf(&resp[delta]+len, resp_size-delta-len, "6:result2:oke");
    }
    else
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"resp buf is not enough %d %d", resp_size, delta+len);
    }
    return ret;
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
    if(stateArg->state >= m_cs->m_state_check_count)
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"cmd session [%s] check state failed in cmd session state of [%s]", m_cs->m_session_key->m_cookie, StateName[m_state]);
        return -1;
    }
    return 0;
}

int CmdSessionState::processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState)
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

int CmdSessionInitState::checkState(StateCheckArgs* stateArg)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"must not process check state in cmd state %s in cmd session %s", StateName[m_state], m_cs->m_session_key->m_cookie);
    return -1;
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
               std::string dir2("8:internal8:external");
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
                   tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "err direction %s for cmd session: [%s] to process cmd %s", 
                                                                   direction->c_str(), m_cs->m_session_key->m_cookie, CMD_STR[OFFER_CMD]);
                   goto err_ret;
               }
               sdpArg = new SDPArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
               sdpArg->sdp = sdp;
               sdpArg->direction = dir;
               if(0 != processRTPArgs(sdpArg, m_cs->m_session_key->m_cookie_id))
               {
                   tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"process cmd [%s] error because of sdp args error in cmd session [%s] ", 
                                                                   CMD_STR[OFFER_CMD], m_cs->m_session_key->m_cookie);
                   delete sdpArg;
                   sdpArg = NULL;
                   goto err_ret;
               }
           }
           else
           {
               tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no sdp or no direction in cmd session [%s], to process cmd of [OFFER_CMD]", m_cs->m_session_key->m_cookie);
               goto err_ret;
           }
           *nextState = new CmdSessionOfferProcessingState(m_cs);
           m_cs->m_state_check_count++;
           StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
           args->state = m_cs->m_state_check_count;
           if(0 != add_task(16000, fireArgs2controlProcess_s, args))
           {
               delete args;
               tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session [%s]", m_cs->m_session_key->m_cookie);
           }
           ret = 0;
           break;
        }
        case ANSWER_CMD:
        case DELETE_CMD:
        {
            Args* delCmdArg = new DeleteCmdArg(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != add_task(4000, fireArgs2controlProcess_s, delCmdArg))
            {
                delete delCmdArg;
                delCmdArg = NULL;
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add task of delete cmd session task error for cmd session [%s]", m_cs->m_session_key->m_cookie);
            }
            break;
        }
        case PING_CMD:
        {
            m_count++;
            PingCheckArgs* args = new PingCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            args->ping_recv_count = m_count;
            //args->cmdtype = PING_CHECK_CMD;
            if(0 != add_task(8000, fireArgs2controlProcess_s, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session [%s] when processing cmd of PING_CMD", 
                                                                 m_cs->m_session_key->m_cookie);
            }
            ret = m_cs->sendPongResp();
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session [%s] process unknown cmd [%s] in CmdSessionInitState", m_cs->m_session_key->m_cookie, CMD_STR[cmd]);
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
    int ret = 0;
    rtpSendRecvThreadArgs* rtparg = NULL;
    switch(cmd)
    {
        case OFFER_CMD:
        case ANSWER_CMD:
        case PING_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"currently can not process cmd %s in CmdSessionOfferProcessingState, call id %s, cmd was discard", 
                                                            CMD_STR[cmd], m_cs->m_session_key->m_cookie);
            goto err_ret;
        }
        case DELETE_CMD:
        {
            rtparg = new DeletRtp(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != processRTPArgs(rtparg, m_cs->m_session_key->m_cookie_id))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__,"process cmd DELETE_CMD error because of can not send delete rtp cmd to rtp thread in cmd session %s ", 
                                                                m_cs->m_session_key->m_cookie);
                delete rtparg;
                rtparg = NULL;
                goto err_ret;
            }
            *nextState = new CmdSessionDeleteState(m_cs); // to make sure can process the re-transimitd cmd
            Args* delCmdArg = new DeleteCmdArg(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != add_task(4000, fireArgs2controlProcess_s, delCmdArg))
            {
                delete delCmdArg;
                delCmdArg = NULL;
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add delete cmd session error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"currently can not process cmd %s in state of CmdSessionOfferProcessingState, call id %s, cmd was discard", 
                                                            CMD_STR[cmd], m_cs->m_session_key->m_cookie);
            goto err_ret;
        }
    }
    return ret;

err_ret:
    nextState = NULL;
    ret = -1;
    if(rtparg)
    {
        delete rtparg;
    }
    return ret;
}

#if 0
int CmdSessionOfferProcessingState::checkState(StateCheckArgs* stateArg)
{
    if(stateArg->state >= m_state)
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"cmd session %s check state failed in cmd session state of %s", m_cs->m_session_key->m_cookie, StateName[m_state]);
        return -1;
    }
    return 0;
}
#endif

// in the msg replied to SIP proxy, it doesn't need to add direction info, So don't use direction parameter
int CmdSessionOfferProcessingState::processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState)
{
    char resp[2048];
    int offset = 0;
    int ret = 0;
    int len = 0;
    resp[0] = '\0';
    if(sdp->m_parsed)
    {
        processSdpResp_s(sdp, m_cs->m_cookie.s, m_cs->m_cookie.len, resp, sizeof(resp), &offset);
    }
    else
    {
        snprintf(resp, sizeof(resp), "%s d3:sdp%d:%s6:result2:oke", m_cs->m_cookie.s, sdp->m_sdp_str.len, sdp->m_sdp_str.s);
    }
    tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"cmd session [%s] send sdp resp msg [%s] from direction of %s", m_cs->m_session_key->m_cookie, 
                                                  &resp[offset], g_RTPDirection_str[direction]);
    len = strlen(&resp[offset]);
    ret = m_cs->sendcmd(&resp[offset], len);
    if(0 == ret)
    {
        *nextState = new CmdSessionOfferProcessedState(m_cs);
        m_cs->cache_cookie_resp(m_cs->m_cookie.s, m_cs->m_cookie.len, &resp[offset], len);
    }
    else
    {
        *nextState = NULL;
        tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"response cmd off failed");
        return ret;
    }
    StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
    m_cs->m_state_check_count++;
    args->state = m_cs->m_state_check_count;
    if(0 != add_task(32000, fireArgs2controlProcess_s, args))
    {
        delete args;
        tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
    }
    return ret;
}

CmdSessionOfferProcessedState::CmdSessionOfferProcessedState(CmdSession* cs):CmdSessionState(cs)
{
    m_state = CMDSESSION_OFFER_PROCESSED_STATE;
}

CmdSessionOfferProcessedState::~CmdSessionOfferProcessedState()
{
}

int CmdSessionOfferProcessedState::processCMD(int cmd, CmdSessionState** nextState)
{
    int ret = 0;
    Sdp_session* sdp = NULL;
    SDPArgs* sdpArg = NULL;
    rtpSendRecvThreadArgs*  rtparg = NULL;
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
                std::string dir2("8:internal8:external");
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
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "err direction %s for cmd session: [%s] to process cmd %s", 
                                                                    direction->c_str(), m_cs->m_session_key->m_cookie, CMD_STR[OFFER_CMD]);
                    goto err_ret;
                }
                sdpArg = new SDPArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
                sdpArg->sdp = sdp;
                sdpArg->direction = dir;
                if(0 != processRTPArgs(sdpArg, m_cs->m_session_key->m_cookie_id))
                {
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"process cmd [%s] error because of sdp args error in cmd session [%s] ", 
                                                                    CMD_STR[OFFER_CMD], m_cs->m_session_key->m_cookie);
                    delete sdpArg;
                    sdpArg = NULL;
                    goto err_ret;
                }
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no sdp or no direction in cmd session [%s], to process cmd of [OFFER_CMD]", m_cs->m_session_key->m_cookie);
                goto err_ret;
            }
            *nextState = new CmdSessionOfferProcessingState(m_cs);
            m_cs->m_state_check_count++;
            StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            args->state = m_cs->m_state_check_count;
            if(0 != add_task(1600, fireArgs2controlProcess_s, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session [%s]", m_cs->m_session_key->m_cookie);
            }
            ret = 0;
            break;
        }
        case ANSWER_CMD:
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
                std::string dir2("8:internal8:external");
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
                if(0 != processRTPArgs(sdpArg, m_cs->m_session_key->m_cookie_id))
                {
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"process cmd %s error because of sdp args error in cmd session %s ", 
                                                                    CMD_STR[OFFER_CMD], m_cs->m_session_key->m_cookie);
                    delete sdpArg;
                    goto err_ret;
                }
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no sdp or no direction in cmd session %s, to process cmd of OFFER_CMD", m_cs->m_session_key->m_cookie);
                goto err_ret;
            }
            *nextState = new CmdSessionAnswerProcessingState(m_cs);
            StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            m_cs->m_state_check_count++;
            args->state = m_cs->m_state_check_count;
            if(0 != add_task(1600, fireArgs2controlProcess_s, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            ret = 0;
            break;
        }
        case DELETE_CMD:
        {
            rtparg = new DeletRtp(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != processRTPArgs(rtparg, m_cs->m_session_key->m_cookie_id))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__,"process cmd DELETE_CMD error because of can not send delete rtp cmd to rtp sendrecv thread in cmd session %s ", 
                                                                m_cs->m_session_key->m_cookie);
                delete rtparg;
                rtparg = NULL;
                goto err_ret;
            }
            *nextState = new CmdSessionDeleteState(m_cs); // to make sure can process the re-transimitd cmd
            Args* delCmdArg = new DeleteCmdArg(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != add_task(4000, fireArgs2controlProcess_s, delCmdArg))
            {
                delete delCmdArg;
                delCmdArg = NULL;
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add delete cmd session error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            break;
        }
        case PING_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s must not process pingpong cmd  in CmdSessionOfferProcessedState", 
                                                             m_cs->m_session_key->m_cookie, cmd);
            goto err_ret;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s process unknown cmd %d in CmdSessionOfferProcessedState", 
                                                             m_cs->m_session_key->m_cookie, cmd);
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
    if(rtparg)
    {
        delete rtparg;
    }
    ret = -1;
    *nextState = NULL;
    return ret;
}

#if 0
int CmdSessionOfferProcessedState::checkState(StateCheckArgs* stateArg)
{
    if(stateArg->state >= m_state)
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"cmd session %s check state failed in cmd session state of %s", m_cs->m_session_key->m_cookie, StateName[m_state]);
        return -1;
    }
    return 0;
}
#endif

CmdSessionAnswerProcessingState::CmdSessionAnswerProcessingState(CmdSession* cs):CmdSessionState(cs)
{
    m_state = CMDSESSION_ANSWER_PROCESSING_STATE;
}

CmdSessionAnswerProcessingState::~CmdSessionAnswerProcessingState()
{}

int CmdSessionAnswerProcessingState::processCMD(int cmd, CmdSessionState** nextState)
{
    int ret = 0;
    rtpSendRecvThreadArgs* rtparg = NULL;
    switch(cmd)
    {
        case OFFER_CMD:
        case ANSWER_CMD:
        case PING_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"currently can not process cmd %s in CmdSessionAnswerProcessingState, call id %s, cmd was discard", 
                                                            CMD_STR[cmd], m_cs->m_session_key->m_cookie);
            goto err_ret;
        }
        case DELETE_CMD:
        {
            rtparg = new DeletRtp(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != processRTPArgs(rtparg, m_cs->m_session_key->m_cookie_id))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__,"process cmd DELETE_CMD error because can not send delete-rtp cmd to rtp thread in cmd session %s ", 
                                                                m_cs->m_session_key->m_cookie);
                delete rtparg;
                rtparg = NULL;
                goto err_ret;
            }
            *nextState = new CmdSessionDeleteState(m_cs); // to make sure can process the re-transimitd cmd
            Args* delCmdArg = new DeleteCmdArg(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != add_task(4000, fireArgs2controlProcess_s, delCmdArg))
            {
                delete delCmdArg;
                delCmdArg = NULL;
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add delete cmd session error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            break;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"currently can not process cmd %s in state of CmdSessionAnswerProcessingState, call id %s, cmd was discard", 
                                                            CMD_STR[cmd], m_cs->m_session_key->m_cookie);
            goto err_ret;
        }
    }
    return ret;

err_ret:
    nextState = NULL;
    ret = -1;
    if(rtparg)
    {
        delete rtparg;
    }
    return ret;
}

// in the msg replied to SIP proxy, it doesn't need to add direction info, So don't use direction parameter
int CmdSessionAnswerProcessingState::processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState)
{
    char resp[2048];
    int offset = 0;
    int ret = 0;
    int len = 0;
    resp[0] = '\0';
    if(sdp->m_parsed)
    {
        processSdpResp_s(sdp, m_cs->m_cookie.s, m_cs->m_cookie.len, resp, sizeof(resp), &offset);
    }
    else
    {
        snprintf(resp, sizeof(resp), "%s d3:sdp%d:%s6:result2:oke", m_cs->m_cookie.s, sdp->m_sdp_str.len, sdp->m_sdp_str.s);
    }
    tracelog("RTP", DEBUG_LOG, __FILE__, __LINE__,"cmd session [%s] send sdp resp msg [%s] from direction of [%s]", 
                                                   m_cs->m_session_key->m_cookie, &resp[offset], g_RTPDirection_str[direction]);
    len = strlen(&resp[offset]);
    ret = m_cs->sendcmd(&resp[offset], len);
    if(0 == ret)
    {
        *nextState = new CmdSessionAnswerProcessedState(m_cs);
        m_cs->cache_cookie_resp(m_cs->m_cookie.s, m_cs->m_cookie.len, &resp[offset], len);
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__,"cmd session [%s] send sdp resp failed, msg [%s] from direction of [%s]", 
                                                         m_cs->m_session_key->m_cookie, &resp[offset], g_RTPDirection_str[direction]);
        *nextState = NULL;
        return ret;
    }
    //StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
    //args->state = CMDSESSION_ANSWER_PROCESSED_STATE;
    //if(0 != add_task(32000, fireArgs2controlProcess_s, args))
    //{
    //    delete args;
    //    tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
    //}
    return ret;
}

CmdSessionAnswerProcessedState::CmdSessionAnswerProcessedState(CmdSession* cs):CmdSessionState(cs)
{
    m_state = CMDSESSION_ANSWER_PROCESSED_STATE;
}

CmdSessionAnswerProcessedState::~CmdSessionAnswerProcessedState()
{}

int CmdSessionAnswerProcessedState::processCMD(int cmd, CmdSessionState** nextState)
{
    int ret = 0;
    rtpSendRecvThreadArgs*  rtparg = NULL;
    Sdp_session* sdp = NULL;
    switch (cmd)
    {
        case OFFER_CMD:
        case ANSWER_CMD:
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
                std::string dir2("8:internal8:external");
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
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "err direction %s for cmd session: [%s] to process cmd %s", 
                                                                    direction->c_str(), m_cs->m_session_key->m_cookie, CMD_STR[OFFER_CMD]);
                    goto err_ret;
                }
                SDPArgs* sdpArg = new SDPArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
                sdpArg->sdp = sdp;
                sdpArg->direction = dir;
                if(0 != processRTPArgs(sdpArg, m_cs->m_session_key->m_cookie_id))
                {
                    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"process cmd %s error because of sdp args error in cmd session [%s]", 
                                                                    CMD_STR[OFFER_CMD], m_cs->m_session_key->m_cookie);
                    delete sdpArg;
                    goto err_ret;
                }
            }
            else
            {
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "no sdp or no direction in cmd session %s, to process cmd of OFFER_CMD", m_cs->m_session_key->m_cookie);
                goto err_ret;
            }
            *nextState = new CmdSessionAnswerProcessingState(m_cs);
            StateCheckArgs* args = new StateCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            m_cs->m_state_check_count++;
            args->state = m_cs->m_state_check_count;
            if(0 != add_task(1600, fireArgs2controlProcess_s, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            ret = 0;
            break;
        }
        case DELETE_CMD:
        {
            rtparg = new DeletRtp(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != processRTPArgs(rtparg, m_cs->m_session_key->m_cookie_id))
            {
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__,"process cmd DELETE_CMD error because of can not send delete rtp cmd to rtp sendrecv thread in cmd session %s ", 
                                                              m_cs->m_session_key->m_cookie);
                delete rtparg;
                rtparg = NULL;
                goto err_ret;
            }
            *nextState = new CmdSessionDeleteState(m_cs); // to make sure can process the re-transimitd cmd
            Args* delCmdArg = new DeleteCmdArg(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            if(0 != add_task(4000, fireArgs2controlProcess_s, delCmdArg))
            {
                delete delCmdArg;
                delCmdArg = NULL;
                tracelog("RTP", ERROR_LOG,__FILE__, __LINE__, "add task of delete cmd session error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            break;
        }
        case PING_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s must not process pingpong cmd  in CmdSessionAnswerProcessedState", 
                                                             m_cs->m_session_key->m_cookie);
            goto err_ret;
        }
        default:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s process unknown cmd %d in CmdSessionOfferProcessedState", 
                                                             m_cs->m_session_key->m_cookie, cmd);
            goto err_ret;
        }
    }
    return ret;

err_ret:
    if(sdp)
    {
        delete sdp;
    }
    if(rtparg)
    {
        delete rtparg;
    }
    ret = -1;
    *nextState = NULL;
    return ret;
}

CmdSessionDeleteState::CmdSessionDeleteState(CmdSession* cs):CmdSessionState(cs)
{
    m_state = CMDSESSION_DELETE_STATE;
}

CmdSessionDeleteState::~CmdSessionDeleteState()
{}

int CmdSessionDeleteState::processCMD(int cmd, CmdSessionState** nextState)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s must not process cmd of %s in state CmdSessionDeleteState", m_cs->m_session_key->m_cookie, CMD_STR[cmd]);
    *nextState = NULL;
    return -1;
}



