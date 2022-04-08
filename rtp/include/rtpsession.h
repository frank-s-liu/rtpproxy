#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"



class RtpSession
{
public:
    RtpSession();
    virtual ~RtpSession();

public:
    SessionKey*               m_session_key;
};

#endif
