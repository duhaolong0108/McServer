short encode(int num, short *out) {
    short bytes = 0;
    while (num & -128) {
        out[bytes++] = (num & 0xFF) | 128;
        num >>= 7;
    }
    out[bytes++] = num;
    return bytes;
}