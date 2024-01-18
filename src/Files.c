#include "in.h"

FILE *Open(char *F, char *T)
{
    FILE *file = fopen(F, T);
    return file;
}

void Write(FILE *F, unsigned char *D)
{
    fwrite(D, sizeof(unsigned char), strlen((char *)D), F);
}

void Puts(FILE *F, unsigned char *D)
{
    fputs(D, F);
}

char *Read(FILE *F)
{
    fseek(F, 0, SEEK_END);
    long int fsize = ftell(F);
    fseek(F, 0, SEEK_SET);

    char *data = (char *)malloc(fsize + 1);
    fread(data, 1, fsize, F);
    data[fsize] = '\0';

    return data;
}
