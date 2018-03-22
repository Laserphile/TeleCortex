/**
* settings.cpp
*
* Settings and EEPROM storage
*
* IMPORTANT:  Whenever there are changes made to the variables stored in EEPROM
* in the functions below, also increment the version number. This makes sure that
* the default values are used whenever there is a change to the data, to prevent
* wrong data being written to the variables.
*
* ALSO: Variables in the Store and Retrieve sections must be in the same order.
*       If a feature is disabled, some data must still be written that, when read,
*       either sets a Sane Default, or results in No Change to the existing value.
*
*/

#include "settings.h"

#include "server.h"
#include "eeprom.h"
#include "utility.h"
#include "serial.h"
#include "debug.h"

TeleCortexSettings settings;

//TODO: move this to server.h
int controller_id;

/**
* Post-process after Retrieve or Reset
*/
void TeleCortexSettings::postprocess() {
    // TODO: this
    // TODO: Set brightness
}

bool TeleCortexSettings::eeprom_error;
const char version[4] = EEPROM_VERSION;

void TeleCortexSettings::write_data(int &pos, const uint8_t *value, uint16_t size, uint16_t *crc) {
    if (eeprom_error) return;
    while (size--) {
        uint8_t * const p = (uint8_t * const)pos;
        uint8_t v = *value;
        // EEPROM has only ~100,000 write cycles,
        // so only write bytes that have changed!
        if (v != eeprom_read_byte(p)) {
            eeprom_write_byte(p, v);
            if (eeprom_read_byte(p) != v) {
                STRNCPY_MSG_PSTR("Error writing to EEPROM!");
                print_error(03, msg_buffer);
                eeprom_error = true;
                // TODO: break here?
                return;
            }
        }
        crc16(crc, &v, 1);
        pos++;
        value++;
    };
}

void TeleCortexSettings::read_data(int &pos, uint8_t* value, uint16_t size, uint16_t *crc) {
    if (eeprom_error) return;
    do {
        uint8_t c = eeprom_read_byte((unsigned char*)pos);
        *value = c;
        crc16(crc, &c, 1);
        pos++;
        value++;
    } while (--size);
}

/**
* M500 - Store Configuration
*/
#if EEPROM_SETTINGS
bool TeleCortexSettings::save() {
    char ver[4] = "V00";

    uint16_t working_crc = 0;

    EEPROM_START();

    eeprom_error = false;

    EEPROM_WRITE(ver);     // invalidate data first
    EEPROM_SKIP(working_crc); // Skip the checksum slot

    working_crc = 0; // clear before first "real data"

    // TODO: complete this
    EEPROM_WRITE(controller_id);

    if (!eeprom_error) {
        const int eeprom_size = eeprom_index;

        const uint16_t final_crc = working_crc;

        // Write the EEPROM header
        eeprom_index = EEPROM_OFFSET;

        EEPROM_WRITE(version);
        EEPROM_WRITE(final_crc);

        // Report storage size
        #if EEPROM_DEBUG
        SER_SNPRINT_COMMENT_PSTR(
            "Settings Stored (%d bytes; crc %d)",
             eeprom_size - (EEPROM_OFFSET), (uint32_t)final_crc
        );
        #endif
    }

    return !eeprom_error;
}
#else // !EEPROM_SETTINGS
bool TeleCortexSettings::save() {

    STRNCPY_MSG_PSTR("Writing to Dissabled EEPROM");
    print_error(03, msg_buffer);
    // TODO: break here?
    return false;
}
#endif // !EEPROM_SETTINGS


/**
* M501 - Retrieve Configuration
*/
bool TeleCortexSettings::load() {
    uint16_t working_crc = 0;

    EEPROM_START();

    char stored_ver[4];
    EEPROM_READ(stored_ver);

    uint16_t stored_crc;
    EEPROM_READ(stored_crc);


    // Version has to match or defaults are used
    if (strncmp(version, stored_ver, 3) != 0) {
        if (stored_ver[0] != 'V') {
            stored_ver[0] = '?';
            stored_ver[1] = '\0';
        }
        #if DEBUG_EEPROM
        SNPRINTF_MSG_PSTR(
            "EEPROM version mismatch! EEPROM=%s, Firmware=%s",
            stored_ver, EEPROM_VERSION
        );
        print_error(03, msg_buffer);
        // TODO: break here?
        return false;
        #endif
        reset();
    } else {
        EEPROM_READ(controller_id);

        // TODO: this
    }

    #if DEBUG_EEPROM
    report();
    #endif

    return !eeprom_error;
}

/**
* M502 - Reset Configuration
*/
void TeleCortexSettings::reset() {
    controller_id = DEFAULT_CONTROLLER_ID;
    //TODO: this

    postprocess();
}

/**
* M503 - Report current settings in RAM
*
* Unless specifically disabled, M503 is available even without EEPROM
*/
void TeleCortexSettings::report(const bool forReplay) {
    SER_SNPRINTF_COMMENT_PSTR("SET: Controller ID: %d", controller_id);

    //TODO: this
}
