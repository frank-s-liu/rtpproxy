#ifndef LOGCLIENT_H_
#define LOGCLIENT_H_


#include "logclient.h"
#include "resource.h"

// std include
//#include <map>
//#include <pthread.h> 
#include <string>

const int COMMONLOGWIDE = 130;
//typedef std::map<std::string, int> Modules;
const int MODULE_SIZE = 128;
const int MODULE_SIZE_MASK = MODULE_SIZE-1;


class Logclient
{
public:
    virtual ~Logclient();
    int moduleRegister(const char* moduleName, unsigned int leval);   
    int moduleDeRegister(const char* moduleName);
    int log(const char* moduleName, int leval, const char* file, int line, char* logbuffer, int* len);
    static Logclient* getInstance();
    void configureTimeFun(bool isEnable);
    int logcheck(const char* moduleName, int leval);
    int modifylogLevel(const char* moduleName, int level);
private:
    Logclient();
    Logclient (const Logclient& c);
    Logclient& operator = (const Logclient& other);
    void init();
    void set_datetime_str(char* datetime_str, int* len);
private:
    int m_modules[MODULE_SIZE][MODULE_SIZE];
    //pthread_mutex_t m_lock;
    static Logclient* s_instance;
    bool m_isEnable; // to support change some configure online

};

#endif
