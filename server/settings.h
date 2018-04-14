#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "server.h"
#include "config.h"
#include "macros.h"

class TeleCortexSettings {
public:
    TeleCortexSettings () {};
    static void reset();
    static bool save();

    #ifdef EEPROM_SETTINGS
    static bool load();
    #endif

    #if !DISABLE_M503
        static void report(const bool forReplay=false);
    #else
        FORCE_INLINE
        static void report(const bool forReplay=false) { UNUSED(forReplay); }
    #endif

    // virtual ~TeleCortexSettings ();

private:
    static void postprocess();

    #ifdef EEPROM_SETTINGS
        static bool eeprom_error;
        static void write_data(int &pos, const uint8_t *value, uint16_t size, uint16_t *crc);
        static void read_data(int &pos, uint8_t *value, uint16_t size, uint16_t *crc);
    #endif

};

extern TeleCortexSettings settings;


#ifdef EEPROM_SETTINGS
    #define EEPROM_SETTINGS_START() int eeprom_index = EEPROM_OFFSET
    #define EEPROM_SETTINGS_SKIP(VAR) eeprom_index += sizeof(VAR)
    #define EEPROM_SETTINGS_WRITE(VAR) write_data(eeprom_index, (uint8_t*)&VAR, sizeof(VAR), &working_crc)
    #define EEPROM_SETTINGS_READ(VAR) read_data(eeprom_index, (uint8_t*)&VAR, sizeof(VAR), &working_crc)
#endif

//TODO: move this to server.h
extern int controller_id;

#endif
