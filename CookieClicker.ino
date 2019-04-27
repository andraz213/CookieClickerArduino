/*
   CookieClicker game for arduino
   It's a ripoff of a classic clicker game for phones
   It's really nothing new

   I tested this on atmega 328p and 32u4 micro controllers,
   but it should work on others as well
   the only problem might be the low power library
   I used this one: https://github.com/rocketscream/Low-Power

   That library works on these MCUs:
   ATMega88
   ATMega168
   ATMega168P
   ATMega328P
   ATMega32U4
   ATMega644P
   ATMega1284P
   ATMega2560
   ATMega256RFR2
   ATSAMD21G18A

   the problem with some of these might be lack of memory
   Adafruit SSD1306 library uses a lot of memory, so look out for that



   I made this game a bit more interesting with added levels.
   You have 4 auto clicker uogrades and 4 click upgrades.
   To level up, you have to collect 10 of each.
   Levels get progressively harder and harder.
   I had to make my own big number notation
   1aa = 1
   1ab = 1000
   1ac = 10000000
   ...
   1jj = 10¹⁰⁰

   when you get to that point, you've beaten the game
   It will take you like 70000 years, but that's fine.

   Low Power
   After 30seconds of not doing anything, it goes into deep sleep.
   You get about 1% of the revenue for 2 hours while in sleep mode.
   the first screen also has some power saving measures, so if you want to
   reprogram your MCU, you'll have to do it from the upgrade screens and
   not from the home screen (the MCU turns off 30 times per second, which disables
   all usb functionality). If you want to decrease power conumption, desolder the led
   that is always on. Those pesky little things add 2-3mA to the total consumption.
   Even when the MCU is sleeping, so if you want it to be battery friendly, you really have to do it.

   Hookup guide
   I used pins 0 and 1.
   They work on 32u4. If you use it on other MCUs, have a look at this page to see
   which pins you can use: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

   I'm handling all the clicks with interrupts, so the pins have to support them as well.


   Display
   This game is meant for SSD1306 based 128x32 OLED displays.
   You can change it. I'm using the I2C flavor.


   EEPROM
   The game is being backed up to EEPROM every few minutes and everytime it goes to sleep and when you level up.

   https://github.com/andraz213/CookieClickerArduino

*/



//define your pins
#define CLICKPIN     1
#define CONTEXTPIN   0

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LowPower.h>
Adafruit_SSD1306 display(128, 32, &Wire, -1);

#include <EEPROM.h>

double currentCPS = 0;
double displayCPS = 0;
double cookies = 0;
double previousLvlCPS = 0;
double previousLvlClickWorth = 0;
double clickWorth = 1;
double decimalPower = 1;
int level = 1;
int upgrades [2][4];
double prices [2][4];
int lastBack = 0;

double clicks = 0.0;
double clicksMult = 1.0;
int clickPower = 7;
int secondPower = 10;
int clickCost = 10;
int secondCost = 7;

int context = 0;



long prevClick = 0;
long prevContx = 0;
long prevUpd = 0;
long loopLen = 0;






/////////////////////////////// EEPROM /////////////////////////////////////////////////

struct backupObject {
  double previousLvlCPS;
  double decimalPower;
  int level;
  int upgrades [2][4];
  double cookies;
};


void backupToEEPROM() {
  backupObject bck;
  bck.previousLvlCPS = previousLvlCPS;
  bck.decimalPower = decimalPower;
  bck.level = level;
  for (int i = 0; i < 4; i++) {
    bck.upgrades[0][i] = upgrades[0][i];
    bck.upgrades[1][i] = upgrades[1][i];
  }
  bck.cookies = cookies;
  EEPROM.put(100, bck);
}

void retrieveFromEEPROM() {
  backupObject rtrv;
  EEPROM.get(100, rtrv);
  previousLvlCPS = rtrv.previousLvlCPS;
  decimalPower = rtrv.decimalPower;
  level = rtrv.level;
  for (int i = 0; i < 4; i++) {
    upgrades[0][i] = rtrv.upgrades[0][i];
    upgrades[1][i] = rtrv.upgrades[1][i];
  }
  cookies = rtrv.cookies;
}

///////////////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////// DISPLAYING AND FORMATTING ///////////////////////////////////////


