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
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t address1 = 10;
uint16_t address2 = 5;

AlarmId id;
Servo myservo;
RTC_DS3231 rtc;



//ButtonHandler handleEvent;

//void handleEvent(AceButton*, uint8_t, uint8_t);

unsigned long MOVING_TIME = 3000;

unsigned long currentMillis;
unsigned long previousMillis;
unsigned long elapsedMillis;
unsigned long lastButtonPressTime = 0;
int screenTimeout;

bool delay_state = false;
bool alarm_state = false;
bool alarm_triggered = false;
bool screenOn;

int hours = 0;
int minutes = 0;
int minutesNew = 0;
int hoursNew = 0;

const int reset_servo = 100;
const int release_servo = 0;
bool switch_state = false;
bool rtc_synch_state = false;
bool time_display_state = false;

String timeString;
String timeDisplayString;

char charArray[100];  // Increase size as needed based on maximum input length
String stringTest;
char delimiters[] = ",";
int synch_val[6] = { 0 };  // Ensure the array size matches the number of elements you expect


using namespace ace_button;

const uint8_t BUTTON1_PIN = 1;
const uint8_t BUTTON2_PIN = 2;
const uint8_t BUTTON3_PIN = 3;

ButtonConfig buttonConfig;
AceButton b1(&buttonConfig, BUTTON1_PIN);
AceButton b2(&buttonConfig, BUTTON2_PIN);
AceButton b3(&buttonConfig, BUTTON3_PIN);

class ButtonHandler : public IEventHandler {
public:
  explicit ButtonHandler() {}

  void handleEvent(AceButton* button, uint8_t eventType,
                   uint8_t /*buttonState*/) override {
    uint8_t pin = button->getPin();
    switch (eventType) {
      case AceButton::kEventPressed:
        {
          screenTimeout = 15000;
          uint16_t now = millis();
          if (pin == 1) {
            updateTime(-1);
          } else if (pin == 2) {
            mIsB1Pressed = true;
            updateTime(1);
          } else if (pin == 3) {
            mIsB2Pressed = true;
            switch_state = !switch_state;
            printUpdatedTime();
          }

          if (checkBothPressed()) {
            handleBothPressed();
          }

          break;
        }
        lastButtonPressTime = millis();

      case AceButton::kEventLongPressed:
        screenTimeout = 15000;
        if (button->getPin() == 3) {
          saveAlarmTime();
        }
        break;

      case AceButton::kEventRepeatPressed:
        screenTimeout = 15000;
        //if (time_display_state == false) {
          if (button->getPin() == 1) {
            updateTime(-1);
          } else if (button->getPin() == 2) {
            updateTime(1);
          }
       // }
        break;

      case AceButton::kEventDoubleClicked:
        if (button->getPin() == 3) {
          time_display_state = !time_display_state;
          time_display();
          screenTimeout = 30000;

          printUpdatedTime();
        }
        break;

      case AceButton::kEventClicked:
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

 /* if (time_display_state == true) {
    time_display();
  }*/


  void handleBothPressed() {

  }

  void time_display() {
  timeDisplayString = String(hour()) + "h" + String(minute()) + "m" + String(second()) + "s";
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("meow meow meow meow");
  display.setCursor(0, 7);
  display.print("meow meow meow meow");

  display.setTextSize(2);
  display.setCursor(0, 28);
  display.print(timeDisplayString);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("ello there");

  display.display();
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

    alarm_state = false;
    rtc_synch_state = false;

    if (!EEPROM.getCommitASAP()) {
      EEPROM.commit();
    }
  }

  void printUpdatedTime() {
    timeString = String(hoursNew) + "h" + String(minutesNew) + "m";

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


private:
  /**
     * Determine if a transition to both buttons are pressed at the same time
     * has happened. Pressing one button up and down, while keeping the other
     * one pressed should NOT cause this event.
     */
  bool checkBothPressed() {
    bool bothPressed = mIsB1Pressed && mIsB2Pressed;
    if (bothPressed && !mBothPressed) {
      mBothPressed = true;
      return true;
    } else {
      return false;
    }
  }

private:
  bool mIsB1Pressed = false;
  bool mIsB2Pressed = false;
  bool mBothPressed = false;
};

ButtonHandler handleEvent;
 
void checkButtons() {
  static uint16_t lastCheck;

  // DO NOT USE delay(5) to do this.
  // The (uint16_t) cast is required on 32-bit processors, harmless on 8-bit.
  uint16_t now = millis();
  if ((uint16_t)(now - lastCheck) >= 5) {
    lastCheck = now;
    b1.check();
    b2.check();
    b3.check();
  }
}

void setup() {
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

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

  buttonConfig.setIEventHandler(&handleEvent);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  //buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);

  checkButtons();


}

void loop() {

  DateTime now = rtc.now();

  if (rtc_synch_state == false) {
    setTime(now.hour(), now.minute(), now.second(), now.dayOfTheWeek(), now.month(), now.year());
    rtc_synch_state = true;
  }
  
  checkButtons();

  Alarm.delay(1);
  if (!alarm_state) {
    Alarm.alarmRepeat(hours, minutes, 0, MorningAlarm);
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
      //printUpdatedTime();
    }
  }


currentMillis = millis();
  /*if (millis() > 10000) {
    Serial.flush();
    Serial.end();
  }*/

  static unsigned long lastTime = 0;
  static unsigned long serialTime = 0;

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

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("Integers received:");

  // Display each integer value
  for (int i = 0; i < 6; i++) {
    display.print("synch_val[");
    display.print(i);
    display.print("] = ");
    display.println(synch_val[i]);
  }

  display.display();
}

void time_display() {
  timeDisplayString = String(hour()) + "h" + String(minute()) + "m" + String(second()) + "s";
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("meow meow meow meow");
  display.setCursor(0, 7);
  display.print("meow meow meow meow");

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

    moveServoToPosition(release_servo, reset_servo, -1);
    Alarm.delay(5000);
    moveServoToPosition(reset_servo, release_servo, 1);
    elapsedMillis = 0;
    lastButtonPressTime = millis();

    rtc_synch_state = false;
  }
}