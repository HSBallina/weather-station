#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

#define TFT_WIDTH 480
#define TFT_HEIGHT 320

Adafruit_BME280 bme;

struct readings {
  float temperature, humidity, pressure;
};

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.print("BME280 init ");
  Wire.setPins(2, 1);
  if(!bme.begin(0x76)) {
    Serial.println("failed. Please check your wiring and I2C address");
    while(1);
  }

  Serial.print("succeeded. Sensor id: ");
  Serial.print(bme.sensorID());
  Serial.println(". Done.");

}

void loop() {
  delay(10000);
  dumpReadings(getReadings());
}

readings getReadings() {
    Serial.println("-- Reading BME280 --");
    readings r = {bme.readTemperature(), bme.readHumidity(), bme.readPressure() / 100.0F};
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