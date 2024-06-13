#include <AceButton.h>
#include <SPI.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Servo.h>
#include <FlashStorage_SAMD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "chars.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t address1 = 10;
uint16_t address2 = 5;

AlarmId id;
Servo myservo;

using namespace ace_button;

AceButton button1(1);
AceButton button2(2);
AceButton button3(3);

void handleEvent(AceButton*, uint8_t, uint8_t);

unsigned long MOVING_TIME = 3000; // moving time is 3 seconds

unsigned long currentMillis;
unsigned long previousMillis;
unsigned long elapsedMillis;
unsigned long lastButtonPressTime = 0; // Last button press time
const unsigned long screenTimeout = 15000; // Screen timeout period (30 seconds)

bool delay_state = false;
bool alarm_state = false;
bool alarm_triggered = false; // Flag to track if alarm has triggered
bool screenOn;

int hours = 0;
int minutes = 0;
int minutesNew;
int hoursNew;

const int reset_servo = 100;
const int release_servo = 0;
bool switch_state = false; // Changed to bool

String timeString;

void setup() {
  Serial.begin(57600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  myservo.attach(7);

  myservo.write(reset_servo);

  EEPROM.get(address1, minutes);
  EEPROM.get(address2, hours);
  
  hoursNew = hours;
  minutesNew = minutes;

  Serial.print("Value at address "); Serial.print(address1);  Serial.print(" = "); Serial.println(hours); 
  Serial.print("Value at address "); Serial.print(address2);  Serial.print(" = "); Serial.println(minutes); 
  
  setTime(8,29,29,1,1,11); // set time to Saturday 8:29:00am Jan 1 2011

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
  button1.check();
  button2.check();
  button3.check();
  
  Alarm.delay(1);
  if (!alarm_state) {
    Alarm.alarmRepeat(hours, minutes, 0, MorningAlarm);
    alarm_state = true;
  }

  if (millis() - lastButtonPressTime >= screenTimeout) {
    if (screenOn) {
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
  
  timeString = String(hours) + "h" + String(minutes) + "m";

  Serial.println(elapsedMillis);
  currentMillis = millis();
  elapsedMillis = currentMillis - previousMillis;
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

void moveServoToPosition(int target, int origin, int step) {
  while (origin != target) {
    origin += step;
    myservo.write(origin);
    Alarm.delay(20);
    }
  }


void MorningAlarm() {
  if (elapsedMillis >= 15000){
    previousMillis = currentMillis;

    Serial.println("Morning Alarm triggered");
    
    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap, 128, 64, WHITE);
    display.display();

    moveServoToPosition(release_servo, reset_servo, -1); // Move to release position
    // Move back to neutral after delay
    Alarm.delay(5000); // Adjust delay as needed
    moveServoToPosition(reset_servo, release_servo, 1); // Move back to neutral
    elapsedMillis = 0;
    lastButtonPressTime = millis(); // Update last button press time
  }
}

void updateTime(int increment) {
  if (!switch_state) {
    minutesNew = (minutesNew + increment + 60) % 60;
    if (minutesNew == 0 || minutesNew == 59) {
      hoursNew = (hoursNew + increment + 24) % 24;
    }
  } else {
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
        
  alarm_state = false;    

  if (!EEPROM.getCommitASAP()) {
    EEPROM.commit();
  }

}

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  lastButtonPressTime = millis(); // Update last button press time
  switch (eventType) {
    case AceButton::kEventClicked:
      if (button->getPin() == 1) {
        updateTime(-1);
      } else if (button->getPin() == 2) {
        updateTime(1);
      } else if (button->getPin() == 3) {
        switch_state = !switch_state; // Toggle switch state
        printUpdatedTime();
      }
      break;

    case AceButton::kEventLongPressed:
      if (button->getPin() == 3) {
        saveAlarmTime();
      }
      break;

    case AceButton::kEventRepeatPressed:
      if (button->getPin() == 1 && !switch_state) {
        updateTime(-1);
      } else if (button->getPin() == 2 && !switch_state) {
        updateTime(1);
      }
      break;
  }
}
