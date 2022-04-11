#include "args.h"
#include "cmdSession.h"
#include "cmdSessionManager.h"
#include "log.h"
#include "rtpsession.h"
#include "rtpSendRecvProcs.h"

int Args::setArg(void* arg)
{
    tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "set arg failed");
    return -1;
}

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

StateCheckArgs::StateCheckArgs(char* key, int len)
{
    cs_key = new char[len+1];
    snprintf(cs_key, len+1, "%s", key);
}

StateCheckArgs::~StateCheckArgs()
{
    if(cs_key)
    {
        delete[] cs_key;
        cs_key = NULL;
    }
}

int StateCheckArgs::processCmd()
{
    int ret = 0;
    CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(cs_key);
    if(cs)
    {  
         ret = cs->checkState(this);
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

SendCMDArgs::SendCMDArgs(char* cs_key, int len)
{  
    cs_cookie = new char[len+1];
    snprintf(cs_cookie, len+1, "%s", cs_key);
}

SendCMDArgs:: ~SendCMDArgs()
{
   if(cs_cookie)
   {
       delete[] cs_cookie;
       cs_cookie = NULL;
   }   
}

int SendCMDArgs::processCmd()
{
    int ret = 0;
    CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(cs_cookie);
    if(cs)
    {  
         ret = cs->doAction2PrepareSend();
         //if(0 != ret)
         //{
         //    CmdSessionManager::getInstance()->popCmdSession(cs->m_session_key);
         //    delete cs;
         //    return -1;
         //}
         //return 0;
    }    
    else 
    {        
        return -1;
    }
    return ret;        
}

SDPArgs::SDPArgs(const char* key, int len)
{
    call_id.len = len;
    call_id.s = new char[len+1];
    snprintf(call_id.s, len+1, "%s", key);
    direction = MAX_DIRECTION;
    sdp = NULL;
    process = NULL;
}

SDPArgs::~SDPArgs()
{
    if(call_id.len)
    {
        delete[] call_id.s;
        call_id.s = NULL;
        call_id.len = 0;
    }
    process = NULL;
    sdp = NULL;
}

int SDPArgs::setArg(void* arg)
{
    process = (RtpProcess*)arg;
    return 0;
}

int SDPArgs::processCmd()
{
    SessionKey* sk = new SessionKey(call_id.s);
    RtpSession* rtpsession = process->getRtpSession(sk);
    if(!rtpsession)
    {
        rtpsession = new RtpSession(sk, process);
    }
    else
    {
        delete sk;
    }
    rtpsession->processSdp(sdp, direction);
    return 0;
}

SDPRespArgs::SDPRespArgs(const char* key, int len)
{
    call_id.len = len;
    call_id.s = new char[len+1];
    snprintf(call_id.s, len+1, "%s", key);
    direction = MAX_DIRECTION;
    sdp = NULL;
}

SDPRespArgs::~SDPRespArgs()
{
    if(call_id.len)
    {
        delete[] call_id.s;
        call_id.s = NULL;
        call_id.len = 0;
    }
    if(sdp)
    {
        delete sdp;
        sdp = NULL;
    }
}

int SDPRespArgs::processCmd()
{
    int ret = 0;
    SessionKey* sk = new SessionKey(call_id.s);
    CmdSession* cs = CmdSessionManager::getInstance()->getCmdSession(sk);
    if(!cs)
    {
        tracelog("RTP", WARNING_LOG,__FILE__, __LINE__, "don't find cmd session whose call id is %s can process SDPRespArgs", call_id.s);
        ret = -1;
        goto ret;
    }
    else
    {
        cs->processSdpResp(sdp, direction);
    }
ret:
    delete sk;
    return ret;
}

PipeEventArgs::~PipeEventArgs()
{
    if(args_data)  
    {
        delete args_data;
        args_data = NULL;
    }
}
