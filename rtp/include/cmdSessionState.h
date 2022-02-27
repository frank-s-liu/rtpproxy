#ifndef __CMD_SESSION_STATE_H__
#define __CMD_SESSION_STATE_H__

class CmdSession;
class PingCheckArgs;


class CmdSessionState
{
public:
    CmdSessionState(CmdSession* cs);
    virtual ~CmdSessionState();
    virtual int processCMD(int cmd) = 0;
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg) = 0;

protected:
    CmdSession* m_cs;
};

class CmdSessionInitState : public CmdSessionState
{
public:
    CmdSessionInitState(CmdSession* cs);
    virtual ~CmdSessionInitState();
    virtual int processCMD(int cmd);
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
public:
    unsigned long m_count;

};

class CmdSessionOfferProcessingState : public CmdSessionState
{
public:
    CmdSessionOfferProcessingState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessingState();
    virtual int processCMD(int cmd);
};

class CmdSessionOfferProcessedState : public CmdSessionState
{
public:
    CmdSessionOfferProcessedState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessedState();
    virtual int processCMD(int cmd);
};

#endif
