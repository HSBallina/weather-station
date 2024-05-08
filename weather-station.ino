#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define TFT_WIDTH 480
#define TFT_HEIGHT 320

Adafruit_BME280 bmeOut;
String version = "20240508b";

struct readings {
  float temperature, humidity, pressure;
};

struct sensor {
  Adafruit_BME280 sensor;
  String name;
  uint8_t address;
};

sensor outdoor = {bmeOut, "outdoor", 0x77};

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Weather Station " + version);
  Serial.println();

  Wire.setPins(2, 1); // SDA, SCL

  initSensor(outdoor);
}

void loop() {
  dumpReadings(getReadings(outdoor));
  delay(10000);
}

void initSensor(sensor &s) {
  Serial.print("Initializing " + s.name);

  if(!s.sensor.begin(s.address)) {
    Serial.println(" failed. Please check wiring and I2C address.");
    while(1);
  }

  Serial.println(" done.");
  Serial.println();
}

readings getReadings(sensor &s) {
    Serial.println("-- Reading " + s.name + " --");
    readings r = {s.sensor.readTemperature(), s.sensor.readHumidity(), s.sensor.readPressure() / 100.0F};
    return r;
}

void dumpReadings(readings r) {
  Serial.print("Temperature = ");
  Serial.print(r.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(r.pressure);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(r.humidity);
  Serial.println(" %");

  Serial.println();
}