#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// Define PWM IO pins
#define TIMER_2_CONTROLLED_PIN 3
#define PWM_TOP_FAN_RPM_PIN 4
#define PWM_BUTTOM_FAN_RPM_PIN 2

// Define the specific screen we have
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

#define DS_18B20_PIN 12  // DS18B20

#define DS_DHT11_PIN 8
#define DHT_TYPE DHT11

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct DhtReading {
  float temperature;
  float humidity;
};

OneWire oneWire(DS_18B20_PIN);
DallasTemperature sensors(&oneWire);

DHT dht(DS_DHT11_PIN, DHT_TYPE);
// Timing for DHT11
unsigned long lastDhtTime = 0, last18B20Time = 0;

const long tempReadingInterval = 2000;  // update every 2s

void setup() {
  Serial.begin(9600);
  // Initialize Screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    printLine("SSD1306 allocation failed");
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();

  // Initialize sensors
  sensors.begin();
  dht.begin();

  // Initialize PWM control
  setupTimer2ForFanControl();
}

void loop() {
  unsigned long currentMillis = millis();
  DhtReading result = handleDht11(currentMillis);
  float ds18b20_result = handle18B20(currentMillis);

  if (isnan(result.temperature) && isnan(result.humidity)) {
    printLines("Wait for thermister reading...", getTimer2PWMDutyCycle());
  } else {
    printLines(formatSensorPrintText(ds18b20_result, result), getTimer2PWMDutyCycle());
  }
  delay(2000);
}

String formatSensorPrintText(float ds18b20_read, DhtReading dht_read) {
  return "D12: " + String(ds18b20_read, 1) + "C D8: " + String(dht_read.temperature, 1) + "C\nHumidity:  " + String(dht_read.humidity, 1) + "%";
}

void printLines(const char* text, const char* text2) {
  Serial.println(text);    // Print to Serial Monitor
  display.clearDisplay();  // Clear previous text
  display.setCursor(0, 0);
  display.println(text);
  if (text2) display.println(text2);
  display.display();
}

void printLine(String text) {
  printLines(text.c_str(), NULL);
}

void printLines(String text, String text2) {
  printLines(text.c_str(), text2.c_str());
}

// Read data every `currentMillis`
float handle18B20(unsigned long currentMillis) {

  if (currentMillis - last18B20Time >= tempReadingInterval) {
    last18B20Time = currentMillis;
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);  // Get temperature in Celsius
    return temperatureC;
  }
  return NAN;
}

DhtReading handleDht11(unsigned long currentMillis) {
  DhtReading result = { NAN, NAN };

  if (currentMillis - lastDhtTime >= tempReadingInterval) {
    lastDhtTime = currentMillis;                // Update the last time for the DHT sensor
    float humidity = dht.readHumidity();        // Read humidity
    float temperature = dht.readTemperature();  // Read temperature

    // Check if readings failed and exit early (optional)
    if (isnan(humidity) || isnan(temperature)) {
      printLine("Failed to read DHT sensor!");
      return result;
    }

    result.temperature = temperature;
    result.humidity = humidity;
  }
  return result;
}

// Set timer 2 to use pin 3 output at 25kHz PWM using Fast PWM mode.
// The setting override pin 11 so that it cannot be used independently.
// The output value will be 0-79 for duty cycle control 0-100%
void setupTimer2ForFanControl() {
  pinMode(TIMER_2_CONTROLLED_PIN, OUTPUT);

  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);  // Fast PWM, non-inverting mode
  TCCR2B = _BV(WGM22) | _BV(CS21);                 // Prescaler = 8
  OCR2A = 79;                                      // Set TOP value for 25 kHz
  OCR2B = 39;                                      // 50% duty cycle as default
}

// @param duty_cycle: any value between 20 - 100
// if < 20 provided, it write 20, if > 100 provided it write 100
void writeTimer2PWMOutput(int duty_cycle) {
  int value_to_write = duty_cycle < 20 ? 20 : duty_cycle;
  value_to_write = duty_cycle > 100 ? 100 : duty_cycle;
  // OCR2B is used to control the duty cycle by:
  // D = OCR2B / (OCR2A + 1) * 100,
  // But the value measure is slightly off, so I use 79 as upper bound
  OCR2B = ceil((value_to_write / 100.0) * 79);
}

// Return the current duty cycle value in % string.
String getTimer2PWMDutyCycle() {
  return "Fan Speed: " + String(OCR2B / 80.0 * 100.0, 1) + "%";
}