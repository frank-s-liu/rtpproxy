#include "sdp.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// example: 
// IN IP4 xxx.xxx.xxx.xxx
// IN IP6 2001:2345:6789:ABCD:EF01:2345:6789:ABCD
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

int Network_address::parse(char* network)
{
    char* end = strstr(network, "\r\n");
    if(!end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "network address parsing failed %s ", network);
        return -1;
    }
    while(network && *network == ' ')
    {
        network++;
    }
    if(network[0] != 'I' || network[1] != 'N')
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "network address parsing net type failed %s ", network);
        return -1;
    }
    network += 2;
    
    while(network && *network == ' ')
    {
        network++;
    }
    if(network[0] != 'I' || network[1] != 'P')
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "network address parsing net type failed %s ", network);
        return -1;
    }
    if(network[2] == '4')
    {
        addr_type = IP4;
    }
    else if(network[2] == '6')
    {
        addr_type = IP6;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "network address parsing address type failed %s ", network);
        return -1;
    }
    network += 3;
    while(network && *network == ' ')
    {
        network++;
    }
    address.len = end - network;
    address.s = new char[address.len+1];
    snprintf(address.s, address.len+1, "%s", network);
    return 0;
}

int Network_address::serialize(char* buf, int buflen)
{
    if(buf && address.len)
    {
        int len = snprintf(buf, buflen, "IN %s %s\r\n", AddrTypeStr[addr_type], address.s);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "Network_address serialize failed buf %s, buffer len%d.", buf, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "Network_address serialize failed, address len %d, buf len %d.", address.len, buflen);
        return -1;
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


// o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5
int Sdp_origin::serialize(char* buf, int buflen)
{
    if(buf && parsed)
    {
        char netaddr[64]={0};
        int len = 0;
        int ret = address.serialize(netaddr, sizeof(netaddr));
        if(0 != ret)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp origin serialize failed because of network address serialize failed, %s", netaddr);
            return -1;
        }
        len = snprintf(buf, buflen, "o=%s %s %lu %s\r\n", username.s, session_id.s, version_num, netaddr);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp origin serialize failed because of buf len,  buf %s, buffer len%d.", buf, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "Sdp_origin serialize failed, parsed=%d, buf len=%d.", parsed, buflen);
        return -1;
    }
}

int Sdp_origin::parse(char* origin)
{
    char* pos = strstr(origin, "o=");
    char* end = strstr(origin, "\r\n");
    char* sid_str = NULL;
    char* ver_str = NULL;
    char* addr_str = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "origin parse failed, %s", origin);
        return -1;
    }
    pos += strlen("o=");
    while(pos && *pos==' ')
    {
        pos++;
    }
    sid_str = strchr(pos, ' ');
    if(!sid_str)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "origin parse failed, no session id info %s", origin);
        return -1;
    }

    username.len = sid_str-pos;
    username.s = new char[username.len+1];
    snprintf(username.s, username.len+1, "%s", pos);

    while(sid_str && *sid_str == ' ')
    {
        sid_str++;
    }
    ver_str = strchr(sid_str, ' ');
    if(!ver_str)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "origin parse failed, no sdp version info %s", origin);
        return -1;
    }
    session_id.len = ver_str-sid_str;
    session_id.s = new char[session_id.len+1];
    snprintf(session_id.s, session_id.len+1, "%s", sid_str);

    while(ver_str && *ver_str==' ')
    {
        ver_str++;
    }
    version_num = strtol(ver_str, &addr_str, 10);
    if(!addr_str || *addr_str != ' ' || addr_str>end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp origin parse failed because of address info, %s", origin);
        return -1;
    }
    if(0 != address.parse(addr_str))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp origin line parsing failed because of parsing network address failed, %s", origin);
        return -1;
    }
    parsed = 1;
    return 0;
}

Sdp_connection::Sdp_connection()
{

}

Sdp_connection::~Sdp_connection()
{

}

// c=IN IP4 xxx.xxx.xxx.xxx
int Sdp_connection::parse(char* network)
{
    if(0 != address.parse(network))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp connection line parsing failed because of parsing network address failed, %s", network);
        return -1;
    }
    parsed = 1;
    return 0;
}

