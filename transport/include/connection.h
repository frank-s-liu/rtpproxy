#ifndef __SIPP_TRANSPORT_H__
#define __SIPP_TRANSPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum connectionType
{
    UDP=0,
    TCP,
    TLS,
    MAX_TRANSPORT
}ConnectionType;


#ifdef __cplusplus
}
#endif

#endif
