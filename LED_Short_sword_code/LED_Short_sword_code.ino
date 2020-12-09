//This code is placed under the MIT license
//Copyright (c) 2020 Albert Barber
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

//Code intended to run on Arduino Pro-mini or Uno

//libraries
#include <EEPROM.h>
#include <PixelStrip.h>
#include <TimerOne.h>

//==============================================================
//YOU CAN SKIP EFFECTS BY CHANGING THEIR CASE VALUE TO 999
//==============================================================

//disables/enables setting brightness on triple button tap using potentiometer,
//if disabled, brightness will be set to initial value of "brightness" var below
#define BRIGHTNESS_MODE_ENABLE true

//disables/enables using EEPROM to store last used effect and brightness
//since EEPROM is limited to 100000 write cycles before it might error, if you're going to be changing the effect mode a lot,
//disabling EEPROM storage might make sense
#define EEPROM_MODE_ENABLE     true

//pin definitions
#define PIXEL_PIN  3  //pixel data output pin
#define BUTTON_PIN 2  //control button pin
#define POT_PIN    A5 //brigntess control potentiometer pin

//EEPROM addresses
//if your EEPROM starts mis-behaving, changing the addresses might help
#define PATTERN_COUNT_ADDR 0
#define BRIGHTNESS_ADDR    1
#define EEPROM_COM_TIME    5000000 //delay between button inputs and EEPROM writing in microsec, done to minimize EEPROM writes

//button debounce, double press values, in milliseconds
#define DEBOUNCE_TIME         100
#define DOUBLE_PRESS_INTERVAL 400 //maximum time between presses for them to count as a double press

#define NUM_EFFECTS           25 //total number of effect animations

//====================================================================================
//some global variables
boolean direct = true; //direction setting for various effects
boolean setBrightnessMode = false; //toggle for jumping into brightness control mode
volatile int brightness = 100; //default initial brightness value (actual value is read from eeprom below)
volatile int patternCounter = 0; //default initial pattern (actual value is read from eeprom below)
int buttonPressCount = 0; //used for counting double/triple button presses
int potReading = 0; //output from brightness control potentiometer
//time vars for checking button presses
volatile unsigned long last_interrupt_time = 0;
volatile unsigned long interrupt_time = 0;

//======================================================================================
//strip setup
const uint16_t stripLength = 164; //num of leds
const uint8_t stripType = NEO_GRB + NEO_KHZ800;
PixelStrip strip = PixelStrip(stripLength, PIXEL_PIN, stripType);

//========================================================================================
//colors and pallets
uint32_t red = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t white = strip.Color(255, 255, 255);
uint32_t ltOrange = strip.Color(255, 154, 0);
uint32_t redOrange = strip.Color(255, 90, 0);
uint32_t redOrangeDark = strip.Color(255, 10, 0);
uint32_t yellow = strip.Color(255, 255, 0);

const uint32_t pastelRainbow0 = strip.Color(130, 185, 226); //178,231,254,
const uint32_t pastelRainbow1 = strip.Color(110, 46, 145); //purple
const uint32_t pastelRainbow2 = strip.Color(54, 174, 218); //teal
const uint32_t pastelRainbow3 = strip.Color(120, 212, 96); //green
const uint32_t pastelRainbow4 = strip.Color(255, 254, 188); //yellow
const uint32_t pastelRainbow5 = strip.Color(236, 116, 70); //orange
const uint32_t pastelRainbow6 = strip.Color(229, 61, 84); //pink red

uint32_t pallet[] = {white};

uint32_t firePallet[4] = { strip.Color(30, 0, 0), redOrangeDark, redOrange, yellow};
uint32_t firePallet2[4] = {strip.Color(10, 1, 30), strip.Color(123, 7, 197), strip.Color(225, 0, 127), strip.Color(238, 130, 238)};

uint32_t pastelRainbowPallet[] = { pastelRainbow0, pastelRainbow1 , pastelRainbow2, pastelRainbow3, pastelRainbow4, pastelRainbow5, pastelRainbow6 };
byte pastelRainbowPattern[] = {  6, 1, 2, 5, 4, 3, 0 };

//pattern/pallet for twinkle effect
byte cyclePattern[] = {0, 1};
uint32_t cyclePallet[] = {0, strip.randColor()};

//pallet for random colors (length sets number of colors used by randomColors effect)
uint32_t tempRandPallet[5];

//========================================================================
//segments setup
const PROGMEM segmentSection sec0[] = {{0, 82}}; //first sword edge, 82 leds
Segment segment0 = { SIZE(sec0), sec0, true }; //numSections, section array pointer

const PROGMEM segmentSection sec1[] = {{82, 82}}; //second sword edge, 82 leds, facing opposite direction from first edge
Segment segment1 = { SIZE(sec1), sec1, false}; //numSections, section array pointer

