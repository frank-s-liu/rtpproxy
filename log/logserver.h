#ifndef LOG_SERVER_H
#define LOG_SERVER_H
#include "thread.h"

// std include
#include <stdio.h>

class Logserver : public Thread
{
public:
    virtual ~Logserver();
    virtual void* run();
    void flushcache();
    static Logserver* getInstance();
    void setPath(const char* path, const char* name);
    void init();
    virtual void doStop();
private:
    Logserver();
    Logserver(const char* path, const char* name);
    Logserver(const Logserver& l);
    Logserver& operator = (const Logserver& other);
    void writeLog2File();
    unsigned long get_file_size();
    void rename();
    void updateTime();
private:
    char* m_filePath_name;
    FILE* m_logfile;
    static Logserver* s_logserver;
};

#endif
