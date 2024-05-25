/**
 * Super-duper clock
 *
 * @author Aleksandr.ru
 * @link http://aleksandr.ru
 */

#include "config.h"
#include "FutabaVFD.h"
#include <RTClib.h> // https://github.com/adafruit/RTClib
#include <PinButton.h> // https://github.com/poelstra/arduino-multi-button/
#include <Wire.h>
#include <SensirionI2CScd4x.h> // https://github.com/Sensirion/arduino-i2c-scd4x
// #include <Adafruit_AHTX0.h> // https://github.com/adafruit/Adafruit_AHTX0
#include <Adafruit_BMP280.h> // https://github.com/adafruit/Adafruit_BMP280_Library

FutabaVFD vfd(VFD_16, VFD_SPI_CS);

RTC_DS1307 rtc;
// Adafruit_AHTX0 aht20;
Adafruit_BMP280 bmp280;
SensirionI2CScd4x scd40;

PinButton btnLeft(PIN_BTN_L);
PinButton btnCenter(PIN_BTN_C);
PinButton btnRight(PIN_BTN_R);

uint8_t brightness = VFD_MAX_BRIGHTNESS / 2;

struct MEASUREMENT {
  float value;
  unsigned long time;
  bool error;
};

MEASUREMENT measurements[4] = {
  {0.0f, 0, true}, // temperatue C
  {0.0f, 0, true}, // humidity %
  {0.0f, 0, true}, // pressure Pa
  {0.0f, 0, true}  // co2 ppm
};

enum MEASUREMENT_IDS {
  TEMP,
  HUMI,
  PRES,
  CO2
};

//enum SENSORS {
////  AHT20,
//  BMP280,
//  SCD40
//};
//
//uint8_t phase = BMP280;

enum MODES {
  CLOCK,
  SET_HOUR,
  SET_MINUTE,
  SET_SECOND,
  SET_DAY,
  SET_MONTH,
  SET_YEAR
};

enum SCENES {
  TODAY,
  TEMPHUM,
  PRESSURE,
  AIR
};

uint8_t mode = CLOCK;
uint8_t scene = TODAY;
bool redraw = true;

const String day_names[12] = {
  vfd.translate("Вс"),
  vfd.translate("Пн"),
  vfd.translate("Вт"),
  vfd.translate("Ср"),
  vfd.translate("Чт"),
  vfd.translate("Пт"),
  vfd.translate("Сб")
};

const String month_names[12] = {
  vfd.translate("Янв"),
  vfd.translate("Фев"),
  vfd.translate("Мар"),
  vfd.translate("Апр"),
  vfd.translate("Мая"),
  vfd.translate("Июн"),
  vfd.translate("Июл"),
  vfd.translate("Авг"),
  vfd.translate("Сен"),
  vfd.translate("Окт"),
  vfd.translate("Ноя"),
  vfd.translate("Дек")
};

const String dimensions[4] = {
  "\xb0", "%",
  vfd.translate("мм.Рс"), // mmHg
  "ppm" // co2
};

enum SETIDS {
  TIMEID,
  DATEID
};

const String set_names[3] = {
  vfd.translate("Время"),
  vfd.translate("Дата")
};

struct INTERVAL {
  unsigned int step;
  unsigned long last;
  bool active;
};

INTERVAL intervals[INTERVALS] = {
  { 500, 0, false}, // blink
  {1000, 0, false}, // clock update
  {3000, 0, false}  // clock scene
};

