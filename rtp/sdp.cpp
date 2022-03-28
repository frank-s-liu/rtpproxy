#include "sdp.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

Network_address::Network_address()
{
    net_type = IN;
    addr_type = IP4;
    address.s = NULL;
    address.len = 0;
}

Network_address::~Network_address()
{
    if(address.len)
    {
        delete[] address.s;
        address.len = 0;
        address.s = NULL;
    }
}

Sdp_origin::Sdp_origin()
{
    username.s = NULL;
    username.len = 0;
    session_id.s = NULL;
    session_id.len = 0;
    version_num = 0;
}

Sdp_origin::~Sdp_origin()
{
    if(username.len)
    {
        delete[] username.s;
        username.s = NULL;
        username.len = 0;
    }
    if(session_id.len)
    {
        delete[] session_id.s;
        session_id.s = NULL;
        session_id.len = 0;
    }
}

Sdp_connection::Sdp_connection()
{

}

Sdp_connection::~Sdp_connection()
{

}

Attr_rtpmap::Attr_rtpmap()
{
    payload_type = MAX_PAYLOAD_TYPE;
    encoding_str.s = NULL;
    encoding_str.len = 0;
    clock_rate_str.s = NULL;
    clock_rate_str.len = 0;
    attr_type = ATTR_RTPMAP;
}

Attr_rtpmap::~Attr_rtpmap()
{
    if(encoding_str.len)
    {
        delete[] encoding_str.s;
        encoding_str.s = NULL;
        encoding_str.len = 0;
    }
    if(clock_rate_str.len)
    {
        delete[] clock_rate_str.s;
        clock_rate_str.s = NULL;
        clock_rate_str.len = 0;
    }
}

int Attr_rtpmap::serialize(char* buf, int buflen)
{
    if(buf && encoding_str.s && clock_rate_str.s)
    {
        int len = snprintf(buf, buflen, "a=rtpmap:%d %s/%s", payload_type, encoding_str.s, clock_rate_str.s);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtpmap attribute serialize failed %d %s/%s in buf, buf len %d.", payload_type, encoding_str.s, clock_rate_str.s, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

Attr_fmtp::Attr_fmtp()
{
    payload_type = MAX_PAYLOAD_TYPE;
    attr_type = ATTR_FMTP;
    format_parms_str.s = NULL;
    format_parms_str.len = 0;
}

Attr_fmtp::~Attr_fmtp()
{
    if(format_parms_str.len)
    {
        delete[] format_parms_str.s;
        format_parms_str.s = NULL;
        format_parms_str.len = 0;
    }
}

int Attr_fmtp::serialize(char* buf, int buflen)
{
    if(buf && format_parms_str.s)
    {
        int len = snprintf(buf, buflen, "a=fmtp:%d %s", payload_type, format_parms_str.s);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "fmtp attribute serialize failed %d %s in buf, buf len %d.", payload_type, format_parms_str.s, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

Sdp_attribute::Sdp_attribute()
{
    attr_type = ATTR_OTHER;
}

Sdp_attribute::~Sdp_attribute()
{
}

Sdp_media::Sdp_media()
{
    media_type = MAX_MEDIA_TYPE;
    transport = MAX_TRANSPORT_PROTOCOL;
    port = 0;
    port_count = 1;
    fmts = NULL;
    fmts_num = 0;
}

Sdp_media::~Sdp_media()
{
    if(fmts_num)
    {
        delete[] fmts;
        fmts = NULL;
    }
    rtpmap_attr_map::iterator ite;
    for (ite = rtpmap_attrs.begin(); ite != rtpmap_attrs.end(); )
    {
        delete ite->second;
        rtpmap_attrs.erase(ite++);
    }

    sdp_attr_map::iterator ite_sdp;
    for (ite_sdp = sdp_attrs.begin(); ite_sdp != sdp_attrs.end(); )
    {
        delete ite_sdp->second;
        sdp_attrs.erase(ite_sdp++);
    }
}

