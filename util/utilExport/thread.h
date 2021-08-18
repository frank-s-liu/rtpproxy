#ifndef  UTIL_THREAD_H
#define UTIL_THREAD_H

#include <pthread.h>
#include <sys/time.h>

class Thread
{
public:
    Thread();
    Thread(int stackSize);// stackSize: Byte
    Thread(const char* name);
    virtual ~Thread();
    bool start();
    void stop(int millisecond = -1); // used for none de-attach mode
    void setAttachMode(int attach);
    virtual void doStop(); // this should be public method
protected:
    virtual bool preStart();
    virtual void* run() = 0;
    virtual bool join(int millisecond = -1);


protected:
    pthread_t m_thread_id;
    char m_thread_name[20]; 
    unsigned int m_stackSize;
    int m_deAttach_state;
    bool m_isStop;
    int m_status;
    enum Attach_state
    {
        ATTACH = 0,
        DE_ATTACH
    };
    enum ThreadStatus
    {
        THREAD_RUNNING = 0,
        THREAD_STOPPED
    };

private:
    static void* threadEntry(void* param);
    static void getTimeout(struct timespec *spec, int timer);
    void initThread(); 

private:
    short m_namelen;
};

#endif
