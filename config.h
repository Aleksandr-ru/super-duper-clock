/**
 * Super-duper clock configuration
 *
 * @author Aleksandr.ru
 * @link http://aleksandr.ru
 */

#define MIN_BRIGHT_A 50 // analog
#define MAX_BRIGHT_A 750 // analog
#define MIN_BRIGHT_V 0x01 // vfd
#define MAX_BRIGHT_V 0xf0 // vfd
#define NIGHT (brightness < 0x02)

#define VFD_CHAR_SHIFT 0x30 // for digits
#define VFD_CHAR_POINT 0x2e // "."
#define VFD_CHAR_COLON 0x3a // ":"
#define VFD_CHAR_SPACE 0x20 // " "
#define VFD_CHAR_MINUS 0x2d // "-"

#define MAX_YEAR 2099 // rtc

//@see SCD4x Data Sheet section 3.7.1
#define SCD40_TEMP_OFFSET  7.4f // experimental value in C, default is 4
#define BMP280_TEMP_OFFSET 2.7f // experimental value in C

#define BMP280_MMHG(x) (x * 0.00750063755419211)
#define SCD40_CO2(x) constrain(x, 400, 2000)

#define PIN_BTN_L 4
#define PIN_BTN_C 5
#define PIN_BTN_R 6

#define INTERVALS 3

#define EVERY_05SEC intervals[0].active
#define EVERY_1SEC  intervals[1].active
#define EVERY_3SEC  intervals[2].active
