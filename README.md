# The-Unstoppable-Alarm
An Arduino alarm clock that requires solving math questions to disarm

## Features
- Displays date and time on a 16x2 LCD
- Alarm triggers at a set time using a buzzer
- Requires solving 5 math questions to dismiss the alarm
- The Math questions include addition, subtraction, multiplication and division
- Alarm time can be set directly using the keypad by holding the * key for 2 seconds

## Components Used 
- Arduino Uno
- DS3231 RTC module
- 16x2 LCD display with I2C converter
- Passive buzzer
- 4x3 matrix keypad
- Breadboard and jumper wires

## Libraries Used
- RTClib by Adafruit
- LiquidCrystal I2C by Frank de Brabander
- Keypad by Mark Stanley

## How to use the Alarm
### Setting the alarm
1. On the clock screen, hold the `*` key for 2 seconds
2. Type the alarm time as 4 digits e.g. `0730` for 7:30am
3. The alarm saves automatically once all 4 digits are entered
4. Press `*` to backspace or `#` to cancel

### Disarming the alarm
1. When the alarm sounds, solve the math questions displayed on the LCD
2. Type your answers using the keypad
3. Press `#` to submit your answer
4. Press `*` to backspace for mistakes
5. Solve all 5 questions correctly to disarm the alarm
