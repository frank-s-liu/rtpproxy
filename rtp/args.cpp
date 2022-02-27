#include "args.h"
#include "cmdSession.h"
#include "cmdSessionManager.h"


PingCheckArgs::PingCheckArgs(char* cs_key, int len)
{  
    cs_cookie = new char[len+1];
    snprintf(cs_cookie, len+1, "%s", cs_key);
}

PingCheckArgs:: ~PingCheckArgs()
{
   if(cs_cookie)
   {
       delete[] cs_cookie;
       cs_cookie = NULL;
   }   
}

int PingCheckArgs::processCmd()
{
    int ret = 0;
    CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(cs_cookie);
    if(cs)
    {  
         ret = cs->checkPingKeepAlive(this);
         if(0 != ret)
         {
             CmdSessionManager::getInstance()->popCmdSession(cs->m_session_key);
             delete cs;
             return -1;
         }
         return 0;
    }    
    else 
    {        
        return -1;
    }        
}

PipeEventArgs::~PipeEventArgs()
{
    if(args_data)  
    {
        delete args_data;
        args_data = NULL;
    }
}
