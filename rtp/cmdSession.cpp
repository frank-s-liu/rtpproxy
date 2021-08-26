#include "cmdSession.h"
#include "cmdSessionState.h"

CmdSession::CmdSession()
{
    css = new CmdSessionInitState(this);
}

CmdSession::~CmdSession()
{
    delete css;
    css = NULL;
}

CmdSession::process_cmd()
{
    css->processCMD();
}
