#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;
String version = "20240515a";

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
  delay(1000); // Give time to open serial monitor
  Serial.println();
  Serial.println("Weather Station " + version);
  Serial.println();

  Wire.setPins(2, 1); // SDA, SCL

  initSensor(outdoor);
  initSensor(indoor);
}

void loop() {
  dumpReadings(getReadings(outdoor));
  delay(4000);
  dumpReadings(getReadings(indoor));
  delay(4000);
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