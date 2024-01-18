#include "in.h"

char *T;
FILE *file;

void cout(char *C, char *F, char *T, char *D)
{
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);
    char t[1024];
    snprintf(t, 1024, "[%02d:%02d:%02d] [%s/%s] %s\n", info->tm_hour, info->tm_min, info->tm_sec, T, F, D);
    printf("%s%s\033[0m", C, t);
    Puts(file, t);
}
void Logger(char *iT)
{
    T = iT;
    file = Open("./console.log", "w");
}
void Info(char *D) { cout("\033[0m", "INFO", T, D); }
void Warn(char *D) { cout("\033[33m", "WARN", T, D); }
void Error(char *D) { cout("\033[31m", "ERROR", T, D); }
