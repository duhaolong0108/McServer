#include <stdint.h>

short encode(int num, unsigned short *out) {
    short bytes = 0;
    while (num & 128) {
        out[bytes++] = (num & 0xFF) | 128;
        num >>= 7;
    }
    out[bytes++] = num;
    return bytes;
}

int main(){
    short a[10];
    encode(128,a);
    printf(a[0] == "\x80" ? "T|||":"F|||"); // 检查第一个字节是否为0x80
    printf(a[1] == 0x1 ? "T|||":"F|||");   // 检查第二个字节是否为0x1
}