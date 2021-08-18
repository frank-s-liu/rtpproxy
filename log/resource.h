#ifndef LOG_RESOURCE_H
#define LOG_RESOURCE_H


#include <stddef.h>
#include <string.h>


class ResourceManager
{
public:
    virtual ~ResourceManager();
    static ResourceManager* getInstance();
    static void* operator new(size_t sz);
    void* getresource(unsigned int size);
    void freeresource(void* data);
    void pushlog(void* data);
    void* poplog();
    void updateTime();
private:
    ResourceManager();
    ResourceManager(const ResourceManager& r);
    ResourceManager& operator = (const ResourceManager& other);
    static void operator delete(void* p);
private:
    static ResourceManager* s_instance;
    //log_queue* m_logqueue;
};
 
#endif