void setup()
{
  Wire.begin();

  vfd.brightness(VFD_MAX_BRIGHTNESS);
  vfd.test();
  delay(500);

  scd40.begin(Wire);
  scd40.startPeriodicMeasurement();

//  while (!aht20.begin()) {
//    vfd.writeStr(0, F("AHT20 failed!"));
//    delay(500);
//  }

  while (!bmp280.begin()) {
    vfd.writeStr(0, F("BMP280 failed!"));
    delay(500);
  }
  bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                     Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                     Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                     Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                     Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  while (!rtc.begin()) {
    vfd.writeStr(0, F("DS1307 failed!"));
    delay(500);
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  vfd.writeStr(0, F("  aleksandr.ru  "));
  vfd.brightness(brightness);
  delay(1500);

  vfd.clear();

  mode = CLOCK;
  scene = TODAY;
}

void loop()
{
  btnLeft.update();
  btnCenter.update();
  btnRight.update();
  
  for (uint8_t i = 0; i < INTERVALS; i++) {
    if(millis() - intervals[i].last > intervals[i].step) {
      intervals[i].last = millis();
      intervals[i].active = true;
    }
  }

  if (btnCenter.isClick()) {
    vfd.clear();
    redraw = true;
    
    if (mode < SET_YEAR) mode++;
    else mode = CLOCK;
  }

  switch(mode) {
    case SET_HOUR:
    case SET_MINUTE:
    case SET_SECOND:
      loop_set_time();
      break;
    case SET_DAY:
    case SET_MONTH:
    case SET_YEAR:
      loop_set_date();
      break;
    case CLOCK:
    default:
      loop_clock();
  }

  if (EVERY_05SEC) {
    autoBrightness();
  }

  if (mode == CLOCK) loop_measure();
 
  for (uint8_t i = 0; i < INTERVALS; i++) intervals[i].active = false;
}

void autoBrightness()
{
  // 5506 resistor + 1k resistor
  int a = analogRead(A0);
  a = constrain(a, MIN_BRIGHT_A, MAX_BRIGHT_A);
  byte b = map(a, MIN_BRIGHT_A, MAX_BRIGHT_A, MIN_BRIGHT_V, MAX_BRIGHT_V);

  if (b > brightness) brightness ++;
  else if (b < brightness) brightness --;
  else return;

  vfd.brightness(brightness);
}

void loop_measure()
{
  if (EVERY_1SEC) {
    MEASUREMENT& p = measurements[PRES];
    MEASUREMENT& c = measurements[CO2];
    if (millis() - p.time > 3000) {
      read_bmp280();
    }
    if (millis() - c.time > 5000) {
      correct_scd40();
      read_scd40();
      if (c.error && (millis() - c.time > 60000)) {
        scd40.stopPeriodicMeasurement();
        scd40.performFactoryReset();  
        scd40.startPeriodicMeasurement();
      }
    }
  }
//  if (EVERY_1SEC) {
//    if (phase == BMP280) {
//      MEASUREMENT& m = measurements[PRES];
//      read_bmp280();
//      if (!m.error || (millis() - m.time > 5000)) {
//        scd40.startPeriodicMeasurement();
//        correct_scd40();
//        phase = SCD40;
//      }
//    }
//    else if (phase == SCD40) {
//      MEASUREMENT& m = measurements[CO2];
//      if ((millis() - m.time > 5000) && (millis() - m.time < 30000)) {
//        read_scd40();
//        if (!m.error) {
//          scd40.stopPeriodicMeasurement();
//          phase = BMP280;
//        }
//      }
//      else if (millis() - m.time > 30000) {
//        scd40.stopPeriodicMeasurement();
//        scd40.performFactoryReset();
//        phase = BMP280;
//      }
//    }
//  }
}

void loop_clock()
{
  if (btnRight.isClick() || (!NIGHT && EVERY_3SEC)) {
    vfd.clear();
    redraw = true;

    if (scene < AIR) scene++;
    else scene = TODAY;
  }
  else if (btnLeft.isClick()) {
    vfd.clear();
    redraw = true;
    
    if (scene > TODAY) scene--;
    else scene = AIR;
  }

  if (redraw || EVERY_1SEC) {
    DateTime now = rtc.now();
    draw_clock(now);

    switch(scene) {
      case TEMPHUM:
        draw_temp_hum();
        break;
      case PRESSURE:
        draw_pressure();
        break;
      case AIR:
        draw_air_quality();
        break;
      case TODAY:
      default:
        draw_today(now);
    }
    redraw = false;
  }
}

void draw_clock(DateTime now)
{
  if (!now.isValid()) return; 
  
  const uint8_t start = 0;
  bool flag = millis() / 1000 % 2 == 0;

  if (redraw || now.second() == 0) {
    vfd.writeUserFont(start + 0, 0, vfd.font_digits[ now.hour() / 10 ]);
    vfd.writeUserFont(start + 1, 1, vfd.font_digits[ now.hour() % 10 ]);
    vfd.writeUserFont(start + 3, 2, vfd.font_digits[ now.minute() / 10 ]);
    vfd.writeUserFont(start + 4, 3, vfd.font_digits[ now.minute() % 10 ]);
  }

  if (flag) vfd.writeOneChar(start + 2, VFD_CHAR_COLON);
  else vfd.writeOneChar(start + 2, VFD_CHAR_SPACE);
}

void draw_today(DateTime now)
{
  if (!now.isValid()) return; 
  
  const uint8_t start = 7;

  if (redraw || now.second() == 0) {

    vfd.writeStr(start + 0, day_names[ now.dayOfTheWeek() ].c_str());

    vfd.writeOneChar(start + 3, VFD_CHAR_SHIFT + now.day() / 10);
    vfd.writeOneChar(start + 4, VFD_CHAR_SHIFT + now.day() % 10);

    vfd.writeStr(start + 6, month_names[ now.month() - 1 ].c_str());
  }
}

void draw_temp_hum()
{
  MEASUREMENT& tm = measurements[TEMP];
  MEASUREMENT& hm = measurements[HUMI];
  const uint8_t start = 6;

  if (tm.error && (millis() - tm.time > 60000)) {
    vfd.writeStr(start + 1, F("--"));
  }
  else {
    const uint16_t t = round(abs(tm.value * 10));
    
    if (tm.value < 0) {
      vfd.writeOneChar(start + 0, VFD_CHAR_MINUS);
    }
    else {
      vfd.writeOneChar(start + 0, VFD_CHAR_SPACE);
    }
  
    vfd.writeOneChar(start + 1, VFD_CHAR_SHIFT + t / 100);
    vfd.writeOneChar(start + 2, VFD_CHAR_SHIFT + t / 10 % 10);
    vfd.writeOneChar(start + 3, VFD_CHAR_POINT);
    vfd.writeOneChar(start + 4, VFD_CHAR_SHIFT + t % 10);
  }

  if (hm.error && (millis() - hm.time > 60000)) {
    vfd.writeStr(start + 7, F("--"));
  }
  else {
    const uint8_t h = round(hm.value);

    vfd.writeOneChar(start + 7, VFD_CHAR_SHIFT + h / 10);
    vfd.writeOneChar(start + 8, VFD_CHAR_SHIFT + h % 10);
  }

  if (redraw) {
    vfd.writeStr(start + 5, dimensions[ TEMPHUM-1 ].c_str());
    vfd.writeStr(start + 9, dimensions[ TEMPHUM ].c_str());
  }
}

void draw_pressure()
{
  MEASUREMENT& m = measurements[PRESSURE];
  const uint8_t start = 7;

  if (m.error && (millis() - m.time > 60000)) {
    vfd.writeStr(start + 1, F("--"));
  }
  else {
    const String str = String(round(BMP280_MMHG(m.value)));
    vfd.writeStr(start + 0, str.c_str());
  }
  
  if (redraw) {
    vfd.writeStr(start + 4, dimensions[ PRESSURE ].c_str());
  }
}

void draw_air_quality()
{
  MEASUREMENT& m = measurements[CO2];
  const uint8_t start = 7;

  if (m.error && (millis() - m.time > 60000)) {
    vfd.writeStr(start + 1, F("--"));
  }
  else {
    uint16_t co2 = round(m.value);
    const String str = String(co2);
    vfd.writeStr(start + (co2 < 1000 ? 1 : 0), str.c_str());
  }
  
  if (redraw) {
    vfd.writeStr(start + 5, dimensions[ AIR ].c_str());
  }
}

void loop_set_time()
{
  DateTime now = rtc.now();
  uint8_t nn = 0;

  if (btnRight.isClick()) {
    if (mode == SET_HOUR) {
      if (now.hour() < 23) nn = now.hour() + 1;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), nn, now.minute(), now.second()));
    }
    else if (mode == SET_MINUTE) {
      if (now.minute() < 59) nn = now.minute() + 1;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), nn, now.second()));
    }
    else if (mode == SET_SECOND) {
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), nn));
    }
    redraw = true;
  }
  else if (btnLeft.isClick()) {
    if (mode == SET_HOUR) {
      if (now.hour() < 1) nn = 23;
      else nn = now.hour() - 1;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), nn, now.minute(), now.second()));
    }
    else if (mode == SET_MINUTE) {
      if (now.minute() < 1) nn = 59;
      else nn = now.minute() - 1;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), nn, now.second()));
    }
    else if (mode == SET_SECOND) {
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), nn));
    }
    redraw = true;
  }

  const uint8_t start = 7;
  bool flag = millis() / 500 % 2 == 0;

  if (redraw || EVERY_05SEC) {
    if (redraw) {
      vfd.writeStr(0, set_names[TIMEID].c_str());
    }

    if (mode == SET_HOUR && flag) {
      vfd.writeOneChar(start + 0, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 1, VFD_CHAR_SPACE);
    }
    else {
      vfd.writeOneChar(start + 0, VFD_CHAR_SHIFT + now.hour() / 10);
      vfd.writeOneChar(start + 1, VFD_CHAR_SHIFT + now.hour() % 10);
    }
    vfd.writeOneChar(start + 2, VFD_CHAR_COLON);

    if (mode == SET_MINUTE && flag) {
      vfd.writeOneChar(start + 3, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 4, VFD_CHAR_SPACE);
    }
    else {
      vfd.writeOneChar(start + 3, VFD_CHAR_SHIFT + now.minute() / 10);
      vfd.writeOneChar(start + 4, VFD_CHAR_SHIFT + now.minute() % 10);
    }
    vfd.writeOneChar(start + 5, VFD_CHAR_COLON);

    if (mode == SET_SECOND && flag) {
      vfd.writeOneChar(start + 6, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 7, VFD_CHAR_SPACE);
    }
    else {
      vfd.writeOneChar(start + 6, VFD_CHAR_SHIFT + now.second() / 10);
      vfd.writeOneChar(start + 7, VFD_CHAR_SHIFT + now.second() % 10);
    }

    redraw = false;
  }
}

