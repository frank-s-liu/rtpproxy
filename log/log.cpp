#include "logclient.h"
#include "logserver.h"
#include "log.h"

//std include
#include <stdarg.h>
#include <stdlib.h>


void initLog(const char* logPath)
{
    Logserver::getInstance()->setPath(logPath);
    Logserver::getInstance()->init();
    Logserver::getInstance()->start();
    Logclient::getInstance();
}


int regiterLog(const char* moduleName, int level)
{
    return Logclient::getInstance()->moduleRegister(moduleName, level);
}

int modifylogLevel(const char* moduleName, int level)
{
    return Logclient::getInstance()->modifylogLevel(moduleName, level);
}

void deRegLog(const char* module)
{
    Logclient * client = Logclient::getInstance();
    client->moduleDeRegister(module);
}

void tracelog(const char* moduleName, int level, const char* file, int line, const char* fmt, ... )
//void switch_log_printf(int channel, const char *file, const char *func, int line, const char *userdata, int level, const char *fmt, ...)
{
    //const char* moduleName = "TESTPERFORMANCE";
    //level = DEBUG_LOG;
    Logclient * client = Logclient::getInstance();
    if(NULL != client)
    {
        char* buffer = NULL;
        va_list args;
        va_list ap;
        int len = 0;
        int commonlen = 0;
        int datalen = 0;
        char* logbuffer = NULL;
        if(client->logcheck(moduleName, level))
        {
            return;
        }
        va_start(args, fmt);
        va_copy(ap, args);
        datalen = vsnprintf(buffer, 0, fmt, ap);
        len = datalen + COMMONLOGWIDE+3;      
        buffer = (char*)ResourceManager::getInstance()->getresource(len);
        logbuffer = buffer+4;
        client->log(moduleName, level, file, line, logbuffer, &commonlen);
        if(commonlen >= COMMONLOGWIDE)
        {
            commonlen = COMMONLOGWIDE-1;
        }
#if 1
        vsnprintf(logbuffer+commonlen,len-commonlen, fmt, args);
        if(logbuffer[commonlen + datalen -1] != '\n')
        {
            logbuffer[commonlen + datalen] = '\r';
            logbuffer[commonlen + datalen + 1] = '\n';
            logbuffer[commonlen + datalen + 2] = '\0';
            *(unsigned int*)buffer = commonlen + datalen +2;
        }
        else
        {
            logbuffer[commonlen + datalen] = '\0';
            *(unsigned int*)buffer = commonlen + datalen;
        }
#endif
        //buffer[commonlen] = '\n';
        //buffer[commonlen+1]='\0'; 
        ResourceManager::getInstance()->pushlog(buffer);
        va_end(ap);
        va_end(args);
    }
}
