#include <PixelStrip.h>
#define PIN 3 //4

const uint16_t stripLength = 164;
const uint8_t stripType = NEO_GRB + NEO_KHZ800;
PixelStrip strip = PixelStrip(stripLength, PIN, stripType); //12 NEO_GRB

uint32_t red = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t white = strip.Color(255, 255, 255);
uint32_t ltOrange = strip.Color(255, 154, 0);
uint32_t redOrange = strip.Color(255, 90, 0);
uint32_t redOrangeDark = strip.Color(255, 10, 0);
uint32_t yellow = strip.Color(255, 255, 0);

const uint32_t pastelRainbow = strip.Color(130, 185, 226); //178,231,254,
const uint32_t pastelRainbow1 = strip.Color(110, 46, 145); //purple
const uint32_t pastelRainbow2 = strip.Color(54, 174, 218); //teal
const uint32_t pastelRainbow3 = strip.Color(120, 212, 96); //green
const uint32_t pastelRainbow4 = strip.Color(255, 254, 188); //yellow
const uint32_t pastelRainbow5 = strip.Color(236, 116, 70); //orange
const uint32_t pastelRainbow6 = strip.Color(229, 61, 84); //pink red

uint32_t pallet[] = {white};

uint32_t firePallet[4] = { strip.Color(30, 0, 0), redOrangeDark, redOrange, ltOrange};
uint32_t firePallet2[4] = {strip.Color(10, 1, 30), strip.Color(123, 7, 197), strip.Color(225, 0, 127), strip.Color(238, 130, 238)};

uint32_t pastelRainbowPallet[] = { pastelRainbow, pastelRainbow1 , pastelRainbow2, pastelRainbow3, pastelRainbow4, pastelRainbow5, pastelRainbow6 };
byte pastelRainbowPattern[] = {  6, 1, 2, 5, 4, 3, 0 };

//========================================================================
const PROGMEM segmentSection sec0[] = {{0, 82}};
Segment segment0 = { SIZE(sec0), sec0, true }; //numSections, section array pointer

const PROGMEM segmentSection sec1[] = {{82, 82}};
Segment segment1 = { SIZE(sec1), sec1, false}; //numSections, section array pointer

//=========================================================================

Segment *columbs_arr[] = { &segment0 , &segment1};
SegmentSet columbs = { SIZE(columbs_arr), columbs_arr };
//======================================================================

byte cyclePattern[] = {0, 1};
uint32_t cyclePallet[] = {0, strip.randColor()};

uint32_t tempRandPallet[5]; //pallet for random colors (length sets number of colors used by randomColors effect)

boolean direct = true;
boolean setBrightnessMode = false;
boolean brighnessModeEnable = true;
int brightness = 100;
int brightnessArray[] = { 30, 50, 100, 150, 220};
int brightnessIndex = 2;
int patternCounter = 7;
int buttonPressCount = 0;
int potReading = 0;
void setup() {
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), buttonHandle, FALLING);
  strip.begin();
  strip.setBrightness(brightnessArray[brightnessIndex]);

  strip.show();

  //Serial.begin(9600);
  randomSeed(analogRead(0));
  strip.genRandPallet( tempRandPallet, SIZE(tempRandPallet) );
}

void loop() {
  direct = !direct;
  strip.stripOff();
  if (setBrightnessMode && brighnessModeEnable) {
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
        strip.colorSpinSimple( columbs, 1, 0, 0, 20, 2, 20, 0, 2, 24 * 5, 50 ); //rainbow half
        break;
      case 3:
        //strip.setBrightness(brightnessArray[brightnessIndex] + 60);
        strip.setBrightness(brightness + 60);
        strip.fireworksRand( 1, -1, 0, 2, 1, 40, 60);
        strip.setBrightness(brightness);
        //strip.setBrightness(brightnessArray[brightnessIndex]);
        break;
      case 4:
        strip.genRandPallet( tempRandPallet, SIZE(tempRandPallet) );
        strip.twinkleSet(0, tempRandPallet, SIZE(tempRandPallet), 2, 30, 20, 10000);
        break;
      case 5:
        strip.simpleStreamerRand( 3, 0, 5, 4, 0, direct, 24 * 5, 60);
        break;
      case 6:
        strip.segGradientCycleRand(columbs, 3 , 14, 300, direct, 2, 40);
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
        strip.colorSpinSimple( columbs, 1, white, -1, 1, -1, 4, 0, 1, 24 * 5, 50); //white dots on rainbow bg
        break;
      case 10:
        strip.colorSpinSimple( columbs, 1, 0, 0, 42, 1, 0, 0, 2, 410, 30 );
        break;
      case 11:
        strip.gradientCycleRand( 5, 12, 24 * 7, direct, 60);
        break;
      case 12:
        strip.shooterSeg( columbs, pallet, 5, 0, 1, 12, 0, 0, 4, true, false, 30, 500);
        break;
      case 13:
        strip.shooterSeg( columbs, pallet, 1, -1, 1, 12, 0, 0, 1, true, false, 30, 500); //rainbow Bg, white shooters
        break;
      case 14:
        strip.shooterSeg( columbs, pallet, 5, 0, 2, 5, 1, 5, 2, true, false, 20, 500);
        break;
      case 15:
        strip.shooterSeg( columbs, pallet, 5, 0, 2, 5, 3, 0, 4, true, false, 30, 500);
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
        strip.patternSweepRand( 10, -1, 0, 1, 8, true, 0, 1, 10, 24 * 8 );
        break;
      case 20:
        strip.rainbowCycle(20, 3);
        break;
      case 21:
        strip.patternSweepRainbowRand( 5, 0, 1, 15, direct, 1, 10, 320);
        break;
      case 22:
        strip.sonarSpinRand( columbs, 1, 0, 30, 52, false, false, true, false, 350, 2);
        break;
      case 23:
        strip.sonarSpinRand( columbs, 4, 0, 20, 21, false, true, false, true, 300, 5);
        break;
      default:
        patternCounter = 0;
        break;
    }
  }
}

uint32_t RC() {
  return strip.randColor();
}


//handles both short and double presses
//double presses switch to the next pattern, while short presses restart the current pattern
void buttonHandle() { //interrupt with debounce
  volatile static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 100) { // ignores interupts for 100 milliseconds to debounce

    if (interrupt_time - last_interrupt_time > 600) {
      buttonPressCount = 0;
    }
    interrupt_time = millis();

    if (interrupt_time - last_interrupt_time < 400) {
      buttonPressCount++;
      //setBrightnessMode = false;
    }
  }
  last_interrupt_time = interrupt_time;
  handlePresses();

}

void handlePresses() {
  strip.pixelStripStopPattern = true;
  if (setBrightnessMode && brighnessModeEnable) {
    //brightnessIndex = (brightnessIndex + 1) % SIZE(brightnessArray);
     setBrightnessMode = false;
  } else if (buttonPressCount == 1) {
    patternCounter++;
  } else if (buttonPressCount == 2) {
    setBrightnessMode = true;
  }
}

void adjustBrighness() {
  potReading = analogRead(A5);
  brightness = map(potReading, 0, 1023, 20, 240);
  //strip.setBrightness(brightnessArray[brightnessIndex]);
  strip.setBrightness(brightness);
  strip.fillStrip(blue, true);
}
