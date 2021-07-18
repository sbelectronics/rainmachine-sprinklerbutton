/*
  Rainmachine-SprinklerValue
  Scott Baker, http://www.smbaker.com/

  This sketch implements a Wifi button that will start a zone on a
  rainmachine sprinkler timer for a preconfigured number of minutes.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// SSID, passwords, etc, are stored in private_settings.h.

#include "private_settings.h"

// Holds buffer space for parsing JSON responses from the server.
StaticJsonDocument<512> doc;

void deepsleep() {
  // Print an exit message, delay a second, and then enter deep sleep
  Serial.println("entering deep sleep\r\n");
  delay(1000); 
  ESP.deepSleep(0);
}

int login(BearSSL::WiFiClientSecure *client, char *token)
{
   HTTPClient https;

  Serial.print("[HTTPS] begin login request...\r\n");
  if (!https.begin(*client, "https://198.0.0.94:8080/api/4/auth/login")) {
    Serial.printf("[HTTPS] Unable to connect\r\n");
    return -1;
  }

  int httpCode = https.POST(login_text);
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d\r\n", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("Login Result: ");
  Serial.println(payload);

  // Here's where we parse the response from the rainmachine. It's encoded
  // in JSON and includes several fields including "statusCode" and
  // "access_token". The access_token is what is important to us, as we'll
  // need it later.

  auto error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());  
    return -1;
  }

  int statusCode = doc["statusCode"];
  if (statusCode != 0) {
    Serial.printf("Login status code %d\r\n", statusCode);
    return -1;
  }

  const char* jsonToken = doc["access_token"];
  if ((jsonToken == NULL) || (strlen(jsonToken)>56)) {
    Serial.printf("Invalid access_token");
    return -1;
  }

  Serial.printf("Token: %s\r\n", jsonToken);

  // Return the token to the caller.
  strcpy(token, jsonToken);

  return 0;
}

int getQueue(BearSSL::WiFiClientSecure *client, char *token)
{
  char url[128];
  HTTPClient https;

  sprintf(url, "https://198.0.0.94:8080/api/4/watering/queue?access_token=%s", token);

  Serial.print("[HTTPS] begin getQueue request...\r\n");
  if (!https.begin(*client, url)) {
    Serial.printf("[HTTPS] Unable to connect\r\n");
    return -1;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d\r\n", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("getQueue Result: ");
  Serial.println(payload);

  return 0;
}

int zoneStart(BearSSL::WiFiClientSecure *client, char *token, int zone, int duration)
{
  char url[128], timeRequest[32];
  HTTPClient https;

  sprintf(url, "https://198.0.0.94:8080/api/4/zone/%d/start?access_token=%s", zone, token);

  sprintf(timeRequest, "{\"time\": %d}", duration);

  Serial.print("[HTTPS] begin zone start post request...\r\n");
  if (!https.begin(*client, url)) {
    Serial.printf("[HTTPS] Unable to connect\r\n");
    return -1;
  }

  int httpCode = https.POST(timeRequest);
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d\r\n", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("zoneStart Result: ");
  Serial.println(payload);

  return 0;  
}

int zoneStop(BearSSL::WiFiClientSecure *client, char *token, int zone)
{
  char url[128];
  HTTPClient https;

  sprintf(url, "https://198.0.0.94:8080/api/4/zone/%d/stop?access_token=%s", zone, token);

  Serial.print("[HTTPS] begin zone stop post request...\r\n");
  if (!https.begin(*client, url)) {
    Serial.printf("[HTTPS] Unable to connect\r\n");
    return -1;
  }

  int httpCode = https.POST("");
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] Error %d\r\n", httpCode); 
    return -1;
  }

  String payload = https.getString();
  Serial.print("zoneStop Result: ");
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure;

  // Set the client to operate in insecure mode. The alternative is
  // to get the fingerprint of the SSL certificate. That sounds like
  // work, especially considering a firmware upgrade on the spinkler
  // controller could change it.

  client->setInsecure();

  char token[57];

  // Login. The spinkler timer will respond with a token that can
  // be used for subsequent requests.
  if (login(client, token) !=0 ) {
    deepsleep();
    return;
  }

  // stop zone8, so we can restart it back to the full duration
  zoneStop(client, token, 8);

  // start zone8, with 30 minute duration
  if (zoneStart(client, token, 8, 1800) !=0 ) { // 300=5min; 1800=30min
    deepsleep();
    return;
  }

  deepsleep();
}

// There is no "loop" ... The program effectively terminates in setup() when
// deepsleep is called.
void loop(){}