void loop_set_date()
{
  DateTime now = rtc.now();
  uint16_t nn = 1;
  
  const uint16_t min_year = String(__DATE__).substring(7).toInt();

  if (btnRight.isClick()) {
    if (mode == SET_DAY) {
      if (now.day() < 31) nn = now.day() + 1;
      rtc.adjust(DateTime(now.year(), now.month(), nn, now.hour(), now.minute(), now.second()));
    }
    else if (mode == SET_MONTH) {
      if (now.month() < 12) nn = now.month() + 1;
      rtc.adjust(DateTime(now.year(), nn, now.day(), now.hour(), now.minute(), now.second()));
    }
    else if (mode == SET_YEAR) {
      if (now.year() < MAX_YEAR) nn = now.year() + 1;
      else nn = min_year;
      rtc.adjust(DateTime(nn, now.month(), now.day(), now.hour(), now.minute(), now.second()));
    }
    redraw = true;
  }
  else if (btnLeft.isClick()) {
    if (mode == SET_DAY) {
      nn = now.day() - 1;
      if (nn < 1) nn = 31;
      rtc.adjust(DateTime(now.year(), now.month(), nn, now.hour(), now.minute(), now.second()));
    }
    else if (mode == SET_MONTH) {
      nn = now.month() - 1;
      if (nn < 0) nn = 12;
      rtc.adjust(DateTime(now.year(), nn, now.day(), now.hour(), now.minute(), now.second()));
    }
    else if (mode == SET_YEAR) {
      nn = now.year() - 1;
      if (nn < min_year) nn = MAX_YEAR;
      rtc.adjust(DateTime(nn, now.month(), now.day(), now.hour(), now.minute(), now.second()));
    }
    redraw = true;
  }

  const uint8_t start = 6;
  bool flag = millis() / 500 % 2 == 0;

  if (redraw || EVERY_05SEC) {
    if (redraw) {
      vfd.writeStr(0, set_names[DATEID].c_str());
    }

    if (mode == SET_DAY && flag) {
      vfd.writeOneChar(start + 0, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 1, VFD_CHAR_SPACE);
    }
    else {
      vfd.writeOneChar(start + 0, VFD_CHAR_SHIFT + now.day() / 10);
      vfd.writeOneChar(start + 1, VFD_CHAR_SHIFT + now.day() % 10);
    }
    vfd.writeOneChar(start + 2, VFD_CHAR_POINT);

    if (mode == SET_MONTH && flag) {
      vfd.writeOneChar(start + 3, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 4, VFD_CHAR_SPACE);
    }
    else {
      vfd.writeOneChar(start + 3, VFD_CHAR_SHIFT + now.month() / 10);
      vfd.writeOneChar(start + 4, VFD_CHAR_SHIFT + now.month() % 10);
    }
    vfd.writeOneChar(start + 5, VFD_CHAR_POINT);

    if (mode == SET_YEAR && flag) {
      vfd.writeOneChar(start + 6, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 7, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 8, VFD_CHAR_SPACE);
      vfd.writeOneChar(start + 9, VFD_CHAR_SPACE);
    }
    else {
      String str = String(now.year());
      vfd.writeStr(start + 6, str.c_str());
    }

    redraw = false;
  }
}

