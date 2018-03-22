#include "utility.h"


void crc16(uint16_t *crc, const void * const data, uint16_t cnt) {
    uint8_t *ptr = (uint8_t *)data;
    while (cnt--) {
        *crc = (uint16_t)(*crc ^ (uint16_t)(((uint16_t)*ptr++) << 8));
        for (uint8_t x = 0; x < 8; x++)
        *crc = (uint16_t)((*crc & 0x8000) ? ((uint16_t)(*crc << 1) ^ 0x1021) : (*crc << 1));
    }
}
