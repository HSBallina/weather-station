#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "libs/network.h"
#include "secrets.h"

Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;
String version = "20240521b";

struct readings {
  float temperature, humidity, pressure;
};

struct sensor {
  Adafruit_BME280 sensor;
  String name;
  uint8_t address;
};

sensor outdoor = {bmeOut, "outdoor", 0x77};
sensor indoor = {bmeIn, "indoor", 0x76};

void setup() {
  Serial.begin(115200);

  while(!Serial) {
    delay(100);
  }

  Serial.println();
  Serial.println("Weather Station " + version);
  Serial.println();

  if(!connectWifi(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("Failed to connect to WiFi.");
    while(1);
  }

  Wire.setPins(2, 1); // SDA, SCL

  initSensor(outdoor);
  initSensor(indoor);
}

void loop() {
  delay(10000);
  dumpReadings(getReadings(outdoor));
  delay(10000);
  dumpReadings(getReadings(indoor));
}

void initSensor(sensor &s) {
  Serial.print("Initializing " + s.name);

  if(!s.sensor.begin(s.address)) {
    Serial.println(" failed. Please check wiring and I2C address.");
    while(1);
  }

  Serial.println(" done.");
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