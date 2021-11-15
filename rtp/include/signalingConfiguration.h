#ifndef SIGNALING_CONFIGURATION_H_
#define SIGNALING_CONFIGURATION_H_

typedef struct signaling_interface
{
    unsigned int port;
    int transport
    char ip[64];
}SIGNALING_INTERFACE;

typedef struct rtp_interface
{
    char ip[64];
}RTP_INTERFACE;

typedef struct signaling_control_config
{
    int interface_num;
    SIGNALING_INTERFACE* interfaces;
}SIGNALING_CONTROL_CONFIG;

typedef struct rtp_config
{
    int external_interface_num;
    int internal_interface_num;
    int rtpThreads;
    int srtpThreads;
    int minRtpPort;
    int maxRtpPort;
    RTP_INTERFACE* external_interfaces;
    RTP_INTERFACE* internal_interfaces;
}RTP_CONFIG;
#endif
