#include "eeprom.h"
#include "serial.h"
#include "config.h"
#include "macros.h"

char eeprom_magic[] = ";EEPROM";
int eeprom_magic_len = sizeof(eeprom_magic) / sizeof(eeprom_magic[0]);
char * eeprom_header_buffer = (char *) malloc(eeprom_magic_len);
int eeprom_code_index = 0;

bool eeprom_magic_present() {
    const char* debug_prefix = "EMP";

    // fill eeprom_header_buffer
    for( int offset = 0; offset < eeprom_magic_len; offset++){
        eeprom_header_buffer[offset] = EEPROM.read(EEPROM_CODE_START + offset);
    }
    // eeprom_header_buffer[eeprom_magic_len] = '\0';
    bool response = (strstr(eeprom_header_buffer, eeprom_magic) != NULL);
    // bool response = strstr(eeprom_header_buffer, eeprom_magic);
    #if DEBUG_EEPROM
        SER_SNPRINTF_COMMENT_PSTR("%s: -> eeprom_header_buffer: '%s'", debug_prefix, eeprom_header_buffer);
        SER_SNPRINTF_COMMENT_PSTR("%s: -> eeprom_magic        : '%s'", debug_prefix, eeprom_magic);
        SER_SNPRINTF_COMMENT_PSTR("%s: -> response            : %d", debug_prefix, response);
    #endif
    return response;
}

void write_eeprom_code(const char * code, int offset = 0){
    char * code_write = (char *)code;
    int write_address = EEPROM_CODE_START + offset;
    while( (write_address < EEPROM_CODE_END) && (*code_write != '\0') ){
        EEPROM.update(write_address, *code_write);
        code_write++;
        write_address++;
    }
}

void dump_eeprom_code() {
    int chunk_size = 16;

    for(int chunk_start = EEPROM_CODE_START; chunk_start < (EEPROM_CODE_END - chunk_size); chunk_start += chunk_size){
        STRNCPY_PSTR(fmt_buffer, "%c %04x :", BUFFLEN_FMT);
        snprintf(
            msg_buffer, BUFFLEN_FMT, fmt_buffer,
            COMMENT_PREFIX, chunk_start
        );
        SERIAL_OBJ.print(msg_buffer);
        STRNCPY_PSTR(fmt_buffer, " %02x", BUFFLEN_FMT);
        for(int chunk_offset=0; chunk_offset<chunk_size; chunk_offset++){
            snprintf(
                msg_buffer, BUFFLEN_MSG, fmt_buffer,
                EEPROM.read(chunk_start + chunk_offset)
            );
            SERIAL_OBJ.print(msg_buffer);
        }
        STRNCPY_PSTR(msg_buffer, " | ", BUFFLEN_MSG);
        SERIAL_OBJ.print(msg_buffer);
        STRNCPY_PSTR(fmt_buffer, "%c", BUFFLEN_FMT);
        for(int chunk_offset=0; chunk_offset<chunk_size; chunk_offset++){
            char out = (char)(EEPROM.read(chunk_start + chunk_offset));
            if(!IS_SINGLE_PRINTABLE(out)){
                out = '\xff'; // appears as "⸮" in serial monitor
            }
            snprintf(
                msg_buffer, BUFFLEN_MSG, fmt_buffer,
                out
            );
            SERIAL_OBJ.print(msg_buffer);
        }
        SERIAL_OBJ.println();
    }
}

bool eeprom_code_available() {
    return (eeprom_magic_present() && (EEPROM_CODE_START + eeprom_code_index < EEPROM_CODE_END));

}

char eeprom_code_read() {
    if(EEPROM_CODE_START + eeprom_code_index < EEPROM_CODE_END){
        return EEPROM.read(EEPROM_CODE_START + eeprom_code_index++);
    }
    return '\0';
}
