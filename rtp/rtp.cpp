#include "tinyxml.h"
#include "rtpConfiguration.h"
#include "rtpControlProcess.h"
#include "rtpepoll.h"
#include "rtp.h"

#include <assert.h>

static RTP_CONFIG s_rtp_config;

void parseRtpConfiguration(const char* path_name)
{
    TiXmlDocument xml_conf;
    s_rtp_config.rtp_ctl_interfaces_num = 0;
    int index = 0;
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

    TiXmlElement* rtpcontrol_settings = xml_root->FirstChildElement("rtpconctolsettings");
    if(!rtpcontrol_settings)
    {
        assert(NULL);
    }
    // rtp control interface info start
    s_rtp_config.rtp_ctl_interfaces_num = 0;
    TiXmlElement* rtpcontrol_inf = rtpcontrol_settings->FirstChildElement("interfaces");
    TiXmlElement* param_nic = rtpcontrol_inf->FirstChildElement("param");
    for(; param_nic!=NULL; param_nic=param_nic->NextSiblingElement())
    {
        s_rtp_config.rtp_ctl_interfaces_num++; 
    }
    assert(s_rtp_config.rtp_ctl_interfaces_num>0);
    s_rtp_config.rtpctl_interfaces = new RTP_CTL_ITF_S[s_rtp_config.rtp_ctl_interfaces_num];
    memset(s_rtp_config.rtpctl_interfaces, 0, sizeof(RTP_CTL_ITF_S)*s_rtp_config.rtp_ctl_interfaces_num);
    param_nic = rtpcontrol_inf->FirstChildElement("param");
    for(; param_nic!=NULL; param_nic=param_nic->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = param_nic->FirstAttribute();
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_rtp_config.rtpctl_interfaces[index].ip, 
                         sizeof(s_rtp_config.rtpctl_interfaces[index].ip), "%s", value);
            }
            else if(strncmp(name,"port", strlen("port")) == 0)
            {
                s_rtp_config.rtpctl_interfaces[index].port = atoi(value);
            }
            else if(strncmp(name,"transport", strlen("transport")) == 0)
            {
                 if(strstr(value, "TCP") || strstr(value, "tcp"))
                 {
                     s_rtp_config.rtpctl_interfaces[index].transport = RTP_CTL_TCP;
                 }
                 else if(strstr(value, "UDP") || strstr(value, "udp"))
                 {
                     s_rtp_config.rtpctl_interfaces[index].transport = RTP_CTL_UDP;
                 }
                 else
                 {
                     assert(NULL);
                 }
            }
        }
        index++;
    }
    // rtp control interface info end

    TiXmlElement* rtp_settings = xml_root->FirstChildElement("rtpsettings");
    if(!rtp_settings)
    {
        assert(NULL);
    }

    // rtp external interfaces start
    s_rtp_config.external_interface_num = 0;
    TiXmlElement* rtp_external_interface = rtp_settings->FirstChildElement("external");
    TiXmlElement* external_param = rtp_external_interface->FirstChildElement("param");
    for(; external_param != NULL; external_param=external_param->NextSiblingElement())
    {
        s_rtp_config.external_interface_num++;
    }
    assert(s_rtp_config.external_interface_num>0);
    s_rtp_config.external_interfaces = new RTP_ITF_S[s_rtp_config.external_interface_num];
    external_param = rtp_external_interface->FirstChildElement("param");
    index = 0;
    for(; external_param!=NULL; external_param=external_param->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = external_param->FirstAttribute();
        memset(&s_rtp_config.external_interfaces[index], 0, sizeof(RTP_ITF_S));
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_rtp_config.external_interfaces[index].ip, 
                         sizeof(s_rtp_config.external_interfaces[index].ip), "%s", value);
            }
            else
            {
                continue;
            }
        }
        index++;
    }
    // rtp external interfaces end
 
    // rtp internal interfaces start
    s_rtp_config.internal_interface_num = 0;
    TiXmlElement* rtp_internal_interface = rtp_settings->FirstChildElement("internal");
    TiXmlElement* internal_param = rtp_internal_interface->FirstChildElement("param");
    for(; internal_param != NULL; internal_param=internal_param->NextSiblingElement())
    {
        s_rtp_config.internal_interface_num++;  // to get the rtp internal interface number
    }
    assert(s_rtp_config.internal_interface_num>0);
    s_rtp_config.internal_interfaces = new RTP_ITF_S[s_rtp_config.internal_interface_num];
    internal_param = rtp_internal_interface->FirstChildElement("param");
    index = 0;
    for(; internal_param!=NULL; internal_param=internal_param->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = internal_param->FirstAttribute();
        memset(&s_rtp_config.internal_interfaces[index], 0, sizeof(RTP_ITF_S));
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"ip", strlen("ip")) == 0)
            {
                snprintf(s_rtp_config.internal_interfaces[index].ip, 
                         sizeof(s_rtp_config.internal_interfaces[index].ip), "%s", value);
            }
            else
            {
                continue;
            }
        }
        index++;
    }
    // rtp internal interfaces end
    // rtp parameters start
    s_rtp_config.rtpThreads=1;
    s_rtp_config.minRtpPort=10000;
    s_rtp_config.maxRtpPort=60000;
    TiXmlElement* rtp_params = rtp_settings->FirstChildElement("params");
    TiXmlElement* param = rtp_params->FirstChildElement("param");
    for(; param!=NULL; param=param->NextSiblingElement())
    {
        TiXmlAttribute* attributeOfParam = param->FirstAttribute();
        for( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next())
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"rtpThreads", strlen("rtpThreads")) == 0)
            {
                s_rtp_config.rtpThreads = atoi(value);
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
    //ControlProcess
    ControlProcess::getInstance()->start();

    delete[] s_rtp_config.rtpctl_interfaces;
    s_rtp_config.rtpctl_interfaces = NULL;
    delete[] s_rtp_config.external_interfaces;
    s_rtp_config.external_interfaces = NULL;
    delete[] s_rtp_config.internal_interfaces;
    s_rtp_config.internal_interfaces = NULL;
    return result;
}

RTP_CONFIG* getRtpConf()
{
    return &s_rtp_config;
}
