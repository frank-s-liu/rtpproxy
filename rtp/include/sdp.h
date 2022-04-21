#ifndef _RTP_SDP_H_
#define _RTP_SDP_H_

#include "cstr.h"
#include "rtpEnum.h"

#include <list>
#include <string>


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

enum TransProtocol
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
    int parse(const char* network);
    int serialize(char* buf, int buflen);
    Network_address& operator =(const Network_address& rna);
private:
    Network_address(const Network_address& rna);  // don't allow copy construct function
public:
    unsigned char    net_type;      // IN has the meaning "internet"
    unsigned char    addr_type;     // IP4 and IP6 are defined
    cstr             address;       
    //sockaddr_t       socket_addr;
};

/* example:
 * o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5
 */
class Sdp_origin 
{
public:
    Sdp_origin();
    virtual ~Sdp_origin();
    int parse(const char* origin);
    int serialize(char* buf, int buflen);
    int replaceAddress(const char* ip, int len);
public:
    cstr username;
    cstr session_id;
    Network_address address;
    unsigned long version_num;
    unsigned char parsed:1;
};

class Sdp_connection 
{
public:
    Sdp_connection();
    virtual ~Sdp_connection();
    int parse(const char* network);
    int serialize(char* buf, int buflen);
    int replaceAddress(const char* ip, int len);
public:    
    Network_address address;
    unsigned char parsed:1;
};

class Sdp_attribute 
{
public:
    Sdp_attribute();
    virtual ~Sdp_attribute();
    virtual int serialize(char* buf, int buflen) = 0;
    virtual int parse(const char* line) = 0;
public:
    unsigned char attr_type:6;
    unsigned char parsed:1;
};

// a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encodingparameters>]
class Attr_rtpmap : public Sdp_attribute
{
public:
    Attr_rtpmap();
    virtual ~Attr_rtpmap();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
public:
    cstr encoding_str;
    cstr clock_rate_str;
    unsigned short payload_type;
};

// Codec-specific parameters should be added in attributes "a=fmtp:".
class Attr_fmtp : public Sdp_attribute
{
public:
    Attr_fmtp();
    virtual ~Attr_fmtp();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
public:
    cstr format_parms_str;
    unsigned short payload_type;
};

// a=crypto:<tag> <crypto-suite> <key-params> [<session-params>]
// key-params = <key-method> ":" <key-info> , such as  "inline:" <key||salt> ["|" lifetime] ["|" MKI ":" length]
// a=crypto:2 AES_CM_128_HMAC_SHA1_80 inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz|2^20|1:4;
class Attr_crypto : public Sdp_attribute
{
public:
    Attr_crypto();
    virtual ~Attr_crypto();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
public:
    cstr                     suite_str;
    cstr                     key_params;
    unsigned char            mki_v;     // this value in network order was appending in srtp package, the byte length is mki_len. 
    unsigned short           tag;
    unsigned char            mki_len;   // MKI and byte length of the MKI field in SRTP packets 
                                        // Only 8-bit alignment is assumed. RFC 3711
                                        // normally, length can be 1(char), 2(short), 4(int), 8(long), now not support for others
    unsigned char            lifetime;  // max number of SRTP or SRTCP packets using this master key  max packages = ( 1 < lifetime)
};

class Attr_sendrecv : public Sdp_attribute
{
public:
    Attr_sendrecv();
    virtual ~Attr_sendrecv();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
};

/* RFC 3605
 * a=rtcp:53020
 * a=rtcp:53020 IN IP4 126.16.64.4
 * it MUST NOT be used as a session level attribute 
 */
class Attr_rtcp : public Sdp_attribute
{
public:
    Attr_rtcp();
    virtual ~Attr_rtcp();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
public:
    unsigned short      port;
    Network_address     address;
};

class Attr_unknown : public Sdp_attribute
{
public:
    Attr_unknown();
    virtual ~Attr_unknown();
    virtual int serialize(char* buf, int buflen);
    virtual int parse(const char* line);
public:
    cstr    line;
};

typedef std::list<Sdp_attribute*> Attrs_l;

class Sdp_media 
{
public:
    Sdp_media();
    virtual ~Sdp_media();
    int parse(const char* media);
    int serialize(char* buf, int buflen);
    int replacePort(unsigned short port);
    int replaceTransport(unsigned char type);
    int removecryptoAttrs();
public:
    /* RFC 3551 table 4, 5
     for static fmt, maybe SDP omit the attribute of rtpmap,
     for example:
     m=audio 12345 RTP/AVP 0 8 102
     a=rtpmap:102 opus/48000/2
     omit a=rtpmap:0 PCMU/8000
     and  a=rtpmap:8 PCMA/8000
     */
    Attrs_l              attrs;
    cstr                 fmts;
    unsigned short       port;
    unsigned short       port_count;
    unsigned char        media_type;
    unsigned char        transport;
    unsigned char        parsed:1;
};

typedef std::list<Sdp_media*> Medias_l;
class Sdp_session
{
public:
    Sdp_session();
    virtual ~Sdp_session();
    int parse(const char* sdp, int len);
    int serialize(char* buf, int* buflen);
    int replaceOrigin(const char* ip, int iplen);
    int replaceCon(const char* ip, int iplen);
    int replaceMedia(unsigned short port, unsigned char transport);
    int removeCryptoAttr();
    Sdp_attribute* getcryptoAttrFromAudioMedia(Crypto_Suite chip);
public:
    Sdp_origin       m_orign;
    Sdp_connection   m_con;
    cstr             m_timing;
    cstr             m_session_name;
    Medias_l         m_media_l;
    Attrs_l          m_global_attrs_l;
    char             m_version[8];
    unsigned char    m_parsed:1;
};

#endif
