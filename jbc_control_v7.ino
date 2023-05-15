#include <EEPROM.h>                        // EEPROM Library - for temperature saving during off power
#include <Wire.h>                          // I2c Communication Library
#include <Adafruit_SSD1306.h>              // Library that controls OLED display
#include <Adafruit_GFX.h>                  // Library necesarry to use fancier font
#include <Fonts/FreeSansBold18pt7b.h>      // Fancier font
#include "OneButton.h"                     // Button Library
#define HEAT 9                             // Pin that drives the heater circuit
#define SLEEP 5                            // Sleep detection when put to stand
#define ADC A0                             // ADC Pin that measures amplified thermocouple voltage
Adafruit_SSD1306 display(128, 64, &Wire);  // Display size in pixels
int tempSet;                               // Set tempareture
int tempSleep = 140;                       // Tip temperature at sleep in stand
int temp = 0;                              // Read temperature
int value = 0;                             // ADC thermocouple(after amplification by op amp) value from 0 to 1023
int cycle = 0;                             // Number of heating cycles, after which temperature will be send to display
int state = 1;                             // Status of work. 0=Run 1=Sleep
int time = 0;                              // How long will heater will be on between temperature measuring cycles
bool beep0 = 0;                            // Beeping shit when change state occurs(annoying)
bool beep1 = 0;                            //
OneButton up(4, true, true);               // Assigning pin numbers to buttons
OneButton down(2, true, true);             //
OneButton center(3, true, true);           //


void setup(void) {
  tempSet = (EEPROM.read(0) << 8) + EEPROM.read(1);  // Read last saved temperature from memory
  display.begin(0x02, 0x3c);                         // OLED Display initialization
  display.clearDisplay();                            // Clear Display buffer
  display.setTextColor(SSD1306_WHITE);               // Set color lol
  display.setFont(&FreeSansBold18pt7b);              // Set font
  pinMode(HEAT, OUTPUT);                             // Set heater pin as output
  digitalWrite(HEAT, LOW);                           // And turn heating off in the begining
  pinMode(SLEEP, INPUT);                             // Sleep detection as input ofc
  tone(6, 1800, 100);                                // Play some tone bitch
  up.attachClick(inc1);                              // Buttons attachments to routines and pressing delays
  up.attachDuringLongPress(inc10);
  up.setDebounceTicks(5);
  up.setClickTicks(500);
  up.setPressTicks(150);
  down.attachClick(dec1);
  down.attachDuringLongPress(dec10);
  down.setDebounceTicks(5);
  down.setClickTicks(500);
  down.setPressTicks(150);
}


void loop() {                  //Main loop
  state = digitalRead(SLEEP);  // Read current state of handle
  switch (state) {             // And switch case accordingly
    case 0:                    //
      // beepsleep();          // If You fancy this beeping, play with it
      // beep1 = false;
      sleep();
      break;
    case 1:
      // beeprun();
      //  beep0 = false;
      run();
      break;
  }
}


/////////Other Routines//////////

void run() {
  up.tick();                     // Search for pressed buttons
  down.tick();                   //
  center.tick();                 //
  digitalWrite(HEAT, LOW);       // Turn heat low to measure temperature
  delay(2);                      // Wait for shit to calm down, and op amp to stabilize
  if (cycle > 60) {              // If cycles count is bigger than this number ->
    displrun();                  // Show shit on display
    cycle = 0;                   // And heating fandango begins again
  }                              //
  readValue();                   // Now read the temperature value
  temp = ((value / 2.7) + 24);  // Calculate temperature to degrees celcius and make some math. <<< CALIBRATION HERE BOY
  time = (tempSet - temp) / 3;   // Some tricky shit math to differ length of heating window. When far away form desired temp - longer window. When close to setpoint - just bit by bit. This is instead od some fancy control.
  if (time < 2) {                // Not to make this window too short
    time = 2;                    //
  }                              //
  if (temp >= tempSet) {         // To avoid strange situation. kek
    time = 0;
  }
  if (temp < tempSet) {        // Self explainatory
    digitalWrite(HEAT, HIGH);  //Show 'em boy!
    delay(time);
  } else {
    digitalWrite(HEAT, LOW);
  }
  cycle++;  //Increase cycles to display shot on display.
  //(When heating, we spare no time for some stupid things as showing things on display. Performance first. If not heating, there is more time, and cycle counter will fill up faster, so updates on display will be more often)
}


