char *cAnd(char *A, char *B)
{
    int length = strlen(A) + strlen(B) + 1;
    char *r = malloc(length);
    if (r)
    { // 检查malloc是否成功
        // fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcpy(r, A);
    strcat(r, B);
    return r;
}

// int 合并个鬼
// 你tcp直接两个发送不好吗？

int varcod(int num, unsigned int *out)
{
    int bytes = 0;
    while (num & 128)
    {
        out[bytes++] = (num & 0xFF) | 128;
        num >>= 7;
    }
    out[bytes++] = num;
    return bytes;
}

// 获取varint编码的长度
int varLen(int *data)
{
    int i = 0;
    while (data[i++] >> 7);
    return i;
}


void encode(int32_t num, uint8_t *out, size_t *offset) {
    const uint8_t MSB = 0x80;
    const uint8_t REST = 0x7F;
    const uint8_t MSBALL = ~REST;
    const int32_t INT = pow(2, 31);

    if (num > INT) {
        printf("Could not encode varint\n");
        exit(1);
    }
    size_t old_offset = *offset;

    while (num >= INT) {
        out[*offset++] = (num & 0xFF) | MSB;
        num /= 128;
    }
    while (num & MSBALL) {
        out[*offset++] = (num & 0xFF) | MSB;
        num >>= 7;
    }
    out[*offset++] = num | 0;

    *offset -= old_offset;
}

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