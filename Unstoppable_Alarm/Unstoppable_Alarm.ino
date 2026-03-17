#include <Wire.h> // I2C communication library
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C address is 0x27
RTC_DS3231 rtc;

#define BUZZER_PIN 8 // connect buzzer to pin 8 

// set up Keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// corresponding connected Arduino pins 
byte rowPins[ROWS] = {7, 6, 5, 4};
byte colPins[COLS] = {3, 2, 9}; // used pin 9 as pin 1 was causing an error
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// alarm time (can be changed using Keypad too)
int ALARM_HOUR   = 7;
int ALARM_MINUTE = 30;

// variables to keep track of alarm state
bool alarmRinging      = false;
bool mathActive        = false;
bool alarmDone         = false;
bool settingAlarm      = false;
int  questionsLeft     = 5;

int  num1, num2, correctAnswer;
char currentOperator;
String userInput  = "";
String alarmInput = "";

unsigned long lastBeepTime     = 0;
unsigned long lastFlashTime    = 0;
unsigned long goodMorningStart = 0;
unsigned long holdStartTime    = 0;

const unsigned long HOLD_DURATION = 2000; // 2s holding down * to set alarm 

// helps alternate between time and morning message after alarm is disarmed
bool flashState        = false;
bool showingGoodMorning = false;

const int BEEP_INTERVAL  = 600;
const int FLASH_INTERVAL = 2000;

int  lastSecond     = -1;
bool lastFlashState = !flashState;

// generate a random Maths Qn
void generateQuestion() {
  randomSeed(millis());
  int opIndex = random(0, 4);

  if (opIndex == 0) {
    num1 = random(15, 100);
    num2 = random(15, 100);
    correctAnswer   = num1 + num2;
    currentOperator = '+';
  } else if (opIndex == 1) {
    num1 = random(15, 100);
    num2 = random(15, num1);
    correctAnswer   = num1 - num2;
    currentOperator = '-';
  } else if (opIndex == 2) {
    num1 = random(15, 30);
    num2 = random(15, 30);
    correctAnswer   = num1 * num2;
    currentOperator = '*';
  } else {
    num2 = random(3, 20);
    correctAnswer   = random(3, 20);
    num1 = num2 * correctAnswer;
    currentOperator = '/';
  }
}

void displayQuestion() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(num1);
  lcd.print(' ');
  lcd.print(currentOperator);
  lcd.print(' ');
  lcd.print(num2);
  lcd.print("=?");
  lcd.setCursor(0, 1);
  lcd.print("Ans:");
  lcd.print(userInput);
}

// centres a printed string 
void printCentered(String text, int row) {
  int len    = text.length();
  int spaces = (16 - len) / 2;
  lcd.setCursor(spaces, row);
  lcd.print(text);
}

// adds a 0 in front of single digit numbers 
String twoDigit(int n) {
  return (n < 10 ? "0" : "") + String(n);
}

void displayClock(DateTime now) {
  if (now.second() == lastSecond) return;
  lastSecond = now.second();
  // row 0: date, row 1: time
  lcd.clear();
  String dateStr = String(now.year()) + "/" + twoDigit(now.month()) + "/" + twoDigit(now.day());
  printCentered(dateStr, 0); 

  String timeStr = twoDigit(now.hour()) + ":" + twoDigit(now.minute()) + ":" + twoDigit(now.second());
  printCentered(timeStr, 1);
}

void displayAlarmSetting() {
  lcd.clear();
  printCentered("Set alarm time", 0);

  String display = "";
  if (alarmInput.length() >= 1) display += alarmInput[0]; else display += "_";
  if (alarmInput.length() >= 2) display += alarmInput[1]; else display += "_";
  display += ":";
  if (alarmInput.length() >= 3) display += alarmInput[2]; else display += "_";
  if (alarmInput.length() >= 4) display += alarmInput[3]; else display += "_";

  printCentered(display, 1);
}

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);

  keypad.setHoldTime(HOLD_DURATION);
  keypad.setDebounceTime(50); // delay between each key press

  if (!rtc.begin()) {
    lcd.print("RTC not found!");
    while (1);
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // comment out and reupload after initial upload 
}

