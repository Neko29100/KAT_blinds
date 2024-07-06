#include <AceButton.h>
#include <stdint.h>
#include <Arduino.h>
#include <SPI.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Servo.h>
#include <FlashStorage_SAMD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include "chars.h"

#define TIME_24_HOUR true
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t address1 = 10;
uint16_t address2 = 5;

AlarmId id;
Servo myservo;
RTC_DS3231 rtc;

using namespace ace_button;

AceButton button1(1);
AceButton button2(2);
AceButton button3(3);

void handleEvent(AceButton*, uint8_t, uint8_t);

unsigned long MOVING_TIME = 3000;  // moving time is 3 seconds

unsigned long currentMillis;
unsigned long previousMillis;
unsigned long elapsedMillis;
unsigned long lastButtonPressTime = 0;  // Last button press time
int screenTimeout;              // Screen timeout period (30 seconds)

bool delay_state = false;
bool alarm_state = false;
bool alarm_triggered = false;  // Flag to track if alarm has triggered
bool screenOn;
bool time_display_state = false;

String timeString;
String timeDisplayString;
char *currentDay;

char charArray[100];  
String stringTest;
char delimiters[] = ",";
int synch_val[6] = { 0 };
char daysOfTheWeek[7][12] = {"S0ndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "L0rdag"};


int hours = 0;
int minutes = 0;
int minutesNew;
int hoursNew;

const int reset_servo = 100;
const int release_servo = 0;
bool switch_state = false;  // Changed to bool
bool rtc_synch_state = false;

void setup() {
  Serial.begin(57600);

  if (!rtc.begin()) {
    for (int i; i <= 60; i++) {
      pinMode(PIN_LED2, OUTPUT);
      digitalWrite(PIN_LED2, HIGH);
      delay(500);
      digitalWrite(PIN_LED2, LOW);
      delay(500);
    }
    while (1) delay(10);
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  delay(200);
  display.clearDisplay();
  display.setTextColor(WHITE);

  myservo.attach(7);

  myservo.write(reset_servo);

  EEPROM.get(address1, minutes);
  EEPROM.get(address2, hours);

  hoursNew = abs(hours);
  minutesNew = abs(minutes);

  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);

  printUpdatedTime();
}

void loop() {

  DateTime now = rtc.now();
  currentDay = daysOfTheWeek[now.dayOfTheWeek()];
  

  if (rtc_synch_state == false) {
    setTime(now.hour(), now.minute(), now.second(), now.dayOfTheWeek(), now.month(), now.year());
    rtc_synch_state = true;
  }

  button1.check();
  button2.check();
  button3.check();

  Alarm.delay(1);
  if (!alarm_state) {
    Alarm.alarmRepeat(abs(hours), abs(minutes), 0, MorningAlarm);
    alarm_state = true;
  }

  if (millis() - lastButtonPressTime >= screenTimeout) {
    if (screenOn) {
      time_display_state = false;
      display.clearDisplay();
      display.display();
      screenOn = false;
    }
  } else {
    if (!screenOn) {
      screenOn = true;
      printUpdatedTime();
    }
  }

  if (time_display_state == true) {
    time_display();
  }

  timeString = String(abs(hours)) + "h" + String(abs(minutes)) + "m";

  if (Serial.available() > 0) {
    stringTest = Serial.readStringUntil('\n');  // Read until newline character
    serial_synch();
  }

  currentMillis = millis();
  elapsedMillis = currentMillis - previousMillis;
}

void serial_synch() {

  stringTest.toCharArray(charArray, sizeof(charArray));  // Convert String to char array

  char* ptr = strtok(charArray, delimiters);
  int index = 0;

  while (ptr != NULL && index < 6) {
    synch_val[index] = atoi(ptr);
    index++;
    ptr = strtok(NULL, delimiters);
  }

  rtc.adjust(DateTime(synch_val[0] + 2000, synch_val[1], synch_val[2], synch_val[3], synch_val[4], synch_val[5]));
  rtc_synch_state = false;

}

void printUpdatedTime() {
  timeString = String(abs(hoursNew)) + "h" + String(abs(minutesNew)) + "m";

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Alarm Time");

  display.setTextSize(3);
  display.setCursor(0, 28);
  display.print(timeString);

  if (!switch_state) {
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("Modifying Minutes");
  } else {
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("Modifying Hours");
  }

  display.display();
}

void time_display() {

  timeDisplayString = String(hour()) + "h" + String(minute()) + "m" + String(second()) + "s";
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  int cursorX = 0;

  for (int i = 0; i < strlen(currentDay); i++) {
    if (currentDay[i] == '0') {
      // Draw the custom character `ø`
      display.drawBitmap(cursorX, 3, bitmap_o_12x12, 12, 12, WHITE);
      cursorX += 12; // Move cursor to the next position
    } else {
      // Draw the regular character
      display.setCursor(cursorX, 0);
      display.print(currentDay[i]);
      cursorX += 12; // Move cursor to the next position
    }
  }

  display.setTextSize(2);
  display.setCursor(0, 28);
  display.print(timeDisplayString);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("ello there");

  display.display();
}

void moveServoToPosition(int target, int origin, int step) {
  while (origin != target) {
    origin += step;
    myservo.write(origin);
    Alarm.delay(20);
  }
}


void MorningAlarm() {
  if (elapsedMillis >= 15000) {
    previousMillis = currentMillis;

    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
    display.display();

    moveServoToPosition(release_servo, reset_servo, -1);  // Move to release position
    // Move back to neutral after delay
    Alarm.delay(5000);                                   // Adjust delay as needed
    moveServoToPosition(reset_servo, release_servo, 1);  // Move back to neutral
    elapsedMillis = 0;
    lastButtonPressTime = millis();  // Update last button press time

    rtc_synch_state = false;
  }
}

void updateTime(int increment) {
  if (!switch_state) {  // Update minutes
    int oldMinutes = minutesNew;
    minutesNew = (minutesNew + increment + 60) % 60;

    if (increment > 0 && oldMinutes > minutesNew) {
      // Wrapped around forward
      hoursNew = (hoursNew + 1 + 24) % 24;
    } else if (increment < 0 && oldMinutes < minutesNew) {
      // Wrapped around backward
      hoursNew = (hoursNew - 1 + 24) % 24;
    }
  } else {  // Update hours
    hoursNew = (hoursNew + increment + 24) % 24;
  }
  printUpdatedTime();
}


void saveAlarmTime() {
  EEPROM.put(address1, minutesNew);
  EEPROM.put(address2, hoursNew);
  minutes = minutesNew;
  hours = hoursNew;

  timeString = String(hoursNew) + "h" + String(minutesNew) + "m";
  display.clearDisplay();

  // display Alarm Time
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Alarm Time");

  display.setTextSize(3);
  display.setCursor(0, 28);
  display.print(timeString);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("Alarm Saved!");

  display.display();

  //Serial.println("meow");

  alarm_state = false;

  if (!EEPROM.getCommitASAP()) {
    EEPROM.commit();
  }
}

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  lastButtonPressTime = millis();  // Update last button press time
  switch (eventType) {
    screenTimeout = 15000;
    case AceButton::kEventClicked:
      if (time_display_state == false) {
        if (button->getPin() == 1) {
          updateTime(-1);
        } else if (button->getPin() == 2) {
          updateTime(1);
        } else if (button->getPin() == 3) {
          switch_state = !switch_state;  // Toggle switch state
          printUpdatedTime();
        }
      }
      break;

    case AceButton::kEventLongPressed:
      screenTimeout = 15000;
      if (button->getPin() == 3) {
        saveAlarmTime();
      }
      break;

    case AceButton::kEventDoubleClicked:
      screenTimeout = 30000;
      if (button->getPin() == 3) {
        time_display_state = !time_display_state;
        if (time_display_state == false) {
          printUpdatedTime();
        }
      }
      break;

    case AceButton::kEventRepeatPressed:
      screenTimeout = 15000;
      if (time_display_state == false) {
        if (button->getPin() == 1) {
          updateTime(-1);
        } else if (button->getPin() == 2) {
          updateTime(1);
        }
      }
      break;
  }
}