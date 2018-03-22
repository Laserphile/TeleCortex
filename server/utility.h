#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <Arduino.h>
#include "types.h"

void crc16(uint16_t *crc, const void * const data, uint16_t cnt);

#endif
