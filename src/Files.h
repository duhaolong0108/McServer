#include "in.h"

FILE *Open(char *F, char *T)
{
    FILE *file = fopen(F, T);
    return file;
}

void Puts(FILE *F, char *D)
{
    fputs(D, F);
}

char *Get(FILE *F)
{
    char *a = (char *)malloc(800);
    fgets(a,100,F);
    return a;
}