// This handles my weird number notation
String getMultString(int offset) {
  String res = "";
  int prva = (int) ( (int)(((int)decimalPower + offset) / 3) / 10);
  switch (prva) {
    case 0:
      res += "a";
      break;
    case 1:
      res += "b";
      break;
    case 2:
      res += "c";
      break;
    case 3:
      res += "d";
      break;
    case 4:
      res += "e";
      break;
    case 5:
      res += "f";
      break;
    case 6:
      res += "g";
      break;
    case 7:
      res += "h";
      break;
    case 8:
      res += "i";
      break;
    default:
      res += "j";
      break;
  }

  int druga = (int)((int)(((int)decimalPower + offset) / 3) % 10);
  switch (druga) {
    case 0:
      res += "a";
      break;
    case 1:
      res += "b";
      break;
    case 2:
      res += "c";
      break;
    case 3:
      res += "d";
      break;
    case 4:
      res += "e";
      break;
    case 5:
      res += "f";
      break;
    case 6:
      res += "g";
      break;
    case 7:
      res += "h";
      break;
    case 8:
      res += "i";
      break;
    default:
      res += "j";
      break;
  }
  return res;

}


// This makes numbers fit on the screen
// it just does this: 2443.34aa = 2.44ab

String getNumberDisplay(double num) {
  int offset = 0;
  //Serial.println(num);
  //Serial.println(" sdfds ");
  while (num > 1000) {
    num /= 1000;
    offset += 3;
  }
  String res = String(num);
  res += getMultString(offset);
  return res;
}


void sleepDisplay(Adafruit_SSD1306* display) {
  display->ssd1306_command(SSD1306_DISPLAYOFF);
}

void wakeDisplay(Adafruit_SSD1306* display) {
  display->ssd1306_command(SSD1306_DISPLAYON);
}



//this handles displaying upgrade screens
// UI is always messy, so don't beat me over this one lol

void displayContext() {

  int item = (context - 1) % 4;
  int category = 0;
  if (context - 1 > 3) category++;
  while (upgrades[category][item] > 9) {
    context++;
    context = context % 9;
    item = (context - 1) % 4;
    if (context - 1 > 3) category = 1;
  }
  if (item == 2 && upgrades[category][0] < 10) context += 2;
  if ((item == 3 && upgrades[category][1] < 10)) context ++;
  context = context % 9;

  item = (context - 1) % 4;
  if (context - 1 > 3) category = 1;
  if (context != 0) {

    String itemname = "Auto clicker";
    if (category) {
      itemname = "Click Booster";
    }
    updatePrices();
    int having = upgrades[category][item];
    int performance = 10 * pow(secondPower, item);
    if (category) performance = 10 * pow(clickPower, item);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(itemname);
    display.print(" ");
    display.print(item + 1);
    display.print(" x");
    display.println(having);
    display.print("+");
    display.print(getNumberDisplay(performance));
    if (!category)
      display.println(" CPS");
    if (category)
      display.println("/click");
    display.print("Cost: ");
    display.println(getNumberDisplay(prices[category][item]));
    display.print("Cookies ");
    display.println(getNumberDisplay(cookies));
    display.display();
  }

}



// this is displaying the home screen. Nothing special really
void displayHome() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Cookies ");
  display.println(getNumberDisplay(cookies));
  display.print("CPS ");
  display.println(getNumberDisplay(displayCPS));
  display.print("Level ");
  display.println(level);
  //display.print(clicksMult);
  display.display();

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////// GAMEPLAY AND LEVEL UP STUFF ///////////////////////////////

/////////////// HOW MUCH EARNINGS


// This calculates click worth
// click worth at the end of previous level + all the upgrades from this level
void updateClickWorth() {
  double worth = previousLvlClickWorth;
  for (int i = 0; i < 4; i++) {
    worth += 10 * upgrades[1][i] * pow(clickPower, i);
  }
  clickWorth = worth;
  if (worth == 0) {
    clickWorth = 1;
  }
}

// This figures out how much earnings you get per second  it's the same formula as for clicks really
double figureOutCPS() {
  double res = previousLvlCPS;
  for (int i = 0; i < 4; i++) {
    res += 10 * upgrades[0][i] * pow(secondPower, i);
  }
  return res;
}


//////////// PRICES AND BUYING


// I'm doing some fancy stuff here to make the game interesting
void updatePrices() {
  double multiplier = level * 0.005 + 0.10;
  for (int i = 0; i < 4; i++) {
    prices[0][i] = 40 * level * pow(secondCost, i) * pow((multiplier + 1), upgrades[0][i] * 2.7);
  }

  for (int i = 0; i < 4; i++) {
    prices[1][i] = 40 * level * pow(clickCost, i) * pow((multiplier + 1), upgrades[1][i] * 2);
  }
}


// if you have enough money, it buys what you're looking at
// it also updates all the values that change with buying an item
void buy() {

  updatePrices();
  int item = (context - 1) % 4;
  int category = 0;
  if (context - 1 > 3) category++;

  if (prices[category][item] <= cookies && upgrades[category][item] < 10) {
    cookies -= prices[category][item];
    upgrades[category][item] ++;
    currentCPS = figureOutCPS();
    updateClickWorth();
    amLevelUp();
  }

}



