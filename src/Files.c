#include <stdio.h>
#include <string.h> // Include string.h to use strlen()
#include <stdlib.h> // Include stdlib.h to use malloc(), realloc() and free()

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

char* Read(FILE *F)
{
    fseek(F, 0, SEEK_END); 
    long int fsize = ftell(F);
    fseek(F, 0, SEEK_SET); 

    char* data = (char*)malloc(fsize + 1); 
    fread(data, 1, fsize, F);
    data[fsize] = '\0'; 

    return data;
}