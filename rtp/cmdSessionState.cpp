#include "cmdSessionState.h"
#include "cmdSession.h"
#include "rtpControlProcess.h"
#include "log.h"
#include "args.h"
#include "task.h"
#include "sdp.h"


#include <stdlib.h>

// timer thread call back
static void processPingCheck(void* args)
{
    PingCheckArgs* pingArg = (PingCheckArgs*)args;
    ControlProcess::getInstance()->add_pipe_event(pingArg);
}


CmdSessionState::CmdSessionState(CmdSession* cs)
{
    m_cs = cs;
}

CmdSessionState::~CmdSessionState()
{
    m_cs = NULL;
}

CmdSessionInitState::CmdSessionInitState(CmdSession* cs):CmdSessionState(cs)
{
    m_count = 0;
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

int CmdSessionInitState::processCMD(int cmd)
{
    int ret = 0;
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
               Sdp_session* sdp = new Sdp_session();
               sdp->parse(v->c_str(), v->length());
               std::string dir1("8:external8:internal");
               std::string dir2("8:internal:8:external");
               if(*direction == dir1)
               {
                   m_cs->setSdp(EXTERNAL_PEER, sdp);
               }
               else if(*direction == dir2)
               {
                   m_cs->setSdp(INTERNAL_PEER, sdp);
               }
               else
               {
                   delete sdp;
                   tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"not support direction of %s in cmd session %s", direction->c_str(), m_cs->m_session_key->m_cookie); 
               }
           }
           else
           {
               tracelog("RTP", WARNING_LOG,__FILE__, __LINE__,"bencode no sdp or no direction in cmd session %s", m_cs->m_session_key->m_cookie);
               ret = -1;
               break;
           }
           ret = 0;
           break;
        }
        case ANSWER_CMD:
        case DELETE_CMD:
        {
            tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "cmd session %s cmd %d must not be processed in CmdSessionInitState", m_cs->m_session_key->m_cookie, cmd);
            ret = -1;
            break;
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
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            m_cs->sendPongResp();
            break;
        }
        default:
        {
            ret = 1;
            break;
        }
    }
    return ret;
}
