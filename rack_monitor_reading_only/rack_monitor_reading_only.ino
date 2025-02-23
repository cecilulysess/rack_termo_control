#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#define DS_18B20_PIN 12  // DS18B20

#define DS_DHT11_PIN 8
#define DHT_TYPE DHT11

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
  sensors.begin();
  dht.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  DhtReading result = handleDht11(currentMillis);
  float ds18b20_result = handle18B20(currentMillis);

  if (isnan(result.temperature) && isnan(result.humidity)) {
    Serial.println("Wait for thermister reading...");
  } else {
    Serial.print("Temperature: D12: ");
    Serial.print(ds18b20_result);
    Serial.print("°C, D8: ");
    Serial.print(result.temperature);
    Serial.print("°C; Humidity: ");
    Serial.print(result.humidity);
    Serial.println("%");
  }
  delay(2000);
}

// Read data every `currentMillis`
float handle18B20(unsigned long currentMillis) {

  if (currentMillis - last18B20Time >= tempReadingInterval) {
    last18B20Time= currentMillis;
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);  // Get temperature in Celsius
    return temperatureC;
  }
  return NAN;

  // Serial.print("Temperature: ");
  // Serial.print(temperatureC);
  // Serial.println(" °C");
}

DhtReading handleDht11(unsigned long currentMillis) {
  DhtReading result = { NAN, NAN };

  if (currentMillis - lastDhtTime >= tempReadingInterval) {
    lastDhtTime = currentMillis;                // Update the last time for the DHT sensor
    float humidity = dht.readHumidity();        // Read humidity
    float temperature = dht.readTemperature();  // Read temperature

    // Check if readings failed and exit early (optional)
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read DHT sensor!");
      return result;
    }

    result.temperature = temperature;
    result.humidity = humidity;

    // // Print the DHT sensor readings
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.print("%  Temperature: ");
    // Serial.print(temperature);
    // Serial.println(" °C");
  }
  return result;
}