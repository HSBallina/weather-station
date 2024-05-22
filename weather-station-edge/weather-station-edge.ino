#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "AzureIoT.h"

#include "secrets.h"

#define SERIAL_LOGGER_BAUD_RATE 115200
#define UNIX_TIME_NOV_13_2017 1510592825
#define UNIX_EPOCH_START_YEAR 1900

Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;
const char* VERSION = "20240522a";

static void logging_function(log_level_t log_level, char const* const format, ...);
static void connectWifi();

struct readings {
  float temperature, humidity, pressure;
};

struct sensor {
  Adafruit_BME280 sensor;
  String name;
  uint8_t address;
};

sensor outdoor = {bmeOut, "outdoor", 0x77};
sensor indoor = {bmeIn, "indoor", 0x76};


void setup() {
  Serial.begin(SERIAL_LOGGER_BAUD_RATE);

  while(!Serial) {
    delay(100);
  }

  set_logging_function(logging_function);

  Serial.println("");
  LogInfo("Weather Station %s", VERSION);
  Serial.println("");

  connectWifi();

  Wire.setPins(2, 1); // SDA, SCL

  initSensor(outdoor);
  initSensor(indoor);

  LogInfo("Setup done");
  Serial.println("");
}

void loop() {
  delay(10000);
  dumpReadings(getReadings(outdoor));
  delay(10000);
  dumpReadings(getReadings(indoor));
}

void initSensor(sensor &s) {
  LogInfo("Initializing sensor %s", s.name);

  if(!s.sensor.begin(s.address)) {
    LogError("Failed to initialize sensor %s", s.name);
    while(1);
  }

  LogInfo("Initializing sensor %s done", s.name);
}

readings getReadings(sensor &s) {
    Serial.println("-- Reading " + s.name + " --");
    readings r = {s.sensor.readTemperature(), s.sensor.readHumidity(), s.sensor.readPressure() / 100.0F};
    return r;
}

void dumpReadings(readings r) {
  Serial.print("Temperature = ");
  Serial.print(r.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(r.pressure);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(r.humidity);
  Serial.println(" %");

  Serial.println();
}

static void connectWifi()
{
  LogInfo("Connecting to WiFi %s", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    i++;
    if(i > 10) {
      Serial.println("");
      LogError("Failed to connect to WiFi %s", WIFI_SSID);
      while(1);
    }
  }

  Serial.println("");

  LogInfo("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
}

static void logging_function(log_level_t log_level, char const* const format, ...)
{
  struct tm* ptm;
  time_t now = time(NULL);

  ptm = gmtime(&now);

  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1);
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");

  if (ptm->tm_hour < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_sec);

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