#ifndef __RTP_SESSION_H__
#define __RTP_SESSION_H__

#include "sessionKey.h"


class Sdp_session;
class RtpSession
{
public:
    RtpSession();
    RtpSession(SessionKey* key);
    virtual ~RtpSession();
    int processSdp(Sdp_session* sdp, int direction);

public:
    SessionKey*               m_session_key;
};

#endif
