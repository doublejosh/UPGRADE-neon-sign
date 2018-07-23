/**
 * Control RGB LEDs with spectrum analyzer sheild (SparkFun).
 *
 * Original: Ben Moyes, 2010
 * Updated: Josh Lind, 2018
 */

#include <Adafruit_NeoPixel.h>

#define LEDS_BANDS 7     // Number of pixels and spectrum bands.
#define THREASHOLD 3     // Cuts off low readings.
#define DIVISOR_FLOOR 64 // Lowest analog reading will be devided by, for sensitivity adjustment
#define ADJUST_WINDOW 10 // How many cycles to delay adjusting sensitivity.
#define LOOP_DELAY 15    // Milliseconds to wait for each cycle.
#define QUIET_WINDOW 50  // Cycles to wait to consider sound off (mode switching).

#define LED_PIN 8

// Spectrum analyzer shield pins.
// You can move pins 4 and 5, but you must cut the
// trace on the shield and re-route from the 2 jumpers. 
#define SPECTRUM_RESET 5
#define SPECTRUM_STROBE 4
// 0 for left channel, 1 for right.
#define SPECTRUM_ANALOG 0

// uint8_t mode = 0;

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LEDS_BANDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t offColor = ledStrip.Color(0, 0, 0);
uint32_t baseColor = ledStrip.Color(0, 255, 0);
uint32_t triggerColor = ledStrip.Color(255, 145, 0);
uint32_t colors[] = {
           ledStrip.Color(0, 255, 0),
           ledStrip.Color(150, 255, 0),
           ledStrip.Color(255, 255, 0),
           ledStrip.Color(255, 0, 0),
           ledStrip.Color(255, 0, 0),
           //ledStrip.Color(255, 0, 255),
           //ledStrip.Color(0, 0, 255)
           //ledStrip.Color(0, 255, 255)
         };

unsigned int mode = 1;

// Spectrum analyzer read values.
int spectrum[LEDS_BANDS];

// Adjust the spectrum read divisor if levels are too high/low.
unsigned int divisor = 80;
unsigned int adjustTimer = 0;
unsigned int quietTimer = 0;

void setup() {
  ledStrip.begin();
  ledStrip.show();

  // Setup pins to drive the spectrum analyzer. 
  pinMode(SPECTRUM_RESET, OUTPUT);
  pinMode(SPECTRUM_STROBE, OUTPUT);

  // Init spectrum analyzer.
  digitalWrite(SPECTRUM_STROBE, LOW);
  delay(1);
  digitalWrite(SPECTRUM_RESET, HIGH);
  delay(1);
  digitalWrite(SPECTRUM_STROBE, HIGH);
  delay(1);
  digitalWrite(SPECTRUM_STROBE, LOW);
  delay(1);
  digitalWrite(SPECTRUM_RESET, LOW);
  delay(5);
  // Reading the analyzer now will read the lowest frequency.

  Serial.begin(115200);
}

void loop() {
  showSpectrum();
  ledStrip.show();
  delay(LOOP_DELAY);
}

/**
 * Read 7 band equalizer input.
 */
void readSpectrum() {
  for (unsigned int band=0; band<LEDS_BANDS; band++) {
    // Read twice and take the average by dividing by 2.
    spectrum[band] = (analogRead(SPECTRUM_ANALOG) + analogRead(SPECTRUM_ANALOG)) >> 1;
    // Communicate with the sheld.
    digitalWrite(SPECTRUM_STROBE, HIGH);
    digitalWrite(SPECTRUM_STROBE, LOW);     
  }
}

/**
 * Visualize the spectrum data.
 *
 * Whole thing is a level meter.
 *
 * @todo Second mode where each band is a pixel color.
 */
void showSpectrum() {
  unsigned int reading, lowMax, currentMax = 0;

  readSpectrum();

  // Graph all output LEDs with corresponding band.
  for (unsigned int band=0; band<LEDS_BANDS; band++) {
    // Bands are read in as 10 bit values, scale them down.
    reading = spectrum[band] / divisor;
    // Track the largest band reading.
    if (reading > currentMax) currentMax = reading;

    // Spectrum bands.
    if (mode == 1) {
      if ((reading >= THREASHOLD) && (reading == currentMax)) ledStrip.setPixelColor(band, colors[reading-THREASHOLD]);
      else ledStrip.setPixelColor(band, offColor);
    }

    Serial.print(reading);
    Serial.print(" - ");
  }

  for (unsigned int band=0; band<LEDS_BANDS; band++) {
    // Spectrum bands, no input detected.
    if (mode == 1) {
      if ((currentMax) < THREASHOLD && (quietTimer >= QUIET_WINDOW)) ledStrip.setPixelColor(band, baseColor);
    }

    // Level meter.
    if (mode == 2) {
      // Just listen for bass.
      if ( (reading > lowMax)) lowMax = reading;
      // Mark the pixel of the current max value.
      if ((currentMax != 0) && (currentMax == band)) ledStrip.setPixelColor(band-1, triggerColor);
      else ledStrip.setPixelColor(band, baseColor);
    }
  }

  // Allow for time window to consider sound break, but keep it within bounds.
  if (currentMax < THREASHOLD) {
    if (quietTimer <= QUIET_WINDOW) quietTimer++;
  }
  else quietTimer = 0;

  if (quietTimer == QUIET_WINDOW) {
    if (mode == 1) mode = 2;
    else mode = 1;
  }

  Serial.print(" -- ");
  Serial.print(quietTimer);

  Serial.print(" --- ");
  Serial.println(currentMax);

  // Reduce sensitivity if levels are too high, divided reading is larger than pixel range.
  if (currentMax >= 8) {
    divisor++;
    adjustTimer = 0;
  }
  // Increase sensitivity if time window has passed.
  else if ((currentMax < 4) && (divisor > DIVISOR_FLOOR) && (adjustTimer++ >= ADJUST_WINDOW)) {
    divisor--;
    adjustTimer = 0;
  }
}

/**
 * Create a color from a 0-255 integer.
 */
// uint32_t colorWheel(byte value) {
//   value = 255 - value;
//   if(value < 85) {
//     return ledStrip.Color(255 - value * 3, 0, value * 3);
//   }
//   if(value < 170) {
//     value -= 85;
//     return ledStrip.Color(0, value * 3, 255 - value * 3);
//   }
//   value -= 170;
//   return ledStrip.Color(value * 3, 255 - value * 3, 0);
// }
