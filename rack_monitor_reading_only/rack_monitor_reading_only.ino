#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

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
}

void loop() {
  unsigned long currentMillis = millis();
  DhtReading result = handleDht11(currentMillis);
  float ds18b20_result = handle18B20(currentMillis);

  if (isnan(result.temperature) && isnan(result.humidity)) {
    printLine("Wait for thermister reading...");
  } else {
    printLine(formatSensorPrintText(ds18b20_result, result));
  }
  delay(2000);
}

String formatSensorPrintText(float ds18b20_read, DhtReading dht_read) {
  return "Temperature:\n\tD12: " + String(ds18b20_read) + "C\n\tD8 : " + 
    String(dht_read.temperature) + "C\nHumidity: " + String(dht_read.humidity) + "%";
}

void printLine(String text) {
  Serial.println(text);    // Print to Serial Monitor
  display.clearDisplay();  // Clear previous text
  display.setCursor(0, 0);
  display.println(text);  // Print to OLED
  display.display();
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