void loop() {
  DateTime now = rtc.now();

  // reset alarmDone at midnight so alarm can trigger again the next day
  if (now.hour() == 0 && now.minute() == 0 && now.second() == 0) {
    alarmDone = false;
  }

  // trigger alarm only if not already done this minute (in the event you finish the math questions in less than 1min)
  if (now.hour() == ALARM_HOUR && now.minute() == ALARM_MINUTE && !alarmDone && !alarmRinging && !settingAlarm) {
    alarmRinging  = true;
    mathActive    = true;
    questionsLeft = 5;
    lastSecond    = -1;
    generateQuestion();
    displayQuestion();
  }

  if (alarmRinging) {
    if (millis() - lastBeepTime >= BEEP_INTERVAL) {
      tone(BUZZER_PIN, 4000, 500);
      lastBeepTime = millis();
    }

    if (mathActive) {
      char key = keypad.getKey();

      if (key) {
        if (key >= '0' && key <= '9') {
          if (userInput.length() < 4) {
            userInput += key;
            displayQuestion();
          }
        } else if (key == '*') {
          if (userInput.length() > 0) {
            userInput.remove(userInput.length() - 1);
            displayQuestion();
          }
        } else if (key == '#') {
          if (userInput.length() > 0) {
            int userAnswer = userInput.toInt();
            userInput = "";

            if (userAnswer == correctAnswer) {
              questionsLeft--;
              lcd.clear();

              if (questionsLeft == 0) {
                alarmRinging       = false;
                mathActive         = false;
                alarmDone          = true;
                showingGoodMorning = true;
                goodMorningStart   = millis();
                flashState         = false;
                lastFlashState     = true;
                lastFlashTime      = millis();
                lastSecond         = -1;
                noTone(BUZZER_PIN);
              } else {
                lcd.clear();
                printCentered("Correct!", 0);
                printCentered(String(questionsLeft) + " left", 1);
                delay(1500);
                generateQuestion();
                displayQuestion();
              }
            } else {
              lcd.clear();
              printCentered("Wrong!", 0);
              printCentered("Try again", 1);
              delay(1500);
              displayQuestion();
            }
          }
        }
      }
    }

  } else if (settingAlarm) {
    char key = keypad.getKey();

    if (key) {
      if (key >= '0' && key <= '9') {
        if (alarmInput.length() < 4) {
          alarmInput += key;
          displayAlarmSetting();
        }

        if (alarmInput.length() == 4) {
          int hour   = alarmInput.substring(0, 2).toInt();
          int minute = alarmInput.substring(2, 4).toInt();

          if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
            ALARM_HOUR   = hour;
            ALARM_MINUTE = minute;
            lcd.clear();
            printCentered("Alarm set for", 0);
            printCentered(twoDigit(ALARM_HOUR) + ":" + twoDigit(ALARM_MINUTE), 1);
            delay(2000);
          } else {
            lcd.clear();
            printCentered("Invalid time!", 0);
            printCentered("Try again", 1);
            delay(2000);
            displayAlarmSetting();
          }

          alarmInput   = "";
          settingAlarm = false;
          lastSecond   = -1;
        }

      } else if (key == '*') {
        if (alarmInput.length() > 0) {
          alarmInput.remove(alarmInput.length() - 1);
          displayAlarmSetting();
        }
      } else if (key == '#') {
        alarmInput   = "";
        settingAlarm = false;
        lastSecond   = -1;
        lcd.clear();
        printCentered("Cancelled", 0);
        delay(1500);
      }
    }

  } else if (showingGoodMorning) {
    if (millis() - goodMorningStart >= 30000) {
      showingGoodMorning = false;
      lastSecond = -1;
      lcd.clear();
    } else if (millis() - lastFlashTime >= FLASH_INTERVAL) {
      flashState    = !flashState;
      lastFlashTime = millis();

      if (flashState != lastFlashState) {
        lastFlashState = flashState;
        lcd.clear();
        if (flashState) {
          displayClock(now);
          lastSecond = now.second();
        } else {
          printCentered("Good Morning :)", 0);
          printCentered("Have a good day!", 1);
        }
      }
    }

  } else {
    // check for holding of * to set alarm
    keypad.getKeys(); 

    for (int i = 0; i < LIST_MAX; i++) {
      if (keypad.key[i].kchar == '*') {
        if (keypad.key[i].kstate == HOLD) {
          settingAlarm = true;
          alarmInput   = "";
          lcd.clear();
          printCentered("Setting alarm...", 0);
          delay(1000);
          displayAlarmSetting();
          break;
        }
      }
    }

    displayClock(now);
  }
}