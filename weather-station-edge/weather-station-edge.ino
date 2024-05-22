#include <time.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

#include <mqtt_client.h>

#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"
#include "iot_configs.h"
#include "secrets.h"

#define SERIAL_LOGGER_BAUD_RATE 115200

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define CET_TZ "CET-1CEST,M3.5.0,M10.5.0/3"
#define VERSION "20240522f"

#define MQTT_DO_NOT_RETAIN_MSG 0
#define RESULT_OK 0
#define RESULT_ERROR __LINE__


Adafruit_BME280 bmeOut;
Adafruit_BME280 bmeIn;

static azure_iot_config_t azure_iot_config;
static azure_iot_t azure_iot;
static esp_mqtt_client_handle_t mqtt_client;

static uint32_t properties_request_id = 0;

#define AZ_IOT_DATA_BUFFER_SIZE 1500
static uint8_t az_iot_data_buffer[AZ_IOT_DATA_BUFFER_SIZE];

#define MQTT_PROTOCOL_PREFIX "mqtts://"

static void initTime(String timezone);
static void loggingFunction(log_level_t log_level, char const *const format, ...);
static void connectWifi();
static void configure_azure_iot();
static esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event);
static char mqtt_broker_uri[128];

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

#pragma region MQTT interface functions
/* --- MQTT Interface Functions --- */
/*
 * These functions are used by Azure IoT to interact with whatever MQTT client used by the sample
 * (in this case, Espressif's ESP MQTT). Please see the documentation in AzureIoT.h for more
 * details.
 */

/*
 * See the documentation of `mqtt_client_init_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_init_function(
    mqtt_client_config_t* mqtt_client_config,
    mqtt_client_handle_t* mqtt_client_handle)
{
  int result;
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));

  az_span mqtt_broker_uri_span = AZ_SPAN_FROM_BUFFER(mqtt_broker_uri);
  mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, AZ_SPAN_FROM_STR(MQTT_PROTOCOL_PREFIX));
  mqtt_broker_uri_span = az_span_copy(mqtt_broker_uri_span, mqtt_client_config->address);
  az_span_copy_u8(mqtt_broker_uri_span, null_terminator);

  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_client_config->port;
  mqtt_config.client_id = (const char*)az_span_ptr(mqtt_client_config->client_id);
  mqtt_config.username = (const char*)az_span_ptr(mqtt_client_config->username);

#ifdef IOT_CONFIG_USE_X509_CERT
  LogInfo("MQTT client using X509 Certificate authentication");
  mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
  mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
#else // Using SAS key
  mqtt_config.password = (const char*)az_span_ptr(mqtt_client_config->password);
#endif

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = esp_mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;

  LogInfo("MQTT client target uri set to '%s'", mqtt_broker_uri);

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    LogError("esp_mqtt_client_init failed.");
    result = 1;
  }
  else
  {
    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK)
    {
      LogError("esp_mqtt_client_start failed (error code: 0x%08x).", start_result);
      result = 1;
    }
    else
    {
      *mqtt_client_handle = mqtt_client;
      result = 0;
    }
  }

  return result;
}

/*
 * See the documentation of `mqtt_client_deinit_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_deinit_function(mqtt_client_handle_t mqtt_client_handle)
{
  int result = 0;
  esp_mqtt_client_handle_t esp_mqtt_client_handle = (esp_mqtt_client_handle_t)mqtt_client_handle;

  LogInfo("MQTT client being disconnected.");

  if (esp_mqtt_client_stop(esp_mqtt_client_handle) != ESP_OK)
  {
    LogError("Failed stopping MQTT client.");
  }

  if (esp_mqtt_client_destroy(esp_mqtt_client_handle) != ESP_OK)
  {
    LogError("Failed destroying MQTT client.");
  }

  if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
  {
    LogError("Failed updating azure iot client of MQTT disconnection.");
  }

  return 0;
}

/*
 * See the documentation of `mqtt_client_subscribe_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_subscribe_function(
    mqtt_client_handle_t mqtt_client_handle,
    az_span topic,
    mqtt_qos_t qos)
{
  LogInfo("MQTT client subscribing to '%.*s'", az_span_size(topic), az_span_ptr(topic));

  // As per documentation, `topic` always ends with a null-terminator.
  // esp_mqtt_client_subscribe returns the packet id or negative on error already, so no conversion
  // is needed.
  int packet_id = esp_mqtt_client_subscribe(
      (esp_mqtt_client_handle_t)mqtt_client_handle, (const char*)az_span_ptr(topic), (int)qos);

  return packet_id;
}

/*
 * See the documentation of `mqtt_client_publish_function_t` in AzureIoT.h for details.
 */
