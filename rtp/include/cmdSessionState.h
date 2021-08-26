#ifndef __CMD_SESSION_STATE_H__
#define __CMD_SESSION_STATE_H__

class CmdSession;

class CmdSessionState
{
public:
    CmdSessionState(CmdSession* cs);
    virtual ~CmdSessionState();
    virtual int processCMD() = 0;

protected:
    CmdSession* cs;
};

class CmdSessionInitState : public CmdSessionState
{
public:
    CmdSessionInitState(CmdSession* cs);
    virtual ~CmdSessionInitState();
    virtual int processCMD();
};

#endif
