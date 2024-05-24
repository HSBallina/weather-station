#ifndef SATATUSLED_H
#define SATATUSLED_H

#include <Adafruit_NeoPixel.h>

enum Status_t
{
  STATUS_CONNECTING_WIFI,
  STATUS_CONNECTING_AZURE,
  STATUS_SETTING_TIME,
  STATUS_WIFI_ERROR,
  STATUS_AZURE_ERROR,
  STATUS_TIME_ERROR,
  STATUS_OK,
  STATUS_ERROR
};

#define LED_PIN 8
#define LED_COUNT 1
Adafruit_NeoPixel pxl(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void displayStatus(Status_t status);


#endif