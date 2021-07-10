/*
    This Arduino sketch sends a GET request to the specified
    domain and appends a unique DEVICE_ID as a query parameter
    on the request. After sending the device will go into Deep 
    Sleep mode to conserve power.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include "private_settings.h"

StaticJsonDocument<512> doc;

void deepsleep() {
  // Wait 1 second and then initiate Deep Sleep to conserve battery
  delay(1000); 
  ESP.deepSleep(0);
}

int login(BearSSL::WiFiClientSecure *client, char *token)
{
   HTTPClient https;

  Serial.print("[HTTPS] begin login request...\n");
  if (!https.begin(*client, "https://198.0.0.94:8080/api/4/auth/login")) {
    Serial.printf("[HTTPS] Unable to connect\n");
    return -1;
  }

  int httpCode = https.POST(login_text);
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("Login Result: ");
  Serial.println(payload);

  auto error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());  
    return -1;
  }

  int statusCode = doc["statusCode"];
  if (statusCode != 0) {
    Serial.printf("Login status code %d", statusCode);
    return -1;
  }

  const char* jsonToken = doc["access_token"];
  if ((jsonToken == NULL) || (strlen(jsonToken)>56)) {
    Serial.printf("Invalid access_token");
    return -1;
  }

  Serial.printf("Token: %s\n", jsonToken);

  strcpy(token, jsonToken);

  return 0;
}

int getQueue(BearSSL::WiFiClientSecure *client, char *token)
{
  char url[128];
  HTTPClient https;

  sprintf(url, "https://198.0.0.94:8080/api/4/watering/queue?access_token=%s", token);

  Serial.print("[HTTPS] begin get request...\n");
  if (!https.begin(*client, url)) {
    Serial.printf("[HTTPS] Unable to connect\n");
    return -1;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("GetQueue Result: ");
  Serial.println(payload);

  return 0;
}

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Explicitly set the ESP8266 to be a WiFi-client
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure;
  client->setInsecure();

  char token[57];

  if (login(client, token) !=0 ) {
    deepsleep();
    return;
  }

  if (getQueue(client,token) !=0 ) {
    deepsleep();
    return;
  }

  deepsleep();
}

void loop(){}