static int mqtt_client_publish_function(
    mqtt_client_handle_t mqtt_client_handle,
    mqtt_message_t* mqtt_message)
{
  LogInfo("MQTT client publishing to '%s'", az_span_ptr(mqtt_message->topic));

  int mqtt_result = esp_mqtt_client_publish(
      (esp_mqtt_client_handle_t)mqtt_client_handle,
      (const char*)az_span_ptr(mqtt_message->topic), // topic is always null-terminated.
      (const char*)az_span_ptr(mqtt_message->payload),
      az_span_size(mqtt_message->payload),
      (int)mqtt_message->qos,
      MQTT_DO_NOT_RETAIN_MSG);

  if (mqtt_result == -1)
  {
    return RESULT_ERROR;
  }
  else
  {
    return RESULT_OK;
  }
}

/* --- Other Interface functions required by Azure IoT --- */

/*
 * See the documentation of `hmac_sha256_encryption_function_t` in AzureIoT.h for details.
 */
static int mbedtls_hmac_sha256(
    const uint8_t* key,
    size_t key_length,
    const uint8_t* payload,
    size_t payload_length,
    uint8_t* signed_payload,
    size_t signed_payload_size)
{
  (void)signed_payload_size;
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, key_length);
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, payload_length);
  mbedtls_md_hmac_finish(&ctx, (byte*)signed_payload);
  mbedtls_md_free(&ctx);

  return 0;
}

/*
 * See the documentation of `base64_decode_function_t` in AzureIoT.h for details.
 */
static int base64_decode(
    uint8_t* data,
    size_t data_length,
    uint8_t* decoded,
    size_t decoded_size,
    size_t* decoded_length)
{
  return mbedtls_base64_decode(decoded, decoded_size, decoded_length, data, data_length);
}

/*
 * See the documentation of `base64_encode_function_t` in AzureIoT.h for details.
 */
static int base64_encode(
    uint8_t* data,
    size_t data_length,
    uint8_t* encoded,
    size_t encoded_size,
    size_t* encoded_length)
{
  return mbedtls_base64_encode(encoded, encoded_size, encoded_length, data, data_length);
}

/*
 * See the documentation of `properties_update_completed_t` in AzureIoT.h for details.
 */
static void on_properties_update_completed(uint32_t request_id, az_iot_status status_code)
{
  LogInfo("Properties update request completed (id=%d, status=%d)", request_id, status_code);
}

/*
 * See the documentation of `properties_received_t` in AzureIoT.h for details.
 */
void on_properties_received(az_span properties)
{
  LogInfo("Properties update received: %.*s", az_span_size(properties), az_span_ptr(properties));

  // It is recommended not to perform work within callbacks.
  // The properties are being handled here to simplify the sample.
  if (azure_pnp_handle_properties_update(&azure_iot, properties, properties_request_id++) != 0)
  {
    LogError("Failed handling properties update.");
  }
}

/*
 * See the documentation of `command_request_received_t` in AzureIoT.h for details.
 */
static void on_command_request_received(command_request_t command)
{
  az_span component_name
      = az_span_size(command.component_name) == 0 ? AZ_SPAN_FROM_STR("") : command.component_name;

  LogInfo(
      "Command request received (id=%.*s, component=%.*s, name=%.*s)",
      az_span_size(command.request_id),
      az_span_ptr(command.request_id),
      az_span_size(component_name),
      az_span_ptr(component_name),
      az_span_size(command.command_name),
      az_span_ptr(command.command_name));

  // Here the request is being processed within the callback that delivers the command request.
  // However, for production application the recommendation is to save `command` and process it
  // outside this callback, usually inside the main thread/task/loop.
  (void)azure_pnp_handle_command_request(&azure_iot, command);
}
#pragma endregion

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

  configure_azure_iot();
  azure_iot_start(&azure_iot);

  LogInfo("Azure IoT client initialized (state=%d)", azure_iot.state);

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

static void configure_azure_iot()
{
  /*
   * The configuration structure used by Azure IoT must remain unchanged (including data buffer)
   * throughout the lifetime of the sample. This variable must also not lose context so other
   * components do not overwrite any information within this structure.
   */
  azure_iot_config.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  azure_iot_config.model_id = azure_pnp_get_model_id();
  azure_iot_config.use_device_provisioning = true; // Required for Azure IoT Central.
  azure_iot_config.iot_hub_fqdn = AZ_SPAN_EMPTY;
  azure_iot_config.device_id = AZ_SPAN_EMPTY;

#ifdef IOT_CONFIG_USE_X509_CERT
  azure_iot_config.device_certificate = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT);
  azure_iot_config.device_certificate_private_key = AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY);
  azure_iot_config.device_key = AZ_SPAN_EMPTY;
#else
  azure_iot_config.device_certificate = AZ_SPAN_EMPTY;
  azure_iot_config.device_certificate_private_key = AZ_SPAN_EMPTY;
  azure_iot_config.device_key = AZ_SPAN_FROM_STR(DEVICE_KEY);
