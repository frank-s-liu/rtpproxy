#include "logclient.h"
#include "log.h"
#include "hash.h"


//  std include
#include<string.h>
#include <stdio.h>

static char loglevel[LOG_LEVEL_SUM][10] = {"ERROR", "WARNING", "INFO", "DEBUG"};
static __thread char s_thread_id[32] = {0};

Logclient* Logclient::s_instance = NULL;

Logclient::Logclient()
{
    //pthread_mutex_init(&m_lock, NULL);
    m_isEnable = true;
    for(int i=0; i<MODULE_SIZE; i++)
    {
        for(int j=0; j<MODULE_SIZE; j++)
        {
            m_modules[i][j] = -1;
        }
    }
    init();
}

Logclient::~Logclient()
{
    //pthread_mutex_destroy(&m_lock);
}

Logclient*Logclient:: getInstance()
{
    if(NULL == s_instance)
    {
        s_instance = new Logclient();
    }
    return s_instance;
}

void Logclient::init()
{
}

int Logclient::moduleRegister(const char* moduleName, unsigned int leval)
{
    if(NULL == moduleName || 
       strlen(moduleName) > 64 || 
       strlen(moduleName) < 3 || 
       leval >= LOG_LEVEL_SUM)
    {
        return -1;
    }
    long i = BKDRHash(moduleName, 2) & MODULE_SIZE_MASK;
    long j = BKDRHash(moduleName, strlen(moduleName)) & MODULE_SIZE_MASK;
    int ok = __sync_bool_compare_and_swap(&m_modules[i][j], -1, leval);
    if(!ok)
    {
        return -1;
    }
    return 0;
}

int Logclient::moduleDeRegister(const char* moduleName)
{
    return modifylogLevel(moduleName, -1);
}

int Logclient::modifylogLevel(const char* moduleName, int level)
{
    if(NULL == moduleName || 
       strlen(moduleName) > 64 || 
       strlen(moduleName) < 3 || level > LOG_LEVEL_SUM)
    {
        return -1;
    }
    long i = BKDRHash(moduleName, 2) & MODULE_SIZE_MASK;
    long j = BKDRHash(moduleName, strlen(moduleName)) & MODULE_SIZE_MASK;
    m_modules[i][j] = level;
    return 0;
}

int Logclient::logcheck(const char* moduleName, int leval)
{
    if(NULL == moduleName)
    {
        return 1;
    }
    long i = BKDRHash(moduleName, 2) & MODULE_SIZE_MASK;
    long j = BKDRHash(moduleName, strlen(moduleName)) & MODULE_SIZE_MASK;
    if(m_modules[i][j] < leval)
    {
        return 2;
    }
    return 0;   
}

int Logclient::log(const char* moduleName, int leval, const char* file, int line, char* logbuffer, int* buflen)
{
    int tmplen = 0;
    int len = 0;
    char fileInfo[32];
    if(!m_isEnable)
    {
        return 0;
    }
    if(s_thread_id[0] == '\0')
    {
        snprintf(s_thread_id, sizeof(s_thread_id), "tid-%-12u", (unsigned int)pthread_self());
    }
    len = snprintf(logbuffer, 56, "%s                              ", s_thread_id);
    snprintf(fileInfo,sizeof(fileInfo), "%s:%d", file, line);
    tmplen = snprintf(&logbuffer[len], COMMONLOGWIDE-len, "%-8s   %-7s   %-32s   ",moduleName, loglevel[leval], fileInfo);
    len += tmplen;
    *buflen = len;
    return 0;
}


#if 0
void Logclient::set_datetime_str(char* datetime_str, int* len)
{
    time_t nowtime;
    struct tm *timeinfo;
    int datalen = 0;
    time(&nowtime);
    timeinfo = localtime(&nowtime);
    datalen = snprintf(datetime_str, 32, "%04d-%02d-%02d %02d:%02d:%02d    ",
              timeinfo->tm_year + 1900,
              timeinfo->tm_mon + 1,
              timeinfo->tm_mday,
              timeinfo->tm_hour,
              timeinfo->tm_min,
              timeinfo->tm_sec);
    *len += datalen;
}
#endif
