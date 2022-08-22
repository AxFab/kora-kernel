#include <stdint.h>

void bitsclr(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] &= ~(1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = 0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] &= ~(1 << j);
        j++;
        count--;
    }
}

void bitsset(uint8_t *ptr, int start, int count)
{
    int i = start / 8;
    int j = start % 8;
    while (count > 0 && j != 0) {
        ptr[i] |= (1 << j);
        j = (j + 1) % 8;
        count--;
    }
    i = ALIGN_UP(start, 8) / 8;
    while (count > 8) {
        ptr[i] = ~0;
        i++;
        count -= 8;
    }
    j = 0;
    while (count > 0) {
        ptr[i] |= (1 << j);
        j++;
        count--;
    }
}

int bitschrz(uint8_t *ptr, int len)
{
    int i = 0, j = 0;
    while (i < len / 8 && ptr[i] == 0xFF)
        i++;
    if (i >= len / 8)
        return -1;
    uint8_t by = ptr[i];
    while (by & 1) {
        j++;
        by = by >> 1;
    }
    return i * 8 + j;
}
