/**
 * Library to drive Futaba VFD displays
 * 
 * @author Aleksandr.ru
 * @link http://aleksandr.ru
 * 
 * Based on code from:
 * https://drive.google.com/drive/folders/1ZXyel2LAcHdFSkiIZ5ASejlG-4toeGsX
 * https://github.com/davll/futabavfd8
 * https://github.com/3KUdelta/Futaba-VFD-16bit_ESP32
 */

#include "FutabaVFD.h"

// The font file model should start from the lower left corner,
// first from bottom to top, and then from left to right
const unsigned char FutabaVFD::font_bars[][5] = {
  0x00,0x00,0x00,0x00,0x00, // 0_
  0x40,0x40,0x40,0x40,0x40, // 1_
  0x60,0x60,0x60,0x60,0x60, // 2_
  0x70,0x70,0x70,0x70,0x70, // 3_
  0x78,0x78,0x78,0x78,0x78, // 4_
  0x7C,0x7C,0x7C,0x7C,0x7C, // 5_
  0x7E,0x7E,0x7E,0x7E,0x7E, // 6_
  0x7F,0x7F,0x7F,0x7F,0x7F  // 7_
  };

const unsigned char FutabaVFD::font_digits[][5] = {
  0x7F,0x7F,0x41,0x7F,0x7F, // 0
  0x00,0x06,0x7F,0x7F,0x00, // 1
  0x79,0x79,0x49,0x4F,0x4F, // 2
  0x49,0x49,0x49,0x7F,0x7F, // 3
  0x0F,0x0F,0x08,0x7F,0x7F, // 4
  0x4F,0x4F,0x49,0x79,0x79, // 5
  0x7F,0x7F,0x49,0x79,0x78, // 6
  0x01,0x01,0x71,0x7F,0x0F, // 7
  0x7F,0x7F,0x49,0x7F,0x7F, // 8
  0x0F,0x4F,0x49,0x7F,0x7F, // 9
  };

FutabaVFD::FutabaVFD(uint8_t digits, uint8_t cs)
: _digits(digits), _pin_cs(cs)
{
  pinMode(_pin_cs, OUTPUT);
  digitalWrite(_pin_cs, HIGH);

  pinMode(VFD_SPI_CLK, OUTPUT);
  pinMode(VFD_SPI_DIN, OUTPUT);

  //first byte address for setting DIGITS
  vfd_command(0xe0, _digits);
}

FutabaVFD::FutabaVFD(uint8_t digits, uint8_t cs, uint8_t reset, uint8_t en)
{
  pinMode(en, OUTPUT);
  pinMode(reset, OUTPUT);

  digitalWrite(en, HIGH);
  delayMicroseconds(100);
  digitalWrite(reset, LOW);
  delayMicroseconds(5);
  digitalWrite(reset, HIGH);

  FutabaVFD(digits, cs);
}

void FutabaVFD::vfd_spi_start()
{
    digitalWrite(_pin_cs, LOW);
    delayMicroseconds(VFD_DELAY_TCSCP);
}

void FutabaVFD::vfd_spi_stop()
{
    digitalWrite(_pin_cs, HIGH);
    delayMicroseconds(VFD_DELAY_TCSW);
}

void FutabaVFD::vfd_spi_write(unsigned char data)
{
  for (uint8_t i = 0; i < 8; i++) {
    if (data & 0x01) digitalWrite(VFD_SPI_DIN, HIGH);
    else             digitalWrite(VFD_SPI_DIN, LOW);

    digitalWrite(VFD_SPI_CLK, LOW);
    // If you use a faster arduino device such as ESP32, please add some delay.
    // The VFD SPI clock frequency is as described in the manual 0.5MHZ.
    digitalWrite(VFD_SPI_CLK, HIGH);
    data >>= 1;
  }
}

void FutabaVFD::vfd_command(unsigned char c0)
{
    vfd_spi_start();
    vfd_spi_write(c0);
    delayMicroseconds(VFD_DELAY_TCPCS);
    vfd_spi_stop();
}

void FutabaVFD::vfd_command(unsigned char c0, unsigned char c1)
{
    vfd_spi_start();
    vfd_spi_write(c0);
    delayMicroseconds(VFD_DELAY_TDOFF);
    vfd_spi_write(c1);
    delayMicroseconds(VFD_DELAY_TCPCS);
    vfd_spi_stop();
}

void FutabaVFD::vfd_show()
{
    //Open display command
    vfd_command(0xe8);
}

void FutabaVFD::test()
{
    //full brightness test screen
    vfd_command(0xe9);
}

void FutabaVFD::brightness(uint8_t level)
{
    // first byte address for setting BRIGHTNESS
    vfd_command(0xe4, level);
}

