#include "tinyxml.h"
#include "signalingConfiguration.h"
#include "connection.h"
#include "controlProcess.h"

#include <assert.h>

static SIGNALING_CONTROL_CONFIG s_signaling_configures;
static RTP_CONFIG s_rtp_config;

void parseRtpConfiguration(const char* path_name)
{
    TiXmlDocument xml_conf;
    int interfaceNum = 0;
    if(!xml_conf.LoadFile(path_name))
    {
        assert(NULL);
        return;
    }

    // root poit
    TiXmlElement* xml_root = xml_conf.RootElement();
    if(NULL == xml_root)
    {
        assert(NULL);
        return;
    }

    TiXmlElement* signaling_settings = xml_root->FirstChildElement("signalingsettings");
    if(!signaling_settings)
    {
        assert(NULL);
    }

    // signaling interface info start
    s_signaling_configures.interface_num = 0;
    TiXmlElement* signal_interface = signaling_settings->FirstChildElement("interface");
    for(; signal_interface!=NULL; signal_interface=signal_interface->NextSiblingElement())
    {
        interfaceNum++;  // to get the signaling interface number
    }
    assert(interfaceNum>0);
    s_signaling_configures.interfaces = new SIGNALING_INTERFACE[interfaceNum];
    signal_interface = signaling_settings->FirstChildElement("interface");
    for(; signal_interface!=NULL; signal_interface=signal_interface->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = signal_interface->FirstAttribute();
        memset(&s_signaling_configures.interfaces[s_signaling_configures.interface_num], 0, sizeof(SIGNALING_INTERFACE));
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_signaling_configures.interfaces[s_signaling_configures.interface_num]->ip, 
                         sizeof(s_signaling_configures.interfaces[0]->ip), "%s", value);
            }
            else if(strncmp(name,"port", strlen("port")) == 0)
            {
                s_signaling_configures.interfaces[s_signaling_configures.interface_num]->port = atoi(value);
            }
            else if(strncmp(name,"transport", strlen("transport")) == 0)
            {
                 if(strstr(value, "TCP") || strstr(value, "tcp"))
                 {
                     s_signaling_configures.interfaces[s_signaling_configures.interface_num]->transport = TCP;
                 }
                 else if(strstr(value, "UDP") || strstr(value, "udp"))
                 {
                     s_signaling_configures.interfaces[s_signaling_configures.interface_num]->transport = UDP;
                 }
                 else
                 {
                     assert(NULL);
                 }
            }
        }
        s_signaling_configures.interface_num++;
    }
    // signaling interface info end

    TiXmlElement* rtp_settings = xml_root->FirstChildElement("rtpsettings");
    if(!rtp_settings)
    {
        assert(NULL);
    }

    // rtp external interfaces start
    interfaceNum = 0;
    s_rtp_config.external_interface_num = 0;
    TiXmlElement* rtp_external_interface = rtp_settings->FirstChildElement("external");
    for(; rtp_external_interface != NULL; rtp_external_interface=rtp_external_interface->NextSiblingElement())
    {
        interfaceNum++;  // to get the rtp external interface number
    }
    assert(interfaceNum>0);
    s_rtp_config.external_interfaces = new SIGNALING_INTERFACE[interfaceNum];
    rtp_external_interface = rtp_settings->FirstChildElement("external");
    for(; rtp_external_interface!=NULL; rtp_external_interface=rtp_external_interface->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = rtp_external_interface->FirstAttribute();
        memset(&s_rtp_config.external_interfaces[s_rtp_config.external_interface_num], 0, sizeof(SIGNALING_INTERFACE));
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_rtp_config.external_interfaces[s_rtp_config.external_interface_num]->ip, 
                         sizeof(s_rtp_config.external_interfaces[0]->ip), "%s", value);
            }
            else
            {
                continue;
            }
        }
        s_rtp_config.external_interface_num++;
    }
    // rtp external interfaces end
    
    // rtp internal interfaces start
    interfaceNum = 0;
    s_rtp_config.internal_interface_num = 0;
    TiXmlElement* rtp_internal_interface = rtp_settings->FirstChildElement("internal");
    for(; rtp_internal_interface != NULL; rtp_internal_interface=rtp_internal_interface->NextSiblingElement())
    {
        interfaceNum++;  // to get the rtp internal interface number
    }
    assert(interfaceNum>0);
    s_rtp_config.internal_interfaces = new SIGNALING_INTERFACE[interfaceNum];
    rtp_internal_interface = rtp_settings->FirstChildElement("internal");
    for(; rtp_internal_interface!=NULL; rtp_internal_interface=rtp_internal_interface->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = rtp_internal_interface->FirstAttribute();
        memset(&s_rtp_config.internal_interfaces[s_rtp_config.internal_interface_num], 0, sizeof(SIGNALING_INTERFACE));
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_rtp_config.internal_interfaces[s_rtp_config.internal_interface_num]->ip, 
                         sizeof(s_rtp_config.internal_interfaces[0]->ip), "%s", value);
            }
            else
            {
                continue;
            }
        }
        s_rtp_config.internal_interface_num++;
    }
    // rtp internal interfaces end

    // rtp parameters start
    s_rtp_config.rtpThreads=1;
    s_rtp_config.srtpThreads=1;
    s_rtp_config.minRtpPort=10000;
    s_rtp_config.maxRtpPort=60000;
    TiXmlElement* rtp_params = rtp_settings->FirstChildElement("param");
    for(; rtp_params!=NULL; rtp_params=rtp_params->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = rtp_params->FirstAttribute();
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"rtpThreads", strlen("rtpThreads")) == 0)
            {
                s_rtp_config.rtpThreads = atoi(value);
            }
            else if(strncmp(name,"srtpThreads", strlen("srtpThreads")) == 0)
            {
                s_rtp_config.srtpThreads = atoi(value);
            }
            else if(strncmp(name,"portMin", strlen("portMin")) == 0)
            {
                s_rtp_config.minRtpPort = atoi(value);;
            }
            else if(strncmp(name,"portMax", strlen("portMax")) == 0)
            {
                s_rtp_config.maxRtpPort = atoi(value);;
            }
            else
            {
                continue;
            }
        }
    }
    // rtp parameters end
}


int initRTP(const char* config_file)
{
    bool result = 0;
    parseRtpConfiguration(config_file);
    ControlProcess
    return result;
}