Segment *columbs_arr[] = { &segment0 , &segment1};
SegmentSet columbs = { SIZE(columbs_arr), columbs_arr };
//======================================================================

//=====================================================================
//                              SETUP
//=====================================================================
void setup() {
  //read and set pattern/brightness values from EEPROM
  if (EEPROM_MODE_ENABLE) {
    if (BRIGHTNESS_MODE_ENABLE) {
      brightness = EEPROM.read(BRIGHTNESS_ADDR);
    }
    patternCounter = EEPROM.read(PATTERN_COUNT_ADDR);
  }

  //start a timer for writing to EEPROM
  //we stop it right away, as we have nothing new to write to EEPROM initially
  Timer1.initialize(EEPROM_COM_TIME);
  Timer1.attachInterrupt(writeEEPROM);
  Timer1.stop();

  //setup the button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  //due to how my pixelStrip lib is written, button presses must be captured using interrupts
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonHandle, FALLING);

  //initialize the led strip
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();

  //Serial.begin(9600);
  //create a random seed and populate the random pallet
  randomSeed(analogRead(0));
  strip.genRandPallet( tempRandPallet, SIZE(tempRandPallet) );
}

//note that effects are changed using double presses, and a restarted with a single press
//effects are not commented, as describing them is hard
//to remove an effect, change its case value to 999
void loop() {
  direct = !direct; //reverse the effect direction
  strip.stripOff();
  //if we're in brightness mode, we'll hop to the brightness adjust mode,
  //otherwise, run an effect
  if (setBrightnessMode && BRIGHTNESS_MODE_ENABLE) {
    adjustBrighness();
  } else {
    switch (patternCounter) {
      case 0:
        cyclePallet[1] = RC();
        cyclePallet[0] = strip.desaturate(cyclePallet[1], 30);
        strip.crossFadeCycle(cyclePattern, 2, cyclePallet, 300, 100, 10);
        break;
      case 1:
        strip.colorSpinRainbow(columbs, direct, 500, 40);
        break;
      case 2:
        strip.setRainbowOffsetCycle(40, false);
        strip.runRainbowOffsetCycle(true);
        strip.colorSpinSimple( columbs, 1, 0, 0, 20, 2, 20, 0, 2, 400, 50 ); //rainbow half
        break;
      case 3:
        strip.setBrightness(brightness + 60);
        strip.fireworksRand( 1, -1, 0, 2, 1, 40, 60);
        strip.setBrightness(brightness);
        break;
      case 4:
        strip.genRandPallet( tempRandPallet, SIZE(tempRandPallet) );
        strip.twinkleSet(0, tempRandPallet, SIZE(tempRandPallet), 2, 30, 20, 10000);
        break;
      case 5:
        strip.simpleStreamerRand( 3, 0, 5, 4, 0, direct, 24 * 5, 60);
        break;
      case 6:
        strip.segGradientCycleRand(columbs, 3, 19, 400, direct, 2, 40);
        break;
      case 7:
        //strip.stripOff();
        strip.doFireV2Seg(columbs, firePallet, SIZE(firePallet), 35, 120, false, 2000, 2); //turning on blend makes it run sloooow
        //strip.doFireV2(firePallet, SIZE(firePallet), 25, 120, 82, true, true, true, false, 400, 10);
        break;
      case 8:
        strip.colorSpinSimple( columbs, 3, 0, 0, 5, -1, 5, 0, 1, 24 * 5, 50);
        break;
      case 9:
        strip.setRainbowOffsetCycle(20, true);
        strip.runRainbowOffsetCycle(true);
        strip.colorSpinSimple( columbs, 1, white, -1, 1, -1, 4, 0, 1, 24 * 5, 50); //white dots on rainbow bg
        break;
      case 10:
        strip.fire(RC(), true, RC(), 5, 30, 60, 50, 10000);
        //strip.colorSpinSimple( columbs, 1, 0, 0, 42, 1, 0, 0, 2, 410, 30 );
        break;
      case 11:
        strip.gradientCycleRand( 5, 16, 300, direct, 60);
        break;
      case 12:
        strip.shooterSeg( columbs, pallet, 4, 0, true, 1, 12, 0, 0, 4, true, false, 30, 500);
        break;
      case 13:
        strip.setRainbowOffsetCycle(40, true);
        strip.runRainbowOffsetCycle(true);
        strip.shooterSeg( columbs, pallet, 1, -1, true, 1, 12, 0, 0, 1, true, false, 30, 500); //rainbow Bg, white shooters
        break;
      case 14:
        strip.shooterSeg( columbs, pallet, 5, 0, true, 2, 5, 1, 5, 2, true, false, 20, 500);
        break;
      case 15:
        strip.shooterSeg( columbs, pallet, 5, 0, true, 2, 5, 3, 0, 4, true, false, 30, 500);
        break;
      case 16:
        strip.solidRainbowCycle(20, 3);
        break;
      case 17:
        //strip.simpleStreamer( pastelRainbowPattern, SIZE(pastelRainbowPattern), pastelRainbowPallet, 3, 0, 0, true, 24 * 5, 70);
        strip.segGradientCycleSweep(columbs, pastelRainbowPattern, SIZE(pastelRainbowPattern), pastelRainbowPallet, SIZE( pastelRainbowPallet ), 10, 100, false, 65);
        break;
      case 18:
        strip.patternSweepRepeatRand(3, 0, 0, 2, 5, false, true, 0, 0, 1, 1, 220 );
        break;
      case 19:
        strip.genRandPallet( tempRandPallet, SIZE(tempRandPallet) );
        strip.patternSweepSetRand( 10, tempRandPallet,  SIZE(tempRandPallet), 0, 1, 8, true, 0, 1, 10, 120);
        //strip.patternSweepRand( 10, -1, 0, 1, 8, true, 0, 1, 10, 24 * 8 );
        break;
      case 20:
        strip.rainbowCycle(20, 3);
        break;
      case 21:
        strip.patternSweepRainbowRand( 5, 0, 1, 15, direct, 1, 10, 320);
        break;
      case 22:
        strip.sonarSpinRand( columbs, 1, 0, 82, 0, true, false, false, false, 345, 5); //30,52
        break;
      case 23:
        strip.sonarSpinRand( columbs, 4, 0, 20, 21, false, true, false, true, 300, 7);
        break;
        break;
      case 24:
        strip.sonarSpinRand( columbs, 1, 0, 30, 52, true, false, true, false, 340, 20); //30,52
        break;
      default:
        patternCounter = (patternCounter + 1) % NUM_EFFECTS;
        break;
    }
  }
}

