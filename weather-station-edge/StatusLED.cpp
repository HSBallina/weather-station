#include "StatusLED.h"

#define LED_BRIGHTNESS 30

void displayStatus(Status_t status)
{
  int blink = 0;
  bool isError = false;

  pxl.clear();

  switch (status)
  {
  case STATUS_CONNECTING_WIFI:
    pxl.setPixelColor(0, pxl.Color(0, 0, 255));
    break;
  case STATUS_CONNECTING_AZURE:
    pxl.setPixelColor(0, pxl.Color(0, 255, 0));
    break;
  case STATUS_SETTING_TIME:
    pxl.setPixelColor(0, pxl.Color(0, 255, 255));
    break;
  case STATUS_WIFI_ERROR:
    pxl.setPixelColor(0, pxl.Color(255, 0, 0));
    blink = 1;
    isError = true;
    break;
  case STATUS_AZURE_ERROR:
    pxl.setPixelColor(0, pxl.Color(255, 0, 0));
    blink = 2;
    isError = true;
    break;
  case STATUS_TIME_ERROR:
    pxl.setPixelColor(0, pxl.Color(255, 0, 0));
    blink = 3;
    isError = true;
    break;
  case STATUS_OK:
    pxl.setPixelColor(0, pxl.Color(0, 255, 0));
    break;
  case STATUS_ERROR:
    pxl.setPixelColor(0, pxl.Color(255, 0, 0));
    blink = 5;
    isError = true;
    break;
  default:
    break;
  }

  pxl.setBrightness(LED_BRIGHTNESS);
  pxl.show();
}
