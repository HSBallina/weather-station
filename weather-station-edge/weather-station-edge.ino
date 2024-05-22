#include <time.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"
#include "iot_configs.h"
#include "secrets.h"

#define SERIAL_LOGGER_BAUD_RATE 115200

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define CET_TZ "CET-1CEST,M3.5.0,M10.5.0/3"
#define VERSION "20240522e"

Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;

static void initTime(String timezone);
static void loggingFunction(log_level_t log_level, char const *const format, ...);
static void connectWifi();

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

sensor outdoor = {bmeOut, "outdoor", 0x77};
sensor indoor = {bmeIn, "indoor", 0x76};

void setup()
{
  Serial.begin(SERIAL_LOGGER_BAUD_RATE);

  while (!Serial)
  {
    delay(100);
  }

  set_logging_function(loggingFunction);

  Serial.println("");
  LogInfo("Weather Station %s", VERSION);
  Serial.println("");

  connectWifi();
  initTime(CET_TZ);

  azure_pnp_init();
  azure_pnp_set_telemetry_frequency(TELEMETRY_FREQUENCY_IN_SECONDS);

  Wire.setPins(2, 1); // SDA, SCL

  initSensor(outdoor);
  initSensor(indoor);

  LogInfo("Setup done");
  Serial.println("");
}

void loop()
{
  delay(10000);
  dumpReadings(outdoor);
  delay(10000);
  dumpReadings(indoor);
}

void initSensor(sensor &s)
{
  LogInfo("Initializing sensor %s", s.name);

  if (!s.sensor.begin(s.address))
  {
    LogError("Failed to initialize sensor %s", s.name);
    while (1)
      ;
  }

  LogInfo("Initializing sensor %s done", s.name);
}

readings getReadings(sensor &s)
{
  return {s.sensor.readTemperature(), s.sensor.readHumidity(), s.sensor.readPressure() / 100.0F};
}

void dumpReadings(sensor &s)
{
  readings r = getReadings(s);

  String payload = "{\"sensor\": \"" + s.name + "\", \"temperature\": " + String(r.temperature) + ", \"humidity\": " + String(r.humidity) + ", \"pressure\": " + String(r.pressure) + "}";
  LogInfo("Sending payload: %s", payload.c_str());
}

static void connectWifi()
{
  LogInfo("Connecting to WiFi %s", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
    i++;
    if (i > 10)
    {
      Serial.println("");
      LogError("Failed to connect to WiFi %s", WIFI_SSID);
      while (1)
        ;
    }
  }

  Serial.println("");

  LogInfo("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
}

static void initTime(String timezone)
{
  LogInfo("Setting time using SNTP");

  struct tm timeinfo;
  configTime(0, 0, NTP_SERVERS);

  Serial.print(".");

  int i = 0;
  while(!getLocalTime(&timeinfo, 2000UL)) {
    Serial.print(".");
    i++;
    if (i > 10)
    {
      Serial.println("");
      LogError("Failed to connect to WiFi %s", WIFI_SSID);
      while (1)
        ;
    }
  }

  Serial.println("");

  setenv("TZ", timezone.c_str(), 1);
  tzset();

  LogInfo("Time set to %s", asctime(&timeinfo));
}

static void loggingFunction(log_level_t log_level, char const *const format, ...)
{
  struct tm ptm;

  if (!getLocalTime(&ptm, 500UL))
  {
    time_t now = time(NULL);
    struct tm *ptmp = gmtime(&now);
    ptm = *ptmp;
  }

  Serial.print(&ptm, "%Y-%m-%d %H:%M:%S %z");

  Serial.print(log_level == log_level_info ? " [INFO] " : " [ERROR] ");

  char message[256];
  va_list ap;
  va_start(ap, format);
  int message_length = vsnprintf(message, 256, format, ap);
  va_end(ap);

  if (message_length < 0)
  {
    Serial.println("Failed encoding log message (!)");
  }
  else
  {
    Serial.println(message);
  }
}