#ifndef LOG_H_EXPORT_
#define LOG_H_EXPORT_

// export to other module

#ifdef __cplusplus
extern "C" 
{
#endif

enum LogLeval
{
    ERROR_LOG  = 0,
    WARNING_LOG   ,
    INFO_LOG,
    DEBUG_LOG,
    LOG_LEVEL_SUM
};


void initLog(const char* logPath);
void deInitLog();
int regiterLog(const char* moduleName, int level);
int modifylogLevel(const char* moduleName, int level);
void deRegLog(const char* module);
void tracelog(const char* moduleName, int level, const char* file, int line, const char* fmt, ... );
char* getLogMemoryInfo();

#ifdef __cplusplus
}
#endif

#endif
