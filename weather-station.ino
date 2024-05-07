#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

struct readings
{
  float temperature, humidity, pressure, altitude;
};

void setup()
{
  Serial.begin(115200);
  delay(100);

  Wire.setPins(2, 1);
  bme.begin(0x76);

}

void loop()
{
  delay(5000);
  dumpReadings(getReadings());
}

readings getReadings() {
    Serial.println("-- Reading BME280 --");
    readings r = {bme.readTemperature(), bme.readHumidity(), bme.readPressure() / 100.0F, bme.readAltitude(SEALEVELPRESSURE_HPA)};
    return r;
}

void dumpReadings(readings r) {
  Serial.print("Temperature = ");
  Serial.print(r.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(r.pressure);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(r.altitude);
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(r.humidity);
  Serial.println(" %");

  Serial.println();
}