//void read_aht20()
//{
//  sensors_event_t humidity, temp;
//  aht20.getEvent(&humidity, &temp);
//  last_temperature = temp.temperature;
//  last_humidity = humidity.relative_humidity;
//}

void read_bmp280()
{
  MEASUREMENT& t = measurements[TEMP];
  MEASUREMENT& p = measurements[PRES];
  
  if (bmp280.takeForcedMeasurement()) {
    measurement_value(t, bmp280.readTemperature());
    measurement_value(p, bmp280.readPressure());
  }
  else {
    measurement_error(t);
    measurement_error(p);
  }
}

void read_scd40()
{
  MEASUREMENT& h = measurements[HUMI];
  MEASUREMENT& c = measurements[CO2];
  
  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool is_ready = false;
  
  scd40.getDataReadyFlag(is_ready);
  
  if (is_ready) scd40.readMeasurement(co2, temperature, humidity);
  
  if (is_ready && co2) {
    measurement_value(h, humidity);
    measurement_value(c, (float)co2);
  }
  else {
    measurement_error(h);
    measurement_error(c);
  }
}

void correct_scd40()
{
  MEASUREMENT p = measurements[PRESSURE];
  
  // @param ambientPressure Ambient pressure in hPa. 
  // Convert value to Pa by: value * 100.
  // uint16_t setAmbientPressure(uint16_t ambientPressure);
  if (!p.error && p.value > 0) {
    uint16_t ambientPressure = p.value / 100;
    scd40.setAmbientPressure(ambientPressure);
  }
}

void measurement_value(MEASUREMENT &m, float val)
{
  m.value = val;
  m.time = millis();
  m.error = false;
}

void measurement_error(MEASUREMENT &m)
{
  m.error = true; 
}
