#ifndef __CMD_SESSION_MANAGER_H__
#define __CMD_SESSION_MANAGER_H__

#include "udpConnection.h"

class CmdSessionManager
{
public:
    CmdSessionManager(const char* localip, int localPort);
    virtual ~CmdSessionManager();
    int processCmd();
private:
    int sendCmdResp(); 
   
private:
    Socket* m_connection;
};

#endif
