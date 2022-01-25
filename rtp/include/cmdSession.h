#ifndef __CMD_CONTROL_SESSION_H__
#define __CMD_CONTROL_SESSION_H__

class CmdSessionState;

typedef enum cmd_type
{
    OFFER_CMD=0,
    ANSWER_CMD,
    DELETE_CMD
    MAX_CONTROL_CMD
}CONTROL_CMD;

class CmdSession
{
public:
    CmdSession();
    virtual ~CmdSession();
    void set_client_addr(char* ip, int port);
    int process_cmd(int cmd);

public:
    long  m_call_id;

private:
    CmdSessionState* css;
    int   m_direction;
    int   m_cmd;
    int   m_client_port;
    long  m_cookie;
    char  m_client_ip[32];
};

#endif
