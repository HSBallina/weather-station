#include <Adafruit_BME280.h>
#include "WeatherData.h"

Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;

sensor outdoor = {bmeOut, "outdoor", 0x77};
sensor indoor = {bmeIn, "indoor", 0x76};

readings getReadings(sensor &s)
{
  return {s.sensor.readTemperature(), s.sensor.readHumidity(), s.sensor.readPressure() / 100.0F};
}