# <center> KAT_blinds </center>
 meow

## <ins>- Parts list: </ins>
- Seeeduino Xiao (ESP32)
- RTC3107
- 3 push buttons
- 128x64 OLED i2c screen
- Servomotor

## <ins>- Libraries required :</ins>
- TimeLib (Time keeping and RTC synchronisation)
- TimeAlarms (Dependent of TimeLib, enables alarm going off at set times)
- Adafruit_SSD1306 (OLED via i2c)
- Adafruit_GFX (More screen tings)
- AceButton (Debouncing of button & press tracking)
- FlashStorage_SAMD (emulating flash memory as EEPROM, as the ESP32 doesnt have any EEPROM)