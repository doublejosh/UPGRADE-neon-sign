/**
 * Control RGB LEDs with spectrum analyzer sheild (SparkFun).
 *
 * Original: Ben Moyes, 2010
 * Updated: Josh Lind, 2018
 */

#include <Adafruit_NeoPixel.h>

#define LEDS_BANDS 7
#define LED_PIN 6
#define LOOP_DELAY 15

// Spectrum analyzer shield pins.
// You can move pins 4 and 5, but you must cut the
// trace on the shield and re-route from the 2 jumpers. 
#define SPECTRUM_RESET 5
#define SPECTRUM_STROBE 4
// 0 for left channel, 1 for right.
#define SPECTRUM_ANALOG 0

Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(LEDS_BANDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t baseColor = ledStrip.Color(0, 255, 0);
uint32_t triggerColor = ledStrip.Color(255, 165, 0);

// Spectrum analyzer read values.
int spectrum[LEDS_BANDS];

// Adjust the spectrum read divisor if levels are too high/low.
unsigned int divisor = 80;
unsigned int adjustTimer = 0;
unsigned int adjustWindow = 20;

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
  unsigned int reading, currentMax = 0;

  readSpectrum();

  // Graph all output LEDs with corresponding band.
  for (unsigned int band = 0; band<LEDS_BANDS; band++) {
    // Bands are read in as 10 bit values, scale them down.
    reading = spectrum[band] / divisor;
    // Track the largest band reading.
    if (reading > currentMax) currentMax = reading;
    // Set LED pixel.
    if (band <= reading) ledStrip.setPixelColor(band, baseColor);
    else ledStrip.setPixelColor(band, triggerColor);
  }

  // Reduce sensitivity if levels are too high.
  if (currentMax >= LEDS_BANDS) {
    divisor++;
    adjustTimer = 0;
  }
  // Increase sensitivity if time window has passed.
  else if ((currentMax < LEDS_BANDS) && (adjustTimer++ >= adjustWindow)) {
    divisor--;
    adjustTimer = 0;
  }
}