void FutabaVFD::clear()
{
    vfd_spi_start();
    vfd_spi_write(0x20);
    for(uint8_t i = 0; i <= _digits; i++) {
        delayMicroseconds(VFD_DELAY_TDOFF);
        vfd_spi_write(0x20);
    }
    delayMicroseconds(VFD_DELAY_TCPCS);
    vfd_spi_stop();

    vfd_show();
}

void FutabaVFD::writeOneChar(uint8_t x, unsigned char chr)
{
    if (x > _digits) return;

    vfd_command(0x20 + x, 0x00 + chr);
    vfd_show();
}

void FutabaVFD::writeStr(uint8_t x, const char *str)
{
    if (x > _digits) return;

    vfd_spi_start();
    vfd_spi_write(0x20 + x); //Start position of address register
    while(*str && x++ <= _digits) {
        delayMicroseconds(VFD_DELAY_TDOFF);
        vfd_spi_write(*str); //ascii and corresponding character table conversion
        str++;
    }
    delayMicroseconds(VFD_DELAY_TCPCS);
    vfd_spi_stop();

    vfd_show();
}

void FutabaVFD::writeStr(uint8_t x, const __FlashStringHelper *s)
{
  const char buf[16] = { 0 };
  strncpy_P(buf, (const char*)s, 16);
  writeStr(x, buf);
}

void FutabaVFD::loadUserFont(uint8_t y, uint8_t n, const unsigned char *s)
{
    if (y > 7 || n < 1) return;

    for (uint8_t j = 0; (j < n) && (y + j < 8); j++) {
        vfd_spi_start();
        vfd_spi_write(0x40 + y + j); //Start position of address register
        for (uint8_t i = 0; i < 7; i++) {
            delayMicroseconds(VFD_DELAY_TDOFF);
            vfd_spi_write(s[i]);
        }
        delayMicroseconds(VFD_DELAY_TCPCS);
        vfd_spi_stop();
    }
}

void FutabaVFD::writeUserFont(uint8_t x, uint8_t y, unsigned const char *s)
{
    if (x > _digits || y > 7) return;

    loadUserFont(y, 1, s);
    writeOneChar(x, y);
}

char* FutabaVFD::translate(const char *str)
{
    char* buf = new char[17] { 0 };
    uint8_t b = 0;

    // А = 0xd0 0x90
    // Я = 0xd0 0xaf
    // а = 0xd0 0xb0
    // п = 0xd0 0xbf
    // р = 0xd1 0x80
    // я = 0xd1 0x8f

    // Ё = 0xd0 0x81
    // ё = 0xd1 0x91

    static const char d0[] = {                   // (0x90 ... 0xbf)
      0x41, 0x08, 0x42, 0x09, 0x0a, // А Б В Г Д
      0x45, 0x0b, 0x0c, 0x0d, 0x0e, // Е Ж З И Й
      0x4b, 0x0f, 0x4d, 0x48, 0x4f, // К Л М Н О
      0x10, 0x50, 0x43, 0x54, 0x11, // П Р С Т У
      0x12, 0x58, 0x13, 0x14, 0x15, // Ф Х Ц Ч Ш
      0x16, 0x17, 0x18, 0x19, 0x1a, // Щ Ъ Ы Ь Э
      0x1b, 0x1c, 0x61, 0x1d, 0x1e, // Ю Я а б в
      0x1f, 0x80, 0x65, 0x81, 0x82, // г д е ж з
      0x83, 0x84, 0x85, 0x86, 0x87, // и й к л м
      0x88, 0x6f, 0x89              // н о п
    };
    static const char d1[] = {                   // (0x80 ... 0x8f)
      0x70, 0x8a, 0x8b, 0x79, 0x8d, // р с т у ф
      0x78, 0x8e, 0x8f, 0x90, 0x91, // х ц ч ш щ
      0x92, 0x93, 0x94, 0x95, 0x96, // ъ ы ь э ю
      0x97                          // я
    };

    while(*str && b++ <= 16) {
        if (str[0] == 0xFFFFFFd0 && str[1] >= 0xFFFFFF90 && str[1] <= 0xFFFFFFbf) {
          buf[b-1] = d0[ str[1] - 0xFFFFFF90 ];
          str+=2;
        }
        else if (str[0] == 0xFFFFFFd1 && str[1] >= 0xFFFFFF80 && str[1] <= 0xFFFFFF8f) {
          buf[b-1] = d1[ str[1] - 0xFFFFFF80 ];
          str+=2;
        }
        else if (str[0] == 0xFFFFFFd0 && str[1] == 0xFFFFFF81) {
          buf[b-1] = 0xcb; // Ё
          str+=2;
        }
        else if (str[0] == 0xFFFFFFd1 && str[1] == 0xFFFFFF91) {
          buf[b-1] = 0xeb; // ё
          str+=2;
        }
        else {
          buf[b-1] = str[0];
          str++;
        }
    }
    return buf;
}
