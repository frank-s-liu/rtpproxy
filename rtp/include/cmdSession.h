#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

class CmdSessionState;
class CmdSession
{
public:
    CmdSession();
    virtual ~CmdSession();
    int process_cmd();
private:
    CmdSessionState* css;
        
};

#endif
