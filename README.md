# Weather Station
The basic idea is to have a number of BME280 sensors reading temperature, pressure and relative humidity, plotting curves to a display. In a longer perspective also collecting historical data using Azure IoT Hub.

## Hardware
- ESP32 with WiFi
- Display, either separate or combo with dev board (TBD)
- BME280 sensors
- TBA

## Software
The project depends on a number of built-in and external libraries. Notably:
- [Adafruit_BME280 & Adafruit_Sensor](https://github.com/adafruit)
- [Azure SDK for C Arduino library](https://github.com/Azure/azure-sdk-for-c-arduino)
- [AzureIoTHubMQTTClient](https://github.com/andriyadi/AzureIoTHubMQTTClient)

## Legal stuff
This software is provided 'as-is', without any express or implied warranty. In no event will the authors or contributors be held liable for any damages arising from the use of this software. It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Use it at your own risk.

## License
Weather Station is licensed under [MIT](https://github.com/HSBallina/weather-station/blob/midflight-cleanup/LICENSE) license.
Part of the code is inspired by and/or borrows from the [Azure SDK for C Arduino library](https://github.com/Azure/azure-sdk-for-c-arduino). 