//////LEVELS/////////////

// it checks if you've bought enough upgrades to level up
boolean amLevelUp() {
  for (int i = 0; i < 4; i++) {
    if (upgrades[0][i] < 10 || upgrades[1][i] < 10) return false;
  }
  levelUp();
  context = 0;
  return true;
}


// just levels you up
// I'm doing some weird stuff with my weird bug number notation

int levelUp() {
  previousLvlCPS = figureOutCPS();
  previousLvlClickWorth = clickWorth;
  double chg = previousLvlCPS;
  int offset = 0;
  while (chg > 1000) {
    chg /= 1000;
    offset += 3;
    cookies /= 1000;
    previousLvlClickWorth /= 1000;
  }
  previousLvlCPS = chg;

  decimalPower += offset;
  level ++;
  for (int i = 0; i < 4; i++) {
    upgrades[0][i] = 0;
    upgrades[1][i] = 0;
  }
  updatePrices();
  updateClickWorth();
  currentCPS = figureOutCPS();
  backupToEEPROM();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////// HOUSEKEEPING AND LOW POWER AND INTERRUPTS AND TIMED OPERATIONS ///////



void timedStuff() {

  // millis resets back to zero from time to time
  // this makes sure that the game doesn't get stuck
  if (millis() < 1500) {
    prevClick = 0;
    prevContx = 0;
    prevUpd = 0;
    loopLen = 0;
  }

  // this is updating the averageCPS that gets displayed and number of clicks for click multiplier
  //it's doing that every half a second
  if (millis() - loopLen > 500) {
    displayCPS -= (displayCPS - currentCPS) * (millis() - loopLen) / 1000;
    clicks -= clicks  * (millis() - loopLen) / 10000;
    loopLen = millis();
  }

  //This adds up your per second earnings to you bank account
  // it also keeps track if when the backups are supposed to happen
  if (millis() - prevUpd >= 1000) {
    cookies += currentCPS;
    prevUpd = millis();
    lastBack--;
    if (lastBack < 1) {
      backupToEEPROM();
      lastBack = 600;
    }
  }

}




void lowPowerStuff() {

  // if you're on the home screen, it goes to sleep for 30ms to save power
  if (context == 0) {
    LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
    loopLen -= 30;
    prevUpd -= 30;
  }

  // if you haven't done anything in 30s, it goes into deep sleep
  if (millis() - prevContx > 30000 && millis() - prevClick > 30000) {
    sleepDisplay(&display);
    backupToEEPROM();

    // it wakes up every second to update your earnings for two hours
    int i = 0;
    while (millis() - prevContx > 30000 && millis() - prevClick > 30000 && i < 7200) {
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      cookies += clickWorth * clicksMult * (0.01 + 0.1 * level);
      i++;
    }
    while (millis() - prevContx > 30000 && millis() - prevClick) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }

    wakeDisplay(&display);
  }

}

// interrupt services for the clicks
void contextSwitch() {
  if (millis() - prevContx > 250) {
    context += 1;
    context = context % 9;
    prevContx = millis();
  }
}


void clickISR() {
  if (millis() - prevClick > 70) {
    if (context == 0) {
      clicks++;
      clicksMult = 1 + clicks * 9.0 / 35.0;
      displayCPS += clickWorth * clicksMult;
      cookies += clickWorth * clicksMult;
    }
    else {
      buy();
    }
    prevClick = millis();
  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////7



void setup() {


  pinMode(CONTEXTPIN, INPUT_PULLUP);
  pinMode(CLICKPIN, INPUT_PULLUP);


  //holding down both buttons before it boots will restart the game
  //       IMPORTANT       you should also do that on the first boot to wipe the EEPROM
  // if you don't do that, you will get weird values for the ammout of cookies you have

  if (digitalRead(CONTEXTPIN) == LOW && digitalRead(CLICKPIN) == LOW) {
    backupToEEPROM();
  }


  attachInterrupt(digitalPinToInterrupt(CONTEXTPIN), contextSwitch, FALLING);
  attachInterrupt(digitalPinToInterrupt(CLICKPIN), clickISR, FALLING);


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Cookie ");
  display.println("Clicker ");
  display.display();
  delay(1000);


  retrieveFromEEPROM();
  currentCPS = figureOutCPS();
  updateClickWorth();
  prevUpd = millis();
  loopLen = millis();


}




void loop() {
  if (context == 0) {
    displayHome();
  }
  else {
    displayContext();
  }

  timedStuff();
  lowPowerStuff();





}
