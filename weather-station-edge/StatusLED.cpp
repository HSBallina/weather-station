#include <Adafruit_NeoPixel.h>
#include "StatusLED.h"

#define LED_BRIGHTNESS 30
#define LED_PIN 8
#define LED_COUNT 1
Adafruit_NeoPixel pxl(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

static void blink(int times, int delayMs, uint32_t color);
static void blink(int times, int delayMs, uint32_t color, uint32_t currentColor);

void initStatusLED()
{
    pxl.begin();
    pxl.clear();
    pxl.show();
}

void displayStatus(Status_t status)
{
    pxl.clear();
    pxl.show();

    switch (status)
    {
    case STATUS_CONNECTING_WIFI:
        pxl.setPixelColor(0, pxl.Color(0, 0, 255));
        break;
    case STATUS_CONNECTING_AZURE:
        pxl.setPixelColor(0, pxl.Color(255, 204, 31));
        break;
    case STATUS_SETTING_TIME:
        pxl.setPixelColor(0, pxl.Color(138, 84, 255));
        break;
    case STATUS_WIFI_ERROR:
        pxl.setPixelColor(0, pxl.Color(255, 0, 0));

        break;
    case STATUS_AZURE_ERROR:
        pxl.setPixelColor(0, pxl.Color(255, 0, 0));
        break;
    case STATUS_TIME_ERROR:
        pxl.setPixelColor(0, pxl.Color(255, 0, 0));
        break;
    case STATUS_OK:
        pxl.setPixelColor(0, pxl.Color(0, 255, 0));
        break;
    case STATUS_ERROR:
        pxl.setPixelColor(0, pxl.Color(255, 0, 0));
        break;
    default:
        break;
    }

    pxl.setBrightness(LED_BRIGHTNESS);
    pxl.show();
}

static void blink(int times, int delayMs, uint32_t color)
{
    blink(times, delayMs, color, pxl.Color(0, 0, 0));
}

static void blink(int times, int delayMs, uint32_t color, uint32_t currentColor)
{
    for (int i = 0; i < times; i++)
    {
        pxl.setPixelColor(0, color);
        pxl.setBrightness(LED_BRIGHTNESS);
        pxl.show();
        delay(delayMs);
        pxl.setPixelColor(0, currentColor);
        pxl.setBrightness(LED_BRIGHTNESS);
        pxl.show();
        delay(delayMs);
    }
}
