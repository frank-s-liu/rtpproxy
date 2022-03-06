#include "log.h"
#include "tinyxml.h"
#include "task.h"
#include "rtp.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

//DEBUG
#include <stdio.h>
#include <unistd.h>

typedef struct logmodule
{
    char*               module_name;
    int                 level;
    struct logmodule *  next;
}LogModule_l;

typedef struct logconfig
{
    char*          logpath;
    char*          logname;
    LogModule_l*   modules;
}LogConfigure;

void daemonize()
{
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    // Clear file creation mask.
    umask(0);

    // Get maximum number of file descriptors. 
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        exit(1);
    }
    // Become a session leader to lose controlling TTY. 
    if ((pid = fork()) < 0)
    {
        exit(1);
    }
    else if (pid != 0)
    {
        // parent exit
        exit(0);
    }
    setsid();

    // Ensure future opens won't allocate controlling TTYs. 
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        exit(1);
    }

    if ((pid = fork()) < 0)
    {
        exit(1);
    }
    else if (pid != 0)
    {
        exit(0);
    }

    close(0);
    close(1);
    close(2);
    // Attach file descriptors 0, 1, and 2 to /dev/null. 
    fd0 = open("/dev/null", O_RDWR); // fd0 == 0
    fd1 = dup(0);                    // fd1 == 1
    fd2 = dup(0);                    // fd2 == 2
    (void)fd0;
    (void)fd1;
    (void)fd2;
}

int parseXMLconfiguration(const char* path_name, LogConfigure* logconfigs)
{
    TiXmlDocument xml_conf;
    if(!xml_conf.LoadFile(path_name))
    {
        assert(0);
        return -1;
    }

    // root poit
    TiXmlElement* xml_root = xml_conf.RootElement();
    if (NULL == xml_root)
    {
        exit(1);
        return -1;
    }
    
    TiXmlElement* xml_settings = xml_root->FirstChildElement("logsettings");
    TiXmlElement* xml_param = xml_settings->FirstChildElement("param");
    LogModule_l* tail = logconfigs->modules;
    assert(tail == NULL);
    for (; xml_param != NULL; xml_param = xml_param->NextSiblingElement() ) 
    {
        LogModule_l* module = NULL;
        TiXmlAttribute* attributeOfParam = xml_param->FirstAttribute();
        for ( ; attributeOfParam != NULL; attributeOfParam = attributeOfParam->Next() )
        {
            const char* name = attributeOfParam->Name();
            const char* value = attributeOfParam->Value();
            if(strncmp(name,"logpath", strlen("logpath")) == 0)
            {
                int len = strlen(value)+1;
                logconfigs->logpath = new char[len];
                snprintf(logconfigs->logpath,len, "%s", value);
            }
            else if(strncmp(name,"logname", strlen("logname")) == 0)
            {
                int len = strlen(value)+1;
                logconfigs->logname = new char[len];
                snprintf(logconfigs->logname,len, "%s", value);
            }
            else if(strncmp(name,"module", strlen("module")) == 0)
            {
                int len = strlen(value)+1;
                if(module == NULL)
                {
                    module = new LogModule_l();
                    module->module_name = NULL;
                    module->level = INFO_LOG;
                    module->next = NULL;
                }
                if(module->module_name)
                {
                    delete module->module_name;
                }
                module->module_name = new char[len];
                snprintf(module->module_name, len, "%s", value);
            }
            else if(strncmp(name,"loglevel", strlen("loglevel")) == 0)
            {
                if(module == NULL)
                {
                    module = new LogModule_l();
                    module->module_name = NULL;
                    module->level = INFO_LOG;
                    module->next = NULL;
                }
                if(strstr(value, "ERROR"))
                {
                    module->level = ERROR_LOG;
                }
                else if(strstr(value, "WARN"))
                {
                    module->level = WARNING_LOG;
                }
                else if(strstr(value, "DEBUG"))
                {
                    module->level = DEBUG_LOG;
                }
            }
        }
        if(module)
        {
            if(tail == NULL)
            {
                tail = module;
                logconfigs->modules = module;
            }
            else
            {
                tail->next = module;
                tail = module;
            }
        }
    }

    return 0;
}

