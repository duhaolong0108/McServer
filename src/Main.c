#include "in.h"
// https://wiki.vg/index.php?title=Protocol&oldid=18641

int main()
{
    Logger("Main");
    short a[10];
    a[0] = 128;
    a[1] = 1;
    a[2] = 114;
    a[3] = 514;

    Error(varLen(a) == 2 ? "T" : "F");
}