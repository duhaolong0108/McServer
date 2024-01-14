#include <stdio.h>
#include <string.h> // Include string.h to use strlen()
#include "logger.c"

FILE *Open(char *F, char *T)
{
    Logger("File");
    FILE *file = fopen(F, T);

    // Check if the file was successfully opened
    if (file == NULL)
    {
        Error("Error opening file!");
    }

    return file;
}

void Write(FILE *F, unsigned char *D)
{
    fwrite(D, sizeof(unsigned char), strlen((char *)D), F);
}