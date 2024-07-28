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
// 与AP模式和Web服务器有关的库
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
// 与屏幕显示有关的库
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>
// #include "bizhi.h"    //导入壁纸数据
// tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);   // 用于壁纸显示的代码

// 屏幕引脚定义
#define TFT_CS 5
#define TFT_RST 12
#define TFT_DC 32
#define TFT_SCLK 18
#define TFT_MOSI 23

// 创建屏幕对象
extern Adafruit_ST7735 tft;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;
// AP模式的SSID和密码
extern const char *ap_ssid;
extern const char *ap_password;
// Web服务器和Preferences对象
extern AsyncWebServer server;
extern Preferences preferences;

void openWeb();
void handleRoot(AsyncWebServerRequest *request);
void handleWifiManagement(AsyncWebServerRequest *request);
void handleMusicManagement(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);
void handleSaveMusic(AsyncWebServerRequest *request);
void handleDeleteMusic(AsyncWebServerRequest *request);
void handleListMusic(AsyncWebServerRequest *request);

#endif