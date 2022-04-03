#ifndef __CMD_SESSION_STATE_H__
#define __CMD_SESSION_STATE_H__

class CmdSession;
class PingCheckArgs;


class CmdSessionState
{
public:
    CmdSessionState(CmdSession* cs);
    virtual ~CmdSessionState();
    virtual int processCMD(int cmd, CmdSessionState** nextState) = 0;
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);

protected:
    CmdSession*     m_cs;
    unsigned char   m_state;
};

class CmdSessionInitState : public CmdSessionState
{
public:
    CmdSessionInitState(CmdSession* cs);
    virtual ~CmdSessionInitState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
public:
    unsigned long m_count; // ping counter from connected established

};

class CmdSessionOfferProcessingState : public CmdSessionState
{
public:
    CmdSessionOfferProcessingState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessingState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
};

class CmdSessionOfferProcessedState : public CmdSessionState
{
public:
    CmdSessionOfferProcessedState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessedState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
};

#endif
