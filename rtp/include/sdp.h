#ifndef _RTP_SDP_H_
#define _RTP_SDP_H_

#include "cstr.h"
#include "map"



enum NetType
{
    IN = 0,
    MAX_NETWORK_TYPE
};

enum AddrType
{
    IP4=0,
    IP6,
    MAX_ADDR_TYPE
};

enum TransProtoc0l
{
    RTP_AVP=0,    // RFC 3550
    RTP_SAVP,     // RFC 3711
    RTP_SAVPF,    // RFC 5124
    MAX_TRANSPORT_PROTOCOL
};

enum MediaType
{
    AUDIO = 0,
    VIDEO, 
    TEXT, 
    APPLICATION,
    MESSAGE,
    MAX_MEDIA_TYPE
};

enum PayloadType
{
    PCMU=0,
    GSM=3,
    G723=4,
    PCMA=8,
    G722=9,
    G728=15,
    G729=18,
    MAX_PAYLOAD_TYPE=0x7FFF
};

enum AttrTYpe 
{
    ATTR_OTHER = 0,
    ATTR_RTCP,
    ATTR_CANDIDATE,
    ATTR_ICE,
    ATTR_ICE_LITE,
    ATTR_ICE_OPTIONS,
    ATTR_ICE_UFRAG,
    ATTR_ICE_PWD,
    ATTR_CRYPTO,
    ATTR_SSRC,
    ATTR_INACTIVE,
    ATTR_SENDRECV,
    ATTR_SENDONLY,
    ATTR_RECVONLY,
    ATTR_RTCP_MUX,
    ATTR_EXTMAP,
    ATTR_GROUP,
    ATTR_MID,
    ATTR_FINGERPRINT,
    ATTR_SETUP,
    ATTR_RTPMAP,
    ATTR_FMTP,
    ATTR_IGNORE,
    ATTR_RTPENGINE,
    ATTR_PTIME,
    ATTR_RTCP_FB,
    ATTR_T38FAXVERSION,
    ATTR_T38FAXUDPEC,
    ATTR_T38FAXUDPECDEPTH,
    ATTR_T38FAXUDPFECMAXSPAN,
    ATTR_T38FAXMAXDATAGRAM,
    ATTR_T38FAXMAXIFP,
    ATTR_T38FAXFILLBITREMOVAL,
    ATTR_T38FAXTRANSCODINGMMR,
    ATTR_T38FAXTRANSCODINGJBIG,
    ATTR_T38FAXRATEMANAGEMENT,
    ATTR_END_OF_CANDIDATES,
};

class Network_address 
{
public:
    Network_address();
    virtual ~Network_address();
public:
    unsigned char    net_type;      // IN has the meaning "internet"
    unsigned char    addr_type;     // IP4 and IP6 are defined
    cstr             address;       
    //sockaddr_t       socket_addr;
};

class Sdp_origin 
{
public:
    Sdp_origin();
    virtual ~Sdp_origin();
public:
    cstr username;
    cstr session_id;
    unsigned long long version_num;
    struct Network_address address;
};

class Sdp_connection 
{
public:
    Sdp_connection();
    virtual ~Sdp_connection();
public:    
    struct Network_address address;
};

class Sdp_attribute 
{
public:
    Sdp_attribute();
    virtual ~Sdp_attribute();
    virtual int serialize(char* buf, int buflen) = 0;
public:
    unsigned char attr_type;
};

// a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encodingparameters>]
class Attr_rtpmap : public Sdp_attribute
{
public:
    Attr_rtpmap();
    virtual ~Attr_rtpmap();
    virtual int serialize(char* buf, int buflen);
public:
    cstr encoding_str;
    cstr clock_rate_str;
    unsigned short payload_type;
};


//Codec-specific parameters should be added in attributes "a=fmtp:".
class Attr_fmtp : public Sdp_attribute
{
public:
    Attr_fmtp();
    virtual ~Attr_fmtp();
    virtual int serialize(char* buf, int buflen);
public:
    cstr format_parms_str;
    unsigned short payload_type;
};

typedef std::map<int, Attr_rtpmap*> rtpmap_attr_map;
typedef std::map<int, Sdp_attribute*> sdp_attr_map;

class Sdp_media 
{
public:
    Sdp_media();
    virtual ~Sdp_media();
public:
    /* RFC 3551 table 4, 5
     for static fmt, maybe SDP omit the attribute of rtpmap,
     for example:
     m=audio 12345 RTP/AVP 0 8 102
     a=rtpmap:102 opus/48000/2
     omit a=rtpmap:0 PCMU/8000
     and  a=rtpmap:8 PCMA/8000
     */
    int*                 fmts;
    rtpmap_attr_map      rtpmap_attrs;
    sdp_attr_map         sdp_attrs;
    unsigned short       port;
    unsigned short       port_count;
    unsigned char        media_type;
    unsigned char        transport;
    unsigned char        fmts_num;
};

#endif