//a shorthand function for getting random colors
uint32_t RC() {
  return strip.randColor();
}

//handles both short and double presses
//double presses switch to the next pattern, while short presses restart the current pattern
//counts button presses using a counter
//if the button is pressed multiple times within DOUBLE_PRESS_INTERVAL, the counter is incremented
//otherwise, the counter is reset to 0
void buttonHandle() {
  //record to current time, so we can check subsequent presses
  interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > DEBOUNCE_TIME) { // ignores interupts for x milliseconds to debounce

    //if the last press was within DOUBLE_PRESS_INTERVAL milliseconds of a previous one, we've had a multipress, increment the counter
    //otherwise, reset the counter
    if (interrupt_time - last_interrupt_time < DOUBLE_PRESS_INTERVAL) {
      buttonPressCount++;
    } else {
      buttonPressCount = 0;
    }
    last_interrupt_time = interrupt_time;
    //call function to determine what action to take based on the button count
    handlePresses();
  }
}

//does actions based on the number of button presses
//single press restarts the current effect
//double press advances to the next effect
//triple press goes into brightness mode (single press releases it)
void handlePresses() {
  strip.pixelStripStopPattern = true;
  if (setBrightnessMode && BRIGHTNESS_MODE_ENABLE) {
    //if we're in brightness mode, any button press brings us out
    setBrightnessMode = false;
    Timer1.resume(); //resume the timer to write the new brightness value to EEPROM
  } else if (buttonPressCount == 1) {
    //if the button has been double pressed, increment the pattern counter
    //(and reset the rainbow offsets and stop any active rainbow cycles)
    strip.runRainbowOffsetCycle(false);
    strip.setRainbowOffset(0);
    patternCounter++;
    Timer1.resume(); //resume the timer to write the new pattern value to EEPROM
  } else if (buttonPressCount == 2) {
    //if the button is triple pressed, we want to jump into setting the brightness
    setBrightnessMode = true;
  }
}

//mode for setting the brightness,
//sets the sword to solid blue, and changes it's brightness based on the potentiometer readings
void adjustBrighness() {
  potReading = analogRead(POT_PIN);
  brightness = map(potReading, 0, 1023, 10, 240); //map the pot readings to fall from 10-240 (read brightness range 0-255)
  strip.setBrightness(brightness);
  strip.fillStrip(blue, true);
}

//update the values in EEPROM, if enabled
//then stop the timer so we only update them once
void writeEEPROM() {
  if (EEPROM_MODE_ENABLE) {
    EEPROM.update(PATTERN_COUNT_ADDR, patternCounter);
    if (BRIGHTNESS_MODE_ENABLE) {
      EEPROM.update(BRIGHTNESS_ADDR, brightness);
    }
    Timer1.stop();
  }
}
