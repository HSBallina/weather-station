#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WiFiClientSecure.h>

bool connectWifi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    i++;
    if(i > 10) {
      Serial.println(" failed to connect.");
      return false;
    }
  }

  Serial.println(" connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

#endif