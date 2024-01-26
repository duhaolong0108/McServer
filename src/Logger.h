#ifndef Log
#define Log

#include "in.h"

void cout(char *C, char *F, char *T, char *D)
{
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);
    printf("%s[%02d:%02d:%02d] [%s/%s] %s\n\033[0m", C, info->tm_hour, info->tm_min, info->tm_sec, T, F, D);
}

#define Info(T, D) cout("\033[0m", "INFO", T, D)
#define Warn(T, D) cout("\033[33m", "WARN", T, D)
#define Error(T, D) cout("\033[31m", "ERROR", T, D)

#endif