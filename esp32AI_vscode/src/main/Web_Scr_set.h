#ifndef Web_Scr_h
#define Web_Scr_h

#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include "Audio1.h"
#include "Audio2.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <Wire.h>
#include <SD.h>
// 与AP模式和Web服务器有关的库
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
// 与屏幕显示有关的库
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "guichu.h"
#include "listening.h"

#include <FastLED.h>
#include <TJpg_Decoder.h>
#define width   128     //图片宽度
#define height  160     //图片高度

// AP模式的SSID和密码
extern const char *ap_ssid;
extern const char *ap_password;
// Web服务器和Preferences对象
extern AsyncWebServer server;
extern Preferences preferences;

extern bool isWebServerStarted;
extern bool isSoftAPStarted;

void openWeb();
void handleRoot(AsyncWebServerRequest *request);
void handleWifiManagement(AsyncWebServerRequest *request);
void handleMusicManagement(AsyncWebServerRequest *request);
void handleLLMManagement(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);
void handleSaveMusic(AsyncWebServerRequest *request);
void handleDeleteMusic(AsyncWebServerRequest *request);
void handleListMusic(AsyncWebServerRequest *request);
void handleSaveLLM(AsyncWebServerRequest *request);
void handleListLLM(AsyncWebServerRequest *request);

#endif