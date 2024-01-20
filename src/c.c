#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

int read_varint(const int *buf, int len) {
    int result = 0;
    int offset = 0;
    int shift = 0;
    // Len 使用 Getlen
    bool continue_reading = true;

    while (continue_reading) {
        if (offset >= len) {
            printf("Error: could not decode varint\n");
            return 0;
        }

        int b = buf[offset++];
        result += (b & 127) << shift;
        shift += 7;

        continue_reading = (b & 128) != 0;
    }
    return result;
}

int main() {
    int data[2] = {128,0x00};
    int len = 2;
    int result = read_varint(data, len);
    printf("%d",result);
    return 0;
}