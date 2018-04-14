#ifndef __EEPROM_H__
#define __EEPROM_H__
#include "server.h"
#include <EEPROM.h>

#define EEPROM_VERSION "V01"

// Change EEPROM version if these are changed:
#define EEPROM_OFFSET 100

/**
* V01 EEPROM Layout:
*
*  100  Version                                    (char x4)
*  104  EEPROM CRC16                               (uint16_t)
*
*  106  M2205 S    Unique ID                       (uint8_t)
*  107  M2206 S    Brightness                      (uint8_t)
*/

#define EEPROM_CODE_START (EEPROM.length() / 2)
#define EEPROM_CODE_END EEPROM.length()

#if defined(ESP_PLATFORM)
#define EEPROM_OBJ_UPDATE(address, value) \
    // TODO this
#else
#define EEPROM_OBJ_UPDATE(address, value) \
    EEPROM.update(address, value)
#endif

#if defined(ESP_PLATFORM)
#define EEPROM_OBJ_WRITE(address, value) \
    // TODO this
#else
#define EEPROM_OBJ_WRITE(address, value) \
    EEPROM.write(address, value)
#endif

#if defined(ESP_PLATFORM)
#define EEPROM_OBJ_READ(address) \
    0 // TODO this
#else
#define EEPROM_OBJ_READ(address) \
    EEPROM.read(address)
#endif

/* Determine if EEPROM has been correctly initialized */
bool eeprom_magic_present();
void write_eeprom_code(const char * code, int offset);

/* Dump the EEPROM to serial */
void dump_eeprom_code();
bool eeprom_code_available();
char eeprom_code_read();

#endif /* __EEPROM_H__ */
