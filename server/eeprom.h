#ifndef __EEPROM_H__
#define __EEPROM_H__
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_CODE_START (EEPROM.length() / 2)
#define EEPROM_CODE_END EEPROM.length()

/* Determine if EEPROM has been correctly initialized */
bool eeprom_magic_present();

void write_eeprom_code(char * code, int offset = 0);

/* Dump the EEPROM to serial */
void dump_eeprom_code();

bool eeprom_code_available();

char eeprom_code_read();

#endif /* __EEPROM_H__ */
