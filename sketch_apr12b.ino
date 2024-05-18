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

unsigned long lastCheckTime;

void(* resetFunc) (void) = 0;

const uint8_t NUM_SPINNERS = 4;

const unsigned int NUM_SITES = 3;

const char* sites[] = {"https://example.com", "https://kaon.cc", "https://mizu.serv.kaon.cc"};

const uint8_t sitePatterns[3][7] = {
  {
    0b10101000,
    0b00101000,
    0b11001000,
    0b00010000,
    0b11100000,
    0b00000000,
    0b00000000,
  },
  {
    0b10000000,
    0b10011000,
    0b01000000,
    0b00111000,
    0b01000000,
    0b10100000,
    0b10010000,
  },
  {
    0b01010000,
    0b00110000,
    0b10000000,
    0b11111000,
    0b00100000,
    0b01000000,
    0b10100000,
  },
};

bool lastCheckSuccess;

void checkSite(int siteIndex) {
  lc.setRow(0, 0, sitePatterns[siteIndex][0]);
  lc.setRow(0, 1, sitePatterns[siteIndex][1]);
  lc.setRow(0, 2, sitePatterns[siteIndex][2]);
  lc.setRow(0, 3, 0b00000010 | sitePatterns[siteIndex][3]);
  lc.setRow(0, 4, sitePatterns[siteIndex][4]);
  lc.setRow(0, 5, 0b00000010 | sitePatterns[siteIndex][5]);
  lc.setRow(0, 6, sitePatterns[siteIndex][6]);
  lc.setRow(0, 7, 0b00000010);
  
  WiFiClientSecure wclient;
  wclient.setInsecure(); // ignore SSL errors

  HTTPClient hclient;
  boolean success = false;
  if (hclient.begin(wclient, sites[siteIndex])) {
    int statusCode = hclient.GET();
    if (statusCode == 200) success = true;
  }

  if (success) {
    lc.setRow(0, 3, sitePatterns[siteIndex][3]);
    lc.setRow(0, 4, 0b00000010 | sitePatterns[siteIndex][4]);
    lc.setRow(0, 5, 0b00000100 | sitePatterns[siteIndex][5]);
    lc.setRow(0, 6, 0b00000010 | sitePatterns[siteIndex][6]);
    lc.setRow(0, 7, 0b00000001);
  } else {
    lc.setRow(0, 3, sitePatterns[siteIndex][3]);
    lc.setRow(0, 5, 0b00000101 | sitePatterns[siteIndex][5]);
    lc.setRow(0, 6, 0b00000010 | sitePatterns[siteIndex][6]);
    lc.setRow(0, 7, 0b00000101);
  }

  lastCheckSuccess = success;
  delay(1000);
}

void setup() {

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,1);
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


  // connect to example.com to make sure
  // everything is working
  checkSite(0);
  lastCheckTime = millis();
  if (lastCheckSuccess) lc.clearDisplay(0); else {
    delay(15000);
    resetFunc();
  }
}

int currSite = 1;

void loop() {
  if ((millis() - lastCheckTime) >= 5000) {
    lc.setIntensity(0, 1);
    checkSite(currSite);
    lastCheckTime = millis();
    if (lastCheckSuccess) lc.clearDisplay(0);
    if ((currSite == 0) && !lastCheckSuccess) {
      delay(15000);
      resetFunc();
    }
    currSite = (currSite + 1) % NUM_SITES;
  } else if (!lastCheckSuccess) {
    if (((millis() - lastCheckTime) % 1000) < 500) {
      lc.setIntensity(0, 8);
    } else {
      lc.setIntensity(0, 1);
    }
  }
}
