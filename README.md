# CookieClickerArduino

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

#   Low Power
   After 30seconds of not doing anything, it goes into deep sleep.
   You get about 1% of the revenue for 2 hours while in sleep mode.
   the first screen also has some power saving measures, so if you want to
   reprogram your MCU, you'll have to do it from the upgrade screens and
   not from the home screen (the MCU turns off 30 times per second, which disables
   all usb functionality). If you want to decrease power conumption, desolder the led
   that is always on. Those pesky little things add 2-3mA to the total consumption.
   Even when the MCU is sleeping, so if you want it to be battery friendly, you really have to do it.

 #  Hookup guide
   I used pins 0 and 1.
   They work on 32u4. If you use it on other MCUs, have a look at this page to see
   which pins you can use: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
   I'm handling all the clicks with interrupts, so the pins have to support them as well.


#   Display
   This game is meant for SSD1306 based 128x32 OLED displays.
   You can change it. I'm using the I2C flavor.


 #  EEPROM
   The game is being backed up to EEPROM every few minutes and everytime it goes to sleep and when you level up.

