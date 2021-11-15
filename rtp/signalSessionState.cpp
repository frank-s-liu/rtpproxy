#include "cmdSessionState.h"

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

}

CmdSessionInitState::~CmdSessionInitState()
{

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
        default:
        {
            ret = 1;
            break;
        }
    }
    return ret;
}
