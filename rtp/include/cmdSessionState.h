#ifndef __CMD_SESSION_STATE_H__
#define __CMD_SESSION_STATE_H__
#include "sdp.h"

class CmdSession;
class PingCheckArgs;
class StateCheckArgs;


/*******************************************************************************************************************************************************************************
 
                                                                         +------------------------+
                                                                         |         init           |
                                                                         +------+-----------------+
                                                                                |
                                                                                |
                                                                         +------v-----------------+
                                                                     +---|     offer processing   |
                                                                     |   +---------+-------^------+
                                                                     |             |       |
                                                                     |             |       |
                                                                     |             |       |
                                                                     |   +---------v-------+------+
                                                                     +---|    offer processed     |
                                                                     |   +---------+--------------+
                                                                     |             |
                                                                     |             |
                                                                     |             |
                                                                     |   +---------v--------------+
                                                                     +---|    answe processing    |
                                                                     |   +----------+-----^-------+
                                                                     |              |     |
                                                                     |              |     |
                                                                     |              |     |
                                                                     |   +----------v-----+-------+
                                                                     +---|    answer processed    |
                                                                     |   +---------+--------------+
                                                                     |             |
                                                                     |             |
                                                                     |             |
                                                                     |   +---------v--------------+
                                                                     +---|  session offer updating|
                                                                     |   +--------+----^----------+
                                                                     |            |    |
                                                                     |            |    |
                                                                     |            |    |
                                                                     |   +--------v----+----------+
                                                                     +---|  session offer updated |
                                                                     |   +--------+---------------+
                                                                     |            |
                                                                     |            |
                                                                     |            |
                                                                     |   +--------v---------------+
                                                                     +---|session answer updating |
                                                                     |   +------+-------^---------+
                                                                     |          |       |
                                                                     |          |       |
                                                                     |          |       |
                                                                     |   +------v-------+---------+
                                                                     |   |session answer updated  |
                                                                     |   +----------+-------------+
                                                                     |              |
                                                                     |              |                           
                                                                     |              |
                                                                     |   +----------v-------------+
                                                                     +-->|         delete         |
                                                                         +------------------------+
 
 
 ******************************************************************************************************************************************************** */

class CmdSessionState
{
public:
    CmdSessionState(CmdSession* cs);
    virtual ~CmdSessionState();
    virtual int processCMD(int cmd, CmdSessionState** nextState) = 0;
    virtual int checkPingKeepAlive(PingCheckArgs* pingArg);
    virtual int checkState(StateCheckArgs* stateArg);
    virtual int processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState);

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
    virtual int checkState(StateCheckArgs* stateArg);
public:
    unsigned long m_count; // ping counter from connected established
};

class CmdSessionOfferProcessingState : public CmdSessionState
{
public:
    CmdSessionOfferProcessingState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessingState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
    //virtual int checkState(StateCheckArgs* stateArg);
    virtual int processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState);
};

class CmdSessionOfferProcessedState : public CmdSessionState
{
public:
    CmdSessionOfferProcessedState(CmdSession* cs);
    virtual ~CmdSessionOfferProcessedState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
    //virtual int checkState(StateCheckArgs* stateArg);
};

class CmdSessionAnswerProcessingState : public CmdSessionState
{
public:
    CmdSessionAnswerProcessingState(CmdSession* cs);
    virtual ~CmdSessionAnswerProcessingState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
    virtual int processSdpResp(Sdp_session* sdp, RTPDirection direction, CmdSessionState** nextState);
};

class CmdSessionAnswerProcessedState : public CmdSessionState
{
public:
    CmdSessionAnswerProcessedState(CmdSession* cs);
    virtual ~CmdSessionAnswerProcessedState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
};

class CmdSessionAnswerPorcessedState : public CmdSessionState
{
public:
    CmdSessionAnswerPorcessedState(CmdSession* cs);
    virtual ~CmdSessionAnswerPorcessedState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
};

class CmdSessionDeleteState : public CmdSessionState
{
public:
    CmdSessionDeleteState(CmdSession* cs);
    virtual ~CmdSessionDeleteState();
    virtual int processCMD(int cmd, CmdSessionState** nextState);
};

#endif