int Sdp_connection::serialize(char* buf, int buflen)
{
    if(buf && parsed)
    {
        char netaddr[64]={0};
        int len = 0;
        int ret = address.serialize(netaddr, sizeof(netaddr));
        if(0 != ret)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp connection serialize failed because of network address serialize failed, %s", netaddr);
            return -1;
        }
        len = snprintf(buf, buflen, "c=%s\r\n", netaddr);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp connection serialize failed because of buf len,  buf %s, buffer len%d.", buf, buflen);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "Sdp connection serialize failed, parsed=%d, buf len=%d.", parsed, buflen);
        return -1;
    }
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
    if(buf && parsed)
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
    while(ec_str && *ec_str == ' ')
    {
        ec_str++;
    }
    encoding_str.len = end-ec_str;
    encoding_str.s = new char[encoding_str.len+1];
    snprintf(encoding_str.s, encoding_str.len+1, "%s", ec_str);
    parsed = 1;
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
    if(buf && parsed)
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
    while(fp_str && *fp_str == ' ')
    {
        fp_str++;
    }
    format_parms_str.len = end-fp_str;
    format_parms_str.s = new char[format_parms_str.len+1];
    snprintf(format_parms_str.s, format_parms_str.len+1, "%s", fp_str);
    parsed = 1;
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
    if(buf && parsed)
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
    parsed = 1;
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
    if(buf && parsed)
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
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sendrecv attribute serialize failed, parsed %d ", parsed);
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
    parsed = 1;
    return 0;
}

Attr_rtcp::Attr_rtcp()
{
    port = 0;
}

Attr_rtcp::~Attr_rtcp()
{

}

int Attr_rtcp::serialize(char* buf, int buflen)
{
    if(buf && parsed)
    {
        char netaddr[64]={0};
        int len = 0;
        int ret = address.serialize(netaddr, sizeof(netaddr));
        if(0 != ret)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute serialize failed because of network address serialize failed, %s", netaddr);
            return -1;
        }
        len = snprintf(buf, buflen, "a=rtcp:%d %s\r\n", port, netaddr);
        if (len >= buflen)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute serialize failed because of buf len, buf le =[%d], buf=[%s]", buflen, buf);
            return -1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute serialize failed, parsed=%d, buf len %d.", parsed, buflen);
        return -1;
    }
}

int Attr_rtcp::parse(char* line)
{
    char* pos = strstr(line, "a=rtcp:");
    char* end = strstr(line, "\r\n");
    char* addr_str = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute parse failed, %s", line);
        return -1;
    }
    pos += strlen("a=rtcp:");
    port = strtol(pos, &addr_str, 10);
    if(!addr_str || *addr_str != ' ' || addr_str>end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute parse failed, %s", line);
        return -1;
    }
    if(0 != address.parse(addr_str))
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "rtcp attribute parse failed because of parsing network address failed, %s", line);
        return -1;
    }
    parsed = 1;
    return 0;
}


Sdp_attribute::Sdp_attribute()
{
    attr_type = ATTR_OTHER;
    parsed = 0;
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
    fmts.s = NULL;
    fmts.len = 0;
}

Sdp_media::~Sdp_media()
{
    if(fmts.len)
    {
        delete[] fmts.s;
        fmts.s = NULL;
        fmts.len = 0;
    }
    Attr_map::iterator ite;
    for (ite = attrs.begin(); ite != attrs.end(); )
    {
        delete ite->second;
        attrs.erase(ite++);
    }
}

