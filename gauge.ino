#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN 6
#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif



// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);


uint8_t brightness = 32;    // Global brightness, 0-255

int16_t hue = 680;          // initial hue 0-1535 (96*16)
uint8_t  width = 16;        // initial pulse width
float   tail = 1.7;         // initial length of tail
float   chroma = 0;         // initial chroma value for tail
int8_t  rate = 2;           // initial spin rate
uint8_t lastPeak = 0;       // storage for ring position

uint32_t iColor[16];        // Background colors

uint8_t colorCorrect(uint8_t value);
uint8_t colorCorrect(uint8_t value, uint8_t br);

int widthPin  = A0;         // Controls width of pulse
int tailPin   = A1;         // Controls length of fade
int huePin    = A2;         // Controls hue of gauge
int chromaPin = A3;         // Controls color of fade
int ratePin   = A4;         // Controls rate/direction of spin
int levelPin  = A5;         // Controls brightness level

  
void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  /*
  hue = getHue();
  width = getWidth();
  tail = getTail();
  rate = getRate();
  */
  //testdrawchar();
  //delay(100000);
  brightness = getLevel();
  colorSwirlIn(hue, width, 4, rate);
  width = getWidth();
  tail  = getTail();
  hue = getHue();
  chroma = getChroma();
  brightness = getLevel();
  rate = getRate();
  showData();
  
  
}

void testdrawchar(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  
  for (uint8_t i=0; i < 255; i++) {
    if (i == '\n') continue;
    display.write(i);
    if ((i > 0) && (i % 16 == 0))
      display.println();
    if ((i > 0) && (i % 128 == 0)) {
      delay(10000);
      display.clearDisplay();
      display.setCursor(0,0);
    }
        
  }    
  display.display();
}

void showData(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0); 
  display.print("Width:");
  display.print(width);
  display.setCursor(64,0);
  display.print("Decay:");
  display.print(tail);
  display.setCursor(0,8); 
  display.print("Rate:");
  display.print(rate);
  display.setCursor(64,8);
  display.print("Level:");
  display.print(brightness);  
  
  display.setCursor(0,16);  
  display.print("Hue:");
  display.print(hue);
  display.setCursor(0,24);
  display.print("Chroma:");
  display.println(chroma);    
  display.display();
}  


uint8_t getWidth() {
  return byte(analogRead(widthPin) / 4);
} 

float getTail() {
  return analogRead(tailPin) / 256.0;
} 

uint16_t getHue() {
  return 1.5 * analogRead(huePin);
} 

float getChroma() {
  return (analogRead(chromaPin) / 16.0) - 32.0;
} 

int8_t getRate() {
  return byte((1024 - analogRead(ratePin)) / 4.0) - 128;
} 

uint8_t getLevel() {
  return uint8_t(analogRead(levelPin) / 4.0);
} 


int32_t hue2color(int16_t hue) {
  int16_t a = hue;
  uint8_t r,g,b;
  switch((hue >> 8) % 6) {
    case 0: r = 255; g =   a; b =   0; break;
    case 1: r =  ~a; g = 255; b =   0; break;
    case 2: r =   0; g = 255; b =   a; break;
    case 3: r =   0; g =  ~a; b = 255; break;
    case 4: r =   a; g =   0; b = 255; break;
    case 5: r = 255; g =   0; b =  ~a; break;
  } 
  return strip.Color(r,g,b);
}
  

