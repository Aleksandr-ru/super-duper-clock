/**
 * Library to drive Futaba VFD displays
 *
 * @author Aleksandr.ru
 * @link http://aleksandr.ru
 */

#include <Arduino.h>

#ifndef FutabaVFD_h
#define FutabaVFD_h

#ifndef VFD_SPI_DIN
#define VFD_SPI_DIN 11 // DA SDI SPI data input
#endif
#ifndef VFD_SPI_CLK
#define VFD_SPI_CLK 13 // CK CLK SPI clock
#endif
#ifndef VFD_SPI_CS
#define VFD_SPI_CS 10 // CS SPI chip select
#endif

#define VFD_DELAY_TCSCP 1
#define VFD_DELAY_TCPCS 1
#define VFD_DELAY_TDOFF 2
#define VFD_DELAY_TCSW  1

#define VFD_6  0x05
#define VFD_8  0x07
#define VFD_16 0x0f

#define VFD_MIN_BRIGHTNESS 0x01 // 0x01 min
#define VFD_MAX_BRIGHTNESS 0xf0 // 0xF0 max

class FutabaVFD {
    public:
        static const unsigned char font_bars[][5];
        static const unsigned char font_digits[][5];

        /**
         * Construct a new Futaba VFD object
         *
         * @param digits VFD_x constant
         * @param cs CS SPI chip select
        */
        FutabaVFD(uint8_t digits, uint8_t cs);

        /**
         * Construct a new Futaba VFD object
         *
         * @param digits VFD_x constant
         * @param cs CS SPI chip select
         * @param reset The reset of the RS VFD screen is active at low level. It is pulled high/no reset during normal use because the module has a built-in RC hardware reset circuit.
         * @param en EN VFD module power supply is partially enabled. EN is enabled at high level. It is recommended to send the VFD initialization command more than 100ms after setting it high to avoid issuing commands before the module power supply is stable. If this function is not used, , direct VCC short circuit/no EN is because the module EN and VCC are already internally linked.
         */
        FutabaVFD(uint8_t digits, uint8_t cs, uint8_t reset, uint8_t en);

        /**
         * @brief Show test screen
         *
         */
        void test();

        /**
         * Set brightness
         *
         * @param level between VFD_MIN_BRIGHTNESS and VFD_MAX_BRIGHTNESS
         */
        void brightness(uint8_t level);

        /**
         * Clear screen (write spaces)
         *
         */
        void clear();

        /**
         * Print a character at the specified position (user-defined, all in CG-ROM)
         *
         * @param x position
         * @param chr character encoding to be displayed
         */
        void writeOneChar(uint8_t x, unsigned char chr);

        /**
         * Print a string at the specified position
         * (Only applicable to English, punctuation, numbers)
         *
         * @param x position
         * @param str string to be displayed
         */
        void writeStr(uint8_t x, const char *str);

        /**
         * Print a string at the specified position
         * (F macro friendly)
         *
         * @param x position
         * @param str string to be displayed
         */
        void writeStr(uint8_t x, const __FlashStringHelper *s);

        /**
         * Load custom characters into RAM
         *
         * @param x position in ram (0-7)
         * @param n number of chars to load from *s
         * @param s character font to be displayed
         */
        void loadUserFont(uint8_t y, uint8_t n, const unsigned char *s);

        /**
         * Print custom characters at the specified position
         *
         * @param x position
         * @param y there is ROOM position (0-7)
         * @param s character font to be displayed
         */
        void writeUserFont(uint8_t x, uint8_t y, const unsigned char *s);

        /**
         * Translate cyrilic unicode string to VFD-compatible string
         *
         * @param str
         * @return char*
         */
        char* translate(const char *str);

    private:
        uint8_t _digits;
        uint8_t _pin_cs;

        void vfd_spi_start();
        void vfd_spi_stop();
        void vfd_spi_write(unsigned char w_data);
        void vfd_command(unsigned char c0);
        void vfd_command(unsigned char c0, unsigned char c1);
        void vfd_show();
};

#endif
