#include "cmdSessionState.h"
#include "cmdSession.h"
#include "rtpControlProcess.h"
#include "log.h"
#include "args.h"
#include "task.h"


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
           ret = 0;
           break;
        }
        case ANSWER_CMD:
        case DELETE_CMD:
        case PING_CMD:
        {
            m_count++;
            PingCheckArgs* args = new PingCheckArgs(m_cs->m_session_key->m_cookie, m_cs->m_session_key->m_cookie_len);
            args->ping_recv_count = m_count;
            //args->cmdtype = PING_CHECK_CMD;
            if(0 != add_task(120000, processPingCheck, args))
            {
                delete args;
                tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "add state check task error for cmd session %s", m_cs->m_session_key->m_cookie);
            }
            //m_cs->respPong();
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
