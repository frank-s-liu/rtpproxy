#include "logserver.h"
#include "logclient.h"

// std include
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
//#include <iostream>

char* LOGNAME = NULL;
const char* DEFAULTFILENAME = "./test.log";
const int LOGSIZE= 268435456; // 256M
const unsigned int IOBUFFERSIZE = 65536;

static char* s_iobuffer = NULL;
static ResourceManager* s_resource = NULL;
Logserver* Logserver::s_logserver = NULL;


Logserver::Logserver():Thread("log2File")
{
    m_filePath_name = NULL;
}

Logserver::Logserver(const char* path, const char* name):Thread("log2File")
{
    if(NULL != path)
    {
        setPath(path, name);
    }
    else
    {
        int namelen = strlen(DEFAULTFILENAME) +1 ;
        m_filePath_name = (char*)ResourceManager::getInstance()->getresource(namelen);
        snprintf(m_filePath_name, namelen, "%s", DEFAULTFILENAME);
    }
    init();
}

Logserver::~Logserver()
{
    if(NULL != m_filePath_name)
    {
        ResourceManager::getInstance()->freeresource(m_filePath_name);
        m_filePath_name = NULL;
    }
    if(m_logfile)
    {
        fclose(m_logfile);
        m_logfile = NULL;
    }
}

void* Logserver::run()
{
     ResourceManager* resource = ResourceManager::getInstance();
     if(NULL == resource)
     {
         return NULL;
     }
     while(!m_isStop)
     {
         writeLog2File();
     }
     return NULL;
}

void Logserver::init()
{
    umask(0022);
    //  default path is "./"
    if( (strncmp(DEFAULTFILENAME, m_filePath_name, strlen(DEFAULTFILENAME)) == 0) )
    {
            m_logfile = fopen(DEFAULTFILENAME, "ab+");
            if(m_logfile == NULL)
            {
                exit(1);
            }
            truncate(DEFAULTFILENAME, 0);
            rewind(m_logfile);
    }
    else
    {
        char* current_pos = m_filePath_name;
        int filePathLen = strlen(m_filePath_name);
        while (*current_pos != '\0')
	{
            if (*current_pos == '\\')
            {
                *current_pos = '/';
            }
            if (*current_pos == '/' && (current_pos != m_filePath_name))
            {
                *current_pos = '\0';
                if(access(m_filePath_name, 0) != 0)
                {
                    mkdir(m_filePath_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                }
                *current_pos = '/';
            }
            current_pos++;
	}
        if (access(m_filePath_name, 0) != 0)
        {
            mkdir(m_filePath_name, 0755);
        }
        if(m_filePath_name[filePathLen-1] != '/')
        {
            m_filePath_name[filePathLen] = '/';
            filePathLen++;
        }
        snprintf(&m_filePath_name[filePathLen], strlen(LOGNAME)+1, "%s", LOGNAME);
        m_logfile = fopen(m_filePath_name, "ab+");
        if(m_logfile == NULL)
        {
            exit(1);
        }
        truncate(m_filePath_name, 0);
        rewind(m_logfile);
    }
    s_resource = ResourceManager::getInstance();
    s_iobuffer = (char*)s_resource->getresource(IOBUFFERSIZE);
    setvbuf (m_logfile, s_iobuffer, _IOFBF, IOBUFFERSIZE);
}

void Logserver::writeLog2File()
{
    char* log;
    char* loginfo;
    int len;
    static int counter = 0;
    if(counter == 0)
    {
        s_resource->updateTime();
    }
    log = (char*)s_resource->poplog();
    if(NULL == log)
    {
        return;
    }
    loginfo = log+4;
    len = *(unsigned int*)log ;
    fwrite(loginfo, len, 1, m_logfile);
    s_resource->freeresource(log);
    counter++;
    if(counter > 1000)
    {
        counter = 0;
        if(get_file_size() >= LOGSIZE)
        {
            fflush(m_logfile);
            rename();
        }
    }
}

unsigned long Logserver::get_file_size()
{
    unsigned long filesize = 0;
    struct stat statbuff;
    if(stat(m_filePath_name, &statbuff) < 0)
    {
        //if ERR, close
        rename();
        return 0;  
    }
    else
    {  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

void Logserver::doStop()
{

}

void Logserver::rename()
{
    char newname[512] = {0};
    char datetime_str[64] = {0};
    time_t nowtime;
    struct tm timeinfo;

    fclose(m_logfile);
    time(&nowtime);
    localtime_r(&nowtime, &timeinfo);
    snprintf(datetime_str,sizeof(datetime_str), "%04d%02d%02d-%02d%02d%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min, 
             timeinfo.tm_sec);
    snprintf(newname, sizeof(newname), "%s", m_filePath_name);
    snprintf(&newname[strlen(m_filePath_name)-4],sizeof(datetime_str), "-%s.log", datetime_str);
    ::rename(m_filePath_name, newname);
    m_logfile = fopen(m_filePath_name, "ab+");
    if(m_logfile == NULL)
    {
        exit(1);
    }
    truncate(m_filePath_name, 0);
    rewind(m_logfile);
    setvbuf (m_logfile, s_iobuffer, _IOFBF, IOBUFFERSIZE);
}

Logserver* Logserver::getInstance()
{
    if(!s_logserver)
    {
        s_logserver = new Logserver();
    }
    return s_logserver;
}

void Logserver::flushcache()
{
    fflush(m_logfile);
}

void Logserver::setPath(const char* path, const char* name)
{
    int pathlen = strlen(path)+1;
    int namelen = strlen(name)+1;
    int len = pathlen + namelen +4;
    m_filePath_name = (char*)ResourceManager::getInstance()->getresource(len);
    strncpy(m_filePath_name, path, pathlen);
    m_filePath_name[pathlen] = '\0';
    LOGNAME = new char[strlen(name)+1];
    snprintf(LOGNAME, namelen, "%s", name);
}