// m=audio 12345 RTP/SAVP 101 102
int Sdp_media::parse(char* media)
{
    char* pos = strstr(media, "m=");
    char* end = strstr(media, "\r\n");
    char* protocaol_str = NULL;
    if(!end || !pos || pos > end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing failed, %s", media);
        return -1;
    }
    pos += strlen("m=");
    while(pos && *pos==' ')
    {
        pos++;
    }
    if(pos[0] == 'a' && pos[1]=='u' && pos[2] == 'd' && pos[3]=='i' && pos[4]=='o')
    {
        media_type = AUDIO;
        pos += 5;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parse failed, only support audio. media line %s", media);
        return -1;
    }
    while(pos && *pos==' ')
    {
        pos++;
    }
    port = strtol(pos, &protocaol_str, 10);
    if(!protocaol_str || protocaol_str>end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing port failed, %s", media);
        return -1;
    }
    if(*protocaol_str == '/')
    {
        pos = protocaol_str + 1;
        protocaol_str = NULL;
        port_count = strtol(pos, &protocaol_str, 10);
        if(!protocaol_str || *protocaol_str != ' ' || protocaol_str>end)
        {
            tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing port number failed, %s", media);
            return -1;
        }
    }
    while(protocaol_str && *protocaol_str == ' ')
    {
        protocaol_str++;
    }
    if(protocaol_str[0]=='R' && protocaol_str[1]=='T' && protocaol_str[2]=='P' && protocaol_str[3]=='/')
    {
        protocaol_str += 4;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing rtp protocaol failed, media line is[%s]", media);
        return -1;
    }
    if(protocaol_str[0] == 'A' && protocaol_str[1]=='V' && protocaol_str[2]=='P')
    {
        transport = RTP_AVP;
        protocaol_str += 3;
    }
    else if(protocaol_str[0] == 'S' && protocaol_str[1]=='A' && protocaol_str[2]=='V' && protocaol_str[3]=='P' && protocaol_str[4]==' ')
    {
        transport = RTP_SAVP;
        protocaol_str += 4;
    }
    else if(protocaol_str[0] == 'S' && protocaol_str[1]=='A' && protocaol_str[2]=='V' && protocaol_str[3]=='P' && protocaol_str[4]=='F')
    {
        transport = RTP_SAVPF;
        protocaol_str += 5;
    }
    else
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing rtp protocol failed, unknown type, %s", media);
        return -1;
    }
    pos = protocaol_str;
    while(pos && *pos==' ')
    {
        pos ++;
    }
    if(pos >= end)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "media line parsing fmt failed, media line %s", media);
        return-1;
    }
    fmts.len = end - pos;
    fmts.s = new char[fmts.len+1];
    snprintf(fmts.s, fmts.len+1, "%s", pos);
    parsed = 1;
    return 0;
}

Sdp_session::Sdp_session()
{
    snprintf(m_version, sizeof(m_version), "v=0\r\n");
    m_timing.s = NULL;
    m_timing.len = 0;
    m_session_name.s = NULL;
    m_session_name.len = 0;
    m_parsed = 0;
}

Sdp_session::~Sdp_session()
{
    if(m_timing.len)
    {
        delete[] m_timing.s;
        m_timing.s = NULL;
        m_timing.len = 0;
    }
    if(m_session_name.len)
    {
        delete[] m_session_name.s;
        m_session_name.s = NULL;
        m_session_name.len = 0;
    }

    Attr_map::iterator ite;
    for (ite = m_global_attrs_map.begin(); ite != m_global_attrs_map.end(); )
    {
        delete ite->second;
        m_global_attrs_map.erase(ite++);
    }

    Medias_l::iterator ite_l = m_media_l.begin();
    for(; ite_l != m_media_l.end(); )
    {
        if(*ite_l)
        { 
            delete *ite_l;
        }
        else
        {   
            tracelog("RTP", ERROR_LOG, __FILE__, __LINE__, "unknow issue, sdp media instance is NULL");
        }
        ite_l = m_media_l.erase(ite_l);
    }
}

int Sdp_session::parse(char* sdp, int len)
{
    char* nextline = NULL;
    char* end = NULL;
    Sdp_media* media = NULL;
    if(!sdp || len<=0)
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp parsing error becuse of sdp is null");
        return -1;
    }
    end = sdp+(len-2); // point to the last "\r\n" 
    while(sdp && *sdp == ' ')
    {
        sdp++;
    }
    if(!sdp[0] != 'v' || sdp[1] != '=' || sdp[2]!='0')
    {
        tracelog("RTP", WARNING_LOG, __FILE__, __LINE__, "sdp parsing error becuse of sdp is not start with v=0, %s", sdp);
        return -1;
    }
    nextline = strstr(sdp, "\r\n");
    while(nextline && nextline<end)
    {
        char* thisline = nextline+2;
        while(thisline && *thisline == ' ')
        {
            thisline++;
        }
        nextline = strstr(thisline, "\r\n");
        switch (thisline[0])
        {
            case 'o':
            {
                m_orign.parse(thisline);
                break;
            }
            case 's':
            {
                m_session_name.len = nextline-thisline+2;
                m_session_name.s = new char[m_session_name.len+1];
                snprintf(m_session_name.s, m_session_name.len+1, "%s", thisline);
                break;
            }
            case 'c':
            {
                m_con.parse(thisline);
                break;
            }
            case 't':
            {
                m_timing.len = nextline-thisline+2;
                m_timing.s = new char[m_timing.len+1];
                snprintf(m_timing.s, m_timing.len+1, "%s", thisline);
                break;
            }
            case 'm':
            {
                media = new Sdp_media();
                //media->parse(thisline);
                m_media_l.push_back(media);
                break;
            }
            case 'a':
            {
                break;
            }
            default:
            {

            }
        }
    }
    return 0;   
}


