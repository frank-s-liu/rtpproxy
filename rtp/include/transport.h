#ifndef __SIPP_TRANSPORT_H__
#define __SIPP_TRANSPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum transport
{
    UDP=0,
    TCP,
    TLS,
    WSS,
    WS,
    MAX_TRANSPORT
}Transport;


typedef enum tlsv{
    TLSV12 = 0,
    TLSV13,
    TLSVMAX
}TLSVER;

#ifdef __cplusplus
}
#endif

#endif
