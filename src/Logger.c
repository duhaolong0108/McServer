#include <stdio.h>
#include <time.h>
#include <string.h>

char* T;

void cout(char* C ,char *F, char *T, char *D)
{
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);
    printf("%s[%02d:%02d:%02d] [%s/%s] %s\033[0m\n", C, info->tm_hour, info->tm_min, info->tm_sec, T, F, D);
}
void Logger(char *iT) { T = iT; }
void Info(char *D) { cout("\033[0m","Info", T, D); }
void Warn(char *D) { cout("\033[33m","Warn", T, D); }
void Error(char *D) { cout("\033[31m","Error", T, D); }
