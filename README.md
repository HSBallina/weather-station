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

## Legal stuff
This software is provided 'as-is', without any express or implied warranty. In no event will the authors or contributors be held liable for any damages arising from the use of this software. It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.

## License
Weather Station is licensed under [MIT](https://github.com/HSBallina/weather-station/blob/midflight-cleanup/LICENSE) license.