void sleep() {  // Same shit as upwards but temperature is hardset in program(for now), and the temperature ramp up is less steep to spare tip life.
  up.tick();
  down.tick();
  center.tick();
  digitalWrite(HEAT, LOW);
  delay(20);         // Stabilise op amp and make readings and heating less often
  if (cycle > 20) {  // Now we can display things more often because there is no need for performance
    displsleep();    // Show shit on display
    digitalWrite(12, LOW);
    cycle = 0;
  }
  readValue();
  temp = ((value / 2.7) + 24);
  time = (tempSleep - temp) / 20;  // Slower heat up ramp, less thermal shock
  if (time < 1) {
    time = 1;
  }
  if (temp >= tempSleep) {
    time = 0;
  }
  if (temp < tempSleep) {
    digitalWrite(HEAT, HIGH);
    delay(time);
  } else {
    digitalWrite(HEAT, LOW);
  }
  cycle++;
}


void readValue() {  //Reading ADC Routine
  int i;
  int readings = 5;  // Number of readings to average
  for (i = 0; i < readings; i++) {
    value = value + analogRead(ADC);  // Collect all readings on one stack
  }
  value = (value / readings);  //And divide by number of readings to get an average, which is more stable than single shots.
}


void displrun() {  //Display routine when performing
  display.clearDisplay();
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(56, 27);
  display.print(temp);
  display.setCursor(56, 58);
  display.print(tempSet);
  display.setFont();
  display.setCursor(22, 12);
  display.print("TIP:");
  display.setCursor(22, 46);
  display.print("SET:");
  display.display();
}


void displsleep() {  //Display routine when sleeping
  display.clearDisplay();
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(56, 27);
  display.print(temp);
  display.setCursor(56, 58);
  display.print(tempSet);
  display.setFont();
  display.setCursor(22, 12);
  display.print("TIP:");
  display.setCursor(12, 28);
  display.print("Sleep");
  display.setCursor(22, 46);
  display.print("SET:");
  display.display();
}


void inc10() {  // Increase temperature by 10 degrees
  tempSet = tempSet + 10;
  if (tempSet > 450) {  // Max temperature to set
    tempSet = 450;
  }
  cycle = cycle + 30;
  delay(50);
  savetemp();
}


void dec10() {  // Decrease temperature by 10 degrees
  tempSet = tempSet - 10;
  if (tempSet < 50) {  // Min temperature to set
    tempSet = 50;
  }
  cycle = cycle + 30;
  delay(50);
  savetemp();
}


void inc1() {  // Increase temperature by 10 degrees(was ment to be by 1, but there is no need for so small change, you can adjust to your flavor)
  tempSet = tempSet + 10;
  if (tempSet > 450) {
    tempSet = 450;
  }
  cycle = cycle + 30;
  savetemp();
}


void dec1() {  // Decrease temperature by 10 degrees(was ment to be by 1, but there is no need for so small change, you can adjust to your flavor)
  tempSet = tempSet - 10;
  if (tempSet < 50) {
    tempSet = 50;
  }
  cycle = cycle + 30;
  savetemp();
}


void savetemp() {  // Save set temperature to EEPROM dividing it to two bytes because max value of single one is 255
  EEPROM.write(0, tempSet >> 8);
  EEPROM.write(1, tempSet & 0xFF);
}


void beepsleep() {
  if (beep0 == false) {
    tone(6, 1400, 100);
    beep0 = true;
  }
}


void beeprun() {
  if (beep1 == false) {
    tone(6, 1800, 100);
    beep1 = true;
  }
}