#endif // IOT_CONFIG_USE_X509_CERT

  azure_iot_config.dps_id_scope = AZ_SPAN_FROM_STR(SCOPE_ID);
  azure_iot_config.dps_registration_id = AZ_SPAN_FROM_STR(DEVICE_ID); // Use Device ID for Azure IoT Central.
  azure_iot_config.data_buffer = AZ_SPAN_FROM_BUFFER(az_iot_data_buffer);
  azure_iot_config.sas_token_lifetime_in_minutes = MQTT_PASSWORD_LIFETIME_IN_MINUTES;
  azure_iot_config.mqtt_client_interface.mqtt_client_init = mqtt_client_init_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_deinit = mqtt_client_deinit_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_subscribe = mqtt_client_subscribe_function;
  azure_iot_config.mqtt_client_interface.mqtt_client_publish = mqtt_client_publish_function;
  azure_iot_config.data_manipulation_functions.hmac_sha256_encrypt = mbedtls_hmac_sha256;
  azure_iot_config.data_manipulation_functions.base64_decode = base64_decode;
  azure_iot_config.data_manipulation_functions.base64_encode = base64_encode;
  azure_iot_config.on_properties_update_completed = on_properties_update_completed;
  azure_iot_config.on_properties_received = on_properties_received;
  azure_iot_config.on_command_request_received = on_command_request_received;

  azure_iot_init(&azure_iot, &azure_iot_config);
}

static esp_err_t esp_mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
    int i, r;

    case MQTT_EVENT_ERROR:
      LogError("MQTT client in ERROR state.");
      LogError(
          "esp_tls_stack_err=%d; "
          "esp_tls_cert_verify_flags=%d;esp_transport_sock_errno=%d;error_type=%d;connect_return_"
          "code=%d",
          event->error_handle->esp_tls_stack_err,
          event->error_handle->esp_tls_cert_verify_flags,
          event->error_handle->esp_transport_sock_errno,
          event->error_handle->error_type,
          event->error_handle->connect_return_code);

      switch (event->error_handle->connect_return_code)
      {
        case MQTT_CONNECTION_ACCEPTED:
          LogError("connect_return_code=MQTT_CONNECTION_ACCEPTED");
          break;
        case MQTT_CONNECTION_REFUSE_PROTOCOL:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_PROTOCOL");
          break;
        case MQTT_CONNECTION_REFUSE_ID_REJECTED:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_ID_REJECTED");
          break;
        case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE");
          break;
        case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_BAD_USERNAME");
          break;
        case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
          LogError("connect_return_code=MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED");
          break;
        default:
          LogError("connect_return_code=unknown (%d)", event->error_handle->connect_return_code);
          break;
      };

      break;
    case MQTT_EVENT_CONNECTED:
      LogInfo("MQTT client connected (session_present=%d).", event->session_present);

      if (azure_iot_mqtt_client_connected(&azure_iot) != 0)
      {
        LogError("azure_iot_mqtt_client_connected failed.");
      }

      break;
    case MQTT_EVENT_DISCONNECTED:
      LogInfo("MQTT client disconnected.");

      if (azure_iot_mqtt_client_disconnected(&azure_iot) != 0)
      {
        LogError("azure_iot_mqtt_client_disconnected failed.");
      }

      break;
    case MQTT_EVENT_SUBSCRIBED:
      LogInfo("MQTT topic subscribed (message id=%d).", event->msg_id);

      if (azure_iot_mqtt_client_subscribe_completed(&azure_iot, event->msg_id) != 0)
      {
        LogError("azure_iot_mqtt_client_subscribe_completed failed.");
      }

      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      LogInfo("MQTT topic unsubscribed.");
      break;
    case MQTT_EVENT_PUBLISHED:
      LogInfo("MQTT event MQTT_EVENT_PUBLISHED");

      if (azure_iot_mqtt_client_publish_completed(&azure_iot, event->msg_id) != 0)
      {
        LogError("azure_iot_mqtt_client_publish_completed failed (message id=%d).", event->msg_id);
      }

      break;
    case MQTT_EVENT_DATA:
      LogInfo("MQTT message received.");

      mqtt_message_t mqtt_message;
      mqtt_message.topic = az_span_create((uint8_t*)event->topic, event->topic_len);
      mqtt_message.payload = az_span_create((uint8_t*)event->data, event->data_len);
      mqtt_message.qos
          = mqtt_qos_at_most_once; // QoS is unused by azure_iot_mqtt_client_message_received.

      if (azure_iot_mqtt_client_message_received(&azure_iot, &mqtt_message) != 0)
      {
        LogError(
            "azure_iot_mqtt_client_message_received failed (topic=%.*s).",
            event->topic_len,
            event->topic);
      }

      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      LogInfo("MQTT client connecting.");
      break;
    default:
      LogError("MQTT event UNKNOWN.");
      break;
  }

  return ESP_OK;
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
  while (!getLocalTime(&timeinfo, 2000UL))
  {
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