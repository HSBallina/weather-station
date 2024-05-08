# Weather Station
The basic idea is to have two BME280 sensors (indoors and outdoors) reading temperature and humidity, plotting curves to a display. In a longer perspective also collecting historical data using Azure IoT Hub.

## Hardware
- ESP32 with WiFi
- Display, either separate or combo with dev board (TBD)
- 2 BME280 sensors
- TBA

## Software
The following libraries:
- Wire
- Adafruit_BME280
- Adafruit_Sensor
- TFT_eSPI (probably)
