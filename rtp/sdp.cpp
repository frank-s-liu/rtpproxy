#include "sdp.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    if(buf && encoding_str.s)
    {
        int len = snprintf(buf, buflen, "a=rtpmap:%d %s\r\n", payload_type, encoding_str.s);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtpmap attribute serialize failed %d %s in buf, buf len %d.", payload_type, encoding_str.s, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtpmap attribute serialize failed %d %d in buf, buf len %d.", payload_type, encoding_str.len, buflen);
        return -1;
    }
}

int Attr_rtpmap::parse(char* line)
{
    char* pos = strstr(line, "a=rtpmap:");
    char* end = strstr(line, "\r\n");
    char* ec_str = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtpmap attribute parse failed, %s", line);
        return -1;
    }
    pos += strlen("a=rtpmap:");
    payload_type = strtol(pos, &ec_str, 10);
    if(!ec_str || *ec_str != ' ' || ec_str>end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtpmap attribute parse failed, %s", line);
        return -1;
    }
    ec_str++;
    encoding_str.len = end-ec_str;
    encoding_str.s = new char[encoding_str.len+1];
    snprintf(encoding_str.s, encoding_str.len+1, "%s", ec_str);
    return 0;
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
        int len = snprintf(buf, buflen, "a=fmtp:%d %s\r\n", payload_type, format_parms_str.s);
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
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "fmtp attribute serialize failed %d %d in buf, buf len %d.", payload_type, format_parms_str.len, buflen);
        return -1;
    }
}

int Attr_fmtp::parse(char* line)
{
    char* pos = strstr(line, "a=fmtp:");
    char* end = strstr(line, "\r\n");
    char* fp_str = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "fmtp attribute parse failed, %s", line);
        return -1;
    }
    pos += strlen("a=fmtp:");
    payload_type = strtol(pos, &fp_str, 10);
    if(!fp_str || *fp_str != ' ' || fp_str>end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "fmtp attribute parse failed, %s", line);
        return -1;
    }
    fp_str++;
    format_parms_str.len = end-fp_str;
    format_parms_str.s = new char[format_parms_str.len+1];
    snprintf(format_parms_str.s, format_parms_str.len+1, "%s", fp_str);
    return 0;
}

Attr_crypto::Attr_crypto()
{
    tag = -1;
    suite_str.s = NULL;
    suite_str.len = 0;
    key_params.s = NULL;
    key_params.len = 0;
}

Attr_crypto::~Attr_crypto()
{
    if(suite_str.len)
    {
        delete[] suite_str.s;
        suite_str.s = NULL;
        suite_str.len = 0;
    }
    if(key_params.len)
    {
        delete[] key_params.s;
        key_params.s = NULL;
        key_params.len = 0;
    }
}

int Attr_crypto::serialize(char* buf, int buflen)
{
    if(buf && suite_str.s && key_params.s)
    {
        int len = snprintf(buf, buflen, "a=crypto:%d %s %s\r\n", tag, suite_str.s, key_params.s);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto attribute serialize failed %d %s %s in buf, buf len %d.", tag, suite_str.s, key_params.s, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto attribute serialize failed %d %d %d in buf, buf len %d.", tag, suite_str.len, key_params.len, buflen);
        return -1;
    }
}

int Attr_crypto::parse(char* line)
{
    char* pos = strstr(line, "a=crypto:");
    char* end = strstr(line, "\r\n");
    char* suit_str = NULL;
    char* kp_start = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto attribute parse failed, %s", line);
        return -1;
    }
    pos += strlen("a=crypto:");
    tag = strtol(pos, &suit_str, 10);
    if(!suit_str || *suit_str != ' ' || suit_str > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto attribute parse tag failed, %s", line);
        return -1;
    }
    while(suit_str && *suit_str == ' ')
    {
        suit_str++;
    }
    kp_start = strchr(suit_str, ' ');
    if(!kp_start || kp_start > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "crypto attribute parse crypto-suite failed, %s", line);
        return -1;
    }
    suite_str.len = kp_start - suit_str;
    suite_str.s = new char[suite_str.len+1];
    snprintf(suite_str.s, suite_str.len+1, "%s", suit_str);
    
    while(kp_start && *kp_start == ' ')
    {
        kp_start++;
    }
    key_params.len = end - kp_start;
    key_params.s = new char[key_params.len+1];
    snprintf(key_params.s, key_params.len+1, "%s", kp_start);
    
    return 0;
}

Attr_sendrecv::Attr_sendrecv()
{

}

Attr_sendrecv::~Attr_sendrecv()
{

}

int Attr_sendrecv::serialize(char* buf, int buflen)
{
    if(buf)
    {
        int len = 0;
        switch (attr_type)
        {
            case ATTR_INACTIVE:
            {
                len = snprintf(buf, buflen, "a=inactive\r\n");
                break;
            }
            case ATTR_SENDRECV:
            {
                len = snprintf(buf, buflen, "a=sendrecv\r\n");
                break;
            }
            case ATTR_SENDONLY:
            {
                len = snprintf(buf, buflen, "a=sendonly\r\n");
                break;
            }
            case ATTR_RECVONLY:
            {
                len = snprintf(buf, buflen, "a=recvonly\r\n");
                break;
            }
            default:
            {
                tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sendrecv attribute serialize failed because of error attr_type %d.", attr_type);
                return -1;
            }
        }
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sendrecv attribute serialize failed , buf len %d.", buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sendrecv attribute serialize failed becuase of buf is null");
        return -1;
    }
}

int Attr_sendrecv::parse(char* line)
{
    char* end = strstr(line, "\r\n");
    char* pos = strstr(line, "a=sendrecv:");
    if(pos && end && pos < end)
    {
        attr_type = ATTR_SENDRECV;
    }
    else if((pos=strstr(line, "a=sendonly:")) && end && pos < end)
    {
        attr_type = ATTR_SENDONLY;
    }
    else if((pos=strstr(line, "a=recvonly:")) && end && pos < end)
    {
        attr_type = ATTR_RECVONLY;
    }
    else if((pos=strstr(line, "a=inactive:")) && end && pos < end)
    {
        attr_type = ATTR_INACTIVE;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sendrecv attribute parse failed %s", line);
        return -1;
    }
    return 0;
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
    Attr_map::iterator ite;
    for (ite = attrs.begin(); ite != attrs.end(); )
    {
        delete ite->second;
        attrs.erase(ite++);
    }

}

