#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <Adafruit_BME280.h>

struct readings
{
  float temperature, humidity, pressure;
};

struct sensor
{
    Adafruit_BME280 sensor;
    String name;
    uint8_t address;
};

readings getReadings(sensor &s);

#endif // WEATHERDATA_H