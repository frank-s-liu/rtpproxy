#ifndef SIGNALING_CONFIGURATION_H_
#define SIGNALING_CONFIGURATION_H_

typedef struct rtp_control_interface
{
    unsigned short transport;
    unsigned short port;
    char ip[64];
}RTP_CTL_ITF_S;

typedef struct rtp_interface
{
    char ip[64];
}RTP_ITF_S;

typedef struct rtp_config
{
    RTP_ITF_S*        external_interfaces;
    RTP_ITF_S*        internal_interfaces;
    RTP_CTL_ITF_S*    rtpctl_interfaces;
    unsigned short    minRtpPort;
    unsigned short    maxRtpPort;
    unsigned char     external_interface_num;
    unsigned char     internal_interface_num;
    unsigned char     rtp_ctl_interfaces_num;
    unsigned char     rtpThreads;
}RTP_CONFIG;

RTP_CONFIG* getRtpConf();

#endif
