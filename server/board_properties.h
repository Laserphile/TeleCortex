/**
 * Determines the SRAM available to the arduino from preprocessor defines.
 */

// Arduino AVR Boards
#if defined(__AVR_ATmega1280__)
#define DETECTED_BOARD "AVR_ATmega1280"
#define SRAM_SIZE 8192
#elif defined(__AVR_ATmega2560__)
#define DETECTED_BOARD "AVR_ATmega2560"
#define SRAM_SIZE 8192
#elif defined(__AVR_ATmega328__)
#define DETECTED_BOARD "AVR_ATmega328"
#define SRAM_SIZE 2048
#elif defined(__AVR_ATmega328P__)
#define DETECTED_BOARD "AVR_ATmega328P"
#define SRAM_SIZE 2048
#elif defined(__AVR_ATmega168__)
#define DETECTED_BOARD "AVR_ATmega168"
#define SRAM_SIZE 1024
#elif defined(__AVR_ATmega168V__)
#define DETECTED_BOARD "AVR_ATmega168V"
#define SRAM_SIZE 1024
#elif defined(__AVR_ATmega32U4__)
#define DETECTED_BOARD "AVR_ATmega32U4"
#define SRAM_SIZE 2560
#elif defined(__AVR_ATmega16U4__)
#define DETECTED_BOARD "AVR_ATmega16U4"
#define SRAM_SIZE 1280
#elif defined(__AVR_ATtiny85__)
#define DETECTED_BOARD "AVR_ATtiny85"
#define SRAM_SIZE 512
// TeensyDuino
#elif defined(__AVR_AT90USB1286__) /* Teensy++ 2.0 */
#define DETECTED_BOARD "AVR_AT90USB1286"
#define SRAM_SIZE 8192
#elif defined(__MKL26Z64__) /* Teensy LC (Cortex-M0+)*/
#define DETECTED_BOARD "MKL26Z64"
#define SRAM_SIZE 8192
#elif defined(__MK20DX256__) /* Teensy 3.1 -> 3.2 (Cortex-M4) */
#define DETECTED_BOARD "MK20DX256"
#define SRAM_SIZE 65536
#elif defined(__MK20DX128__) /* Teensy 3.0 (Cortex-M4) */
#define DETECTED_BOARD "MK20DX128"
#define SRAM_SIZE 65536
#elif defined(__MK64FX512__) /* Teensy 3.5 (Cortex-M4F) */
#define DETECTED_BOARD "MK64FX512"
#define SRAM_SIZE 196608
#elif defined(__MK66FX1M0__) /* Teensy 3.6 (Cortex-M4F) */
#define DETECTED_BOARD "MK66FX1M0"
#define SRAM_SIZE 262144
#else
// Assume we're on an Arduino Uno
#define DETECTED_BOARD "NONE"
#define SRAM_SIZE 2048
#endif
