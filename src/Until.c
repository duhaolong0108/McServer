char *cAnd(char *A, char *B)
{
    short length = strlen(A) + strlen(B) + 1;
    char *r = malloc(length);
    if (r == NULL)
    { // 检查malloc是否成功
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcpy(r, A);
    strcat(r, B);
    return r;
}

// Short 合并个鬼
// 你tcp直接两个发送不好吗？

short varcod(int num, unsigned short *out)
{
    short bytes = 0;
    while (num & 128)
    {
        out[bytes++] = (num & 0xFF) | 128;
        num >>= 7;
    }
    out[bytes++] = num;
    return bytes;
}

// 获取varint编码的长度
short varLen(short *data)
{
    short i = 0;
    while (data[i++] >> 7)
        ;
    return i;
}