void colorSwirlIn(int16_t initHue, uint8_t wide, float decay, int8_t spinRate) {
    uint8_t initBrightness = brightness;
    int16_t a = initHue,distance;
    uint8_t r,g,b,level,peak;
    uint32_t c;
    float chromic = 0.0;
    uint8_t abortCount = 0;
 
    switch((initHue >> 8) % 6) {
     case 0: r = 255; g =   a; b =   0; break;
     case 1: r =  ~a; g = 255; b =   0; break;
     case 2: r =   0; g = 255; b =   a; break;
     case 3: r =   0; g =  ~a; b = 255; break;
     case 4: r =   a; g =   0; b = 255; break;
     case 5: r = 255; g =   0; b =  ~a; break;
    }
  peak = 0;
  
  for(peak=lastPeak; peak<256; peak+=spinRate) {
    if (abortCount >= 4) {
      lastPeak = peak;
      return;
    }
    spinRate = getRate();
    wide = getWidth();
    wide = max((wide / 2) - 8, 0);

    chromic = getChroma();
    decay = getTail();
    
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      distance = peak - (i * 256/strip.numPixels());
      if (distance <= -128) {
        distance += 256;
      }
      if (distance >= 128) {
        distance -= 256;
      }
        distance = byte(abs(distance));
        distance = max((distance - wide), 0);
      if (distance != 0)
      {
        level = min((65536/pow(distance, decay)),255);
      } 
      else
      {
        level = 255;
      }  
      initHue = getHue();
      brightness = getLevel();
      c = colorCorrect2(hue2color(uint16_t(initHue + chromic*(distance))),level);
      iColor[i]=c;
      strip.setPixelColor(i, c);
      strip.show();
    }
    abortCount++;
  } 
  brightness = initBrightness;
}


void colorFadeIn(int16_t initHue, uint8_t wait) {
    uint8_t initBrightness = brightness;
    int16_t a = initHue;
    uint8_t r,g,b,level;
    uint32_t c; 
    switch((initHue >> 8) % 6) {
     case 0: r = 255; g =   a; b =   0; break;
     case 1: r =  ~a; g = 255; b =   0; break;
     case 2: r =   0; g = 255; b =   a; break;
     case 3: r =   0; g =  ~a; b = 255; break;
     case 4: r =   a; g =   0; b = 255; break;
     case 5: r = 255; g =   0; b =  ~a; break;
    }
  brightness = 255;
  for(level=2; level<250; level+=(1+level/32)) {
    //level = 0;
    c = strip.Color(colorCorrect(r,level), colorCorrect(g,level), colorCorrect(b,level));
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      iColor[i]=c;
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
    }
  } 
  brightness = initBrightness;
}

void colorFadeOut(int16_t initHue, uint8_t wait) {
    uint8_t initBrightness = brightness;
    int16_t a = initHue;
    uint8_t r,g,b,level;
    uint32_t c; 
    switch((initHue >> 8) % 6) {
     case 0: r = 255; g =   a; b =   0; break;
     case 1: r =  ~a; g = 255; b =   0; break;
     case 2: r =   0; g = 255; b =   a; break;
     case 3: r =   0; g =  ~a; b = 255; break;
     case 4: r =   a; g =   0; b = 255; break;
     case 5: r = 255; g =   0; b =  ~a; break;
    }
  brightness = 255;
  for(level=255; level>initBrightness; --level) {
    //level = 0;
    c = strip.Color(colorCorrect(r,level), colorCorrect(g,level), colorCorrect(b,level));
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      iColor[i]=c;
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
    }
  }
  brightness = initBrightness;
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

uint8_t colorCorrect(uint8_t value)
{
  uint8_t corrected = (value * brightness) >> 8;  // set brightness
  //return pgm_read_byte(&gamma8[corrected]);   // gamma correct
  return corrected;
}

uint8_t colorCorrect(uint8_t value, uint8_t level)
{
  uint8_t corrected;
  corrected = (value * level) >> 8;  // set instance brightness
  corrected = (corrected * brightness) >> 8;  // set global brightness
  //return pgm_read_byte(&gamma8[corrected]);   // gamma correct
  return corrected;
}

uint32_t colorCorrect2(uint32_t value, uint8_t level)
{
  
  uint8_t g,r,b;
  r = value >> 16;                   // green component
  g = value - (r << 16) >> 8;        // red component
  b = value - (r << 16) - (g << 8);  //blue component
  g = (g * level) >> 8;
  r = (r * level) >> 8;
  b = (b * level) >> 8;
  g = (g * brightness) >> 8;
  r = (r * brightness) >> 8;
  b = (b * brightness) >> 8;
  //return pgm_read_byte(&gamma8[corrected]);   // gamma correct
  return strip.Color(r,g,b);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(colorCorrect(WheelPos * 3), colorCorrect(255 - WheelPos * 3), 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(colorCorrect(255 - WheelPos * 3), 0, colorCorrect(WheelPos * 3));
  } else {
   WheelPos -= 170;
   return strip.Color(0, colorCorrect(WheelPos * 3), colorCorrect(255 - WheelPos * 3));
  }
}