void show_sig_handler(int sig)
{
    tracelog("SIGNAL", WARNING_LOG, __FILE__, __LINE__, "I got signal %d\n", sig);
    usleep(200000);
    tracelog("SIGNAL", WARNING_LOG, __FILE__, __LINE__, "I got signal %d and sleep 200ms\n", sig);
}

void  processSignal()
{
    // Ignore the SIGPIPE signal
    struct sigaction action_pipe;
    struct sigaction action_init;
    struct sigaction action_quit;
    struct sigaction action_hup;
    struct sigaction action_ill;
    struct sigaction action_kill;
    struct sigaction action_alarm;
    struct sigaction action_term;
    struct sigaction action_continue;

    memset(&action_pipe, 0, sizeof(struct sigaction));
    memset(&action_init, 0, sizeof(struct sigaction));
    memset(&action_quit, 0, sizeof(struct sigaction));
    memset(&action_hup, 0, sizeof(struct sigaction));
    memset(&action_ill, 0, sizeof(struct sigaction));
    memset(&action_kill, 0, sizeof(struct sigaction));
    memset(&action_alarm, 0, sizeof(struct sigaction));
    memset(&action_term, 0, sizeof(struct sigaction));
    memset(&action_continue, 0, sizeof(struct sigaction));

    action_pipe.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &action_pipe, NULL); 
    
    action_init.sa_handler = show_sig_handler;
    sigaction(SIGINT, &action_init, NULL); 

    action_quit.sa_handler = show_sig_handler;
    sigaction(SIGQUIT, &action_quit, NULL); 

    action_hup.sa_handler = show_sig_handler;
    sigaction(SIGHUP, &action_hup, NULL); 

    action_ill.sa_handler = show_sig_handler;
    sigaction(SIGILL, &action_ill, NULL); 

    action_kill.sa_handler = show_sig_handler;
    sigaction(SIGKILL, &action_kill, NULL); 

    action_alarm.sa_handler = show_sig_handler;
    sigaction(SIGALRM, &action_alarm, NULL); 

    action_term.sa_handler = show_sig_handler;
    sigaction(SIGTERM, &action_term, NULL);
 
    action_continue.sa_handler = show_sig_handler;
    sigaction(SIGCONT, &action_continue, NULL);
}

static const  char* configurationFile = "./conf/conf.xml";
void init()
{
    LogConfigure log_conf = {.logpath=NULL, .logname=NULL, .modules=NULL};   
    parseXMLconfiguration(configurationFile, &log_conf);
    initLog(log_conf.logpath, log_conf.logname);
    delete[] log_conf.logpath;
    delete[] log_conf.logname;
    log_conf.logpath = NULL;
    log_conf.logname = NULL;
    LogModule_l* module = log_conf.modules;
    while(module)
    {
        regiterLog(module->module_name, module->level);
        LogModule_l* tmp = module;
        delete[] module->module_name;
        module = module->next;
        delete tmp;
    }
    log_conf.modules = NULL;

    processSignal();
    initTask();
    initRTP(configurationFile);
    //initcli(s_configures.cliip, s_configures.cliport); 
}

int main(int argc, char* argv[])
{
    {
        struct option longOpts[] = { {"help", 0, 0, 'h'}, {"cwd", 1, 0, 'c'} };
        char dir[256] = {0};
        while(1)
        {
            int option_index = 0; 
            int opt = getopt_long(argc, argv, "c:h", longOpts, &option_index);
            if (opt == -1)
            {
                break; 
            }
            switch (opt)
            {
                case 'c':
                {
                    unsigned int i = 0;
                    unsigned int j = 0;
                    for (i=0; i<strlen(optarg); i++)
                    {
                        if(optarg[i] == '"')
                        {
                            continue;
                        }
                        else
                        {
                            if(j == sizeof(dir))
                            {
                                exit(1);
                            }
                            dir[j] = optarg[i];
                            j++;   
                        }
                    }
                    break;
                }
                case 'h':
                default:
                    printf("rtpproxy --cwp= or \r\n rtpproxy -c  \r\n");
                    return 0;
            }
        }
        daemonize();
        {
            // Change the current working directory to the cwd, allows none root users to run this service. 
            if (chdir(dir) < 0)
            {
                exit(1);
            }
        }
        init();
    }
    tracelog("INIT", INFO_LOG, __FILE__, __LINE__, "start rtpproxy finished \n");
    while(1)
    {
        usleep(20000);
    }
    return 0;
}
