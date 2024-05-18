#include <ESP8266WiFi.h>
#include <LedControl.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
#include "c_types.h"
}

// SSID to connect to
char ssid[] = "eduroam";
#include "WIFI_CREDENTIALS.h"

/*
 Now we need a LedControl to work with.
 We have only a single MAX72XX.
 */
const int DIN = 5;
const int SEL = 4;
const int CLK = 0;
LedControl lc=LedControl(DIN, CLK, SEL, 1);

uint8_t target_esp_mac[6] = {0x24, 0x0a, 0xc4, 0x9a, 0x58, 0x28};

const uint8_t SPINNERS[4][3] = {
  {
    0b00000011,
    0b00000000,
    0b00000110,
  },
  {
    0b00000001,
    0b00000101,
    0b00000100,
  },
  {
    0b00000100,
    0b00000101,
    0b00000001,
  },
  {
    0b00000110,
    0b00000000,
    0b00000011,
  },
};

const uint8_t NUM_SPINNERS = 4;

void setup() {

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);

  lc.setRow(0, 0, 0b10101000);
  lc.setRow(0, 1, 0b00101000);
  lc.setRow(0, 2, 0b11001000);
  lc.setRow(0, 3, 0b00010000);
  lc.setRow(0, 4, 0b11100000);
  lc.setRow(0, 5, 0b00000001);
  lc.setRow(0, 6, 0b00101101);
  lc.setRow(0, 7, 0b00000111);

  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(1000);
  Serial.setDebugOutput(true);
  Serial.printf("SDK version: %s\n", system_get_sdk_version());
  Serial.printf("Free Heap: %4d\n",ESP.getFreeHeap());
  
  // Setting ESP into STATION mode only (no AP mode or dual mode)
  wifi_set_opmode(STATION_MODE);

  struct station_config wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  strcpy((char*)wifi_config.ssid, ssid);
  strcpy((char*)wifi_config.password, password);

  wifi_station_set_config(&wifi_config);
  wifi_set_macaddr(STATION_IF,target_esp_mac);
  
  wifi_station_set_wpa2_enterprise_auth(1);

  // Clean up to be sure no old data is still inside
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_clear_enterprise_identity();
  wifi_station_clear_enterprise_username();
  wifi_station_clear_enterprise_password();
  wifi_station_clear_enterprise_new_password();
  
  wifi_station_set_enterprise_identity((uint8*)identity, strlen(identity));
  wifi_station_set_enterprise_username((uint8*)username, strlen(username));
  wifi_station_set_enterprise_password((uint8*)password, strlen((char*)password));

  lc.setRow(0, 0, 0b10101000);
  lc.setRow(0, 1, 0b00101000);
  lc.setRow(0, 2, 0b11001000);
  lc.setRow(0, 3, 0b00010000);
  lc.setRow(0, 4, 0b11100000);
  lc.setRow(0, 5, 0b00000000);
  lc.setRow(0, 6, 0b00000000);
  lc.setRow(0, 7, 0b00000000);

  wifi_station_connect();
  int spinner_index = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    lc.setRow(0, 5, SPINNERS[spinner_index][0]);
    lc.setRow(0, 6, SPINNERS[spinner_index][1]);
    lc.setRow(0, 7, SPINNERS[spinner_index][2]);
    spinner_index = (spinner_index + 1) % NUM_SPINNERS;
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  lc.setRow(0, 3, 0b00010010);
  lc.setRow(0, 5, 0b00000010);
  lc.setRow(0, 6, 0b00000000);
  lc.setRow(0, 7, 0b00000010);

  // connect to example.com to make sure
  // everything is working
  WiFiClientSecure wclient;
  wclient.setInsecure(); // ignore SSL errors

  HTTPClient hclient;
  boolean success = false;
  if (hclient.begin(wclient, "https://example.com")) {
    int statusCode = hclient.GET();
    if (statusCode == 200) success = true;
  }
  if (success) {
    lc.setRow(0, 3, 0b00010000);
    lc.setRow(0, 4, 0b11100010);
    lc.setRow(0, 5, 0b00000100);
    lc.setRow(0, 6, 0b00000010);
    lc.setRow(0, 7, 0b00000001);
  } else {
    lc.setRow(0, 3, 0b00010000);
    lc.setRow(0, 5, 0b00000101);
    lc.setRow(0, 6, 0b00000010);
    lc.setRow(0, 7, 0b00000101);
  }
  delay(1000);
  if (success) lc.clearDisplay(0); else {
    while(1) {}
  }
}

void loop() {
}
