#include "Files.c"
#include "Socket.c"

int main(){
    FILE *a = Open("./c","w");
    Write(a,"114514");
}