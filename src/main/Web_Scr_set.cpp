#include "Web_Scr_set.h"

// 创建屏幕对象
TFT_eSPI tft = TFT_eSPI();  // 创建TFT对象
U8g2_for_TFT_eSPI u8g2;

// AP模式的SSID和密码
const char *ap_ssid = "ESP32-Setup";
const char *ap_password = "12345678";
// Web服务器和Preferences对象
AsyncWebServer server(80);
Preferences preferences;

void openWeb()
{
    // 网络连接失败，启动 AP 模式创建热点用于配网和音乐信息添加
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Started Access Point");
    // 启动 Web 服务器
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiManagement);
    server.on("/music", HTTP_GET, handleMusicManagement);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/delete", HTTP_POST, handleDelete);
    server.on("/list", HTTP_GET, handleList);
    server.on("/saveMusic", HTTP_POST, handleSaveMusic);
    server.on("/deleteMusic", HTTP_POST, handleDeleteMusic);
    server.on("/listMusic", HTTP_GET, handleListMusic);
    
    server.begin();
    Serial.println("WebServer started, waiting for configuration...");
}
// 处理根路径的请求
void handleRoot(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Configuration</title><style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; } h1 { color: #333; } a { display: inline-block; padding: 10px 20px; margin: 10px; border: none; background-color: #333; color: white; text-decoration: none; cursor: pointer; } a:hover { background-color: #555; }</style></head><body><h1>ESP32 Configuration</h1><a href='/wifi'>Wi-Fi Management</a><a href='/music'>Music Management</a></body></html>";
    request->send(200, "text/html", html);
}
// wifi配置界面
void handleWifiManagement(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Wi-Fi Management</title><style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; } h1 { color: #333; } form { display: inline-block; margin-top: 20px; } input[type='text'], input[type='password'] { padding: 10px; margin: 10px 0; width: 200px; } input[type='submit'], input[type='button'] { padding: 10px 20px; margin: 10px 5px; border: none; background-color: #333; color: white; cursor: pointer; } input[type='submit']:hover, input[type='button']:hover { background-color: #555; }</style></head><body><h1>Wi-Fi Management</h1><form action='/save' method='post'><label for='ssid'>Wi-Fi SSID:</label><br><input type='text' id='ssid' name='ssid'><br><label for='password'>Password:</label><br><input type='password' id='password' name='password'><br><input type='submit' value='Save'></form><form action='/delete' method='post'><label for='ssid'>Wi-Fi SSID to Delete:</label><br><input type='text' id='ssid' name='ssid'><br><input type='submit' value='Delete'></form><a href='/list'><input type='button' value='List Wi-Fi Networks'></a><p><a href='/'>Go Back</a></p></body></html>";
    request->send(200, "text/html", html);
}
// 音乐信息配置界面
void handleMusicManagement(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Music Management</title><style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; } h1 { color: #333; } form { display: inline-block; margin-top: 20px; } input[type='text'], input[type='password'] { padding: 10px; margin: 10px 0; width: 200px; } input[type='submit'], input[type='button'] { padding: 10px 20px; margin: 10px 5px; border: none; background-color: #333; color: white; cursor: pointer; } input[type='submit']:hover, input[type='button']:hover { background-color: #555; }</style></head><body><h1>Music Management</h1><form action='/saveMusic' method='post'><label for='musicName'>Music Name:</label><br><input type='text' id='musicName' name='musicName'><br><label for='musicId'>Music ID:</label><br><input type='text' id='musicId' name='musicId'><br><input type='submit' value='Save Music'></form><form action='/deleteMusic' method='post'><label for='musicName'>Music Name to Delete:</label><br><input type='text' id='musicName' name='musicName'><br><input type='submit' value='Delete Music'></form><a href='/listMusic'><input type='button' value='List Saved Music'></a><p><a href='/'>Go Back</a></p></body></html>";
    request->send(200, "text/html", html);
}
// 添加或更新wifi信息逻辑
void handleSave(AsyncWebServerRequest *request)
{
    tft.fillScreen(TFT_WHITE);
    u8g2.setCursor(0, 11);
    u8g2.print("进入网络配置！");

    Serial.println("Start Save!");
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    preferences.begin("wifi_store", false);
    int numNetworks = preferences.getInt("numNetworks", 0);

    for (int i = 0; i < numNetworks; ++i)
    {
        String storedSsid = preferences.getString(("ssid" + String(i)).c_str(), "");
        if (storedSsid == ssid)
        {
            preferences.putString(("password" + String(i)).c_str(), password);
            u8g2.setCursor(0, 11 + 12);
            u8g2.print("wifi密码更新成功！");
            Serial.println("Succeess Update!");
            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Configuration Updated!</h1><p>Network password updated successfully.</p><p><a href='/'>Go Back</a></p></body></html>");
            preferences.end();
            return;
        }
    }

    preferences.putString(("ssid" + String(numNetworks)).c_str(), ssid);
    preferences.putString(("password" + String(numNetworks)).c_str(), password);
    preferences.putInt("numNetworks", numNetworks + 1);
    u8g2.setCursor(0, 11 + 12);
    u8g2.print("新wifi添加成功！");
    Serial.println("Succeess Save!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Configuration Saved!</h1><p>Network information added successfully.</p><p><a href='/'>Go Back</a></p></body></html>");
    preferences.end();
}
// 删除wifi信息逻辑
void handleDelete(AsyncWebServerRequest *request)
{
    tft.fillScreen(TFT_WHITE);
    u8g2.setCursor(0, 11);
    u8g2.print("进入网络配置！");

    Serial.println("Start Delete!");
    String ssidToDelete = request->arg("ssid");

    preferences.begin("wifi_store", false);
    int numNetworks = preferences.getInt("numNetworks", 0);

    for (int i = 0; i < numNetworks; ++i)
    {
        String storedSsid = preferences.getString(("ssid" + String(i)).c_str(), "");
        if (storedSsid == ssidToDelete)
        {
            preferences.remove(("ssid" + String(i)).c_str());
            preferences.remove(("password" + String(i)).c_str());

            for (int j = i; j < numNetworks - 1; ++j)
            {
                String nextSsid = preferences.getString(("ssid" + String(j + 1)).c_str(), "");
                String nextPassword = preferences.getString(("password" + String(j + 1)).c_str(), "");

                preferences.putString(("ssid" + String(j)).c_str(), nextSsid);
                preferences.putString(("password" + String(j)).c_str(), nextPassword);
            }

            preferences.remove(("ssid" + String(numNetworks - 1)).c_str());
            preferences.remove(("password" + String(numNetworks - 1)).c_str());
            preferences.putInt("numNetworks", numNetworks - 1);
            u8g2.setCursor(0, 11 + 12);
            u8g2.print("wifi删除成功！");
            Serial.println("Succeess Delete!");

            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Network Deleted!</h1><p>The network has been deleted.</p><p><a href='/'>Go Back</a></p></body></html>");
            preferences.end();
            return;
        }
    }
    u8g2.setCursor(0, 11 + 12);
    u8g2.print("该wifi不存在！");
    Serial.println("Fail to Delete!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Network Not Found!</h1><p>The specified network was not found.</p><p><a href='/'>Go Back</a></p></body></html>");
    preferences.end();
}
// 显示已有wifi信息逻辑
void handleList(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Saved Wi-Fi Networks</h1><ul>";

    preferences.begin("wifi_store", true);
    int numNetworks = preferences.getInt("numNetworks", 0);

    for (int i = 0; i < numNetworks; ++i)
    {
        String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
        String password = preferences.getString(("password" + String(i)).c_str(), "");
        html += "<li>ssid" + String(i) + ": " + ssid + " " + password + "</li>";
    }

    html += "</ul><p><a href='/'>Go Back</a></p></body></html>";

    request->send(200, "text/html", html);
    preferences.end();
}
// 添加或更新音乐信息逻辑
void handleSaveMusic(AsyncWebServerRequest *request)
{
    tft.fillScreen(TFT_WHITE);
    u8g2.setCursor(0, 11);
    u8g2.print("进入音乐配置！");

    Serial.println("Start Save Music!");
    String musicName = request->arg("musicName");
    String musicId = request->arg("musicId");

    preferences.begin("music_store", false);
    int numMusic = preferences.getInt("numMusic", 0);

    for (int i = 0; i < numMusic; ++i)
    {
        String storedMusicName = preferences.getString(("musicName" + String(i)).c_str(), "");
        if (storedMusicName == musicName)
        {
            preferences.putString(("musicId" + String(i)).c_str(), musicId);
            u8g2.setCursor(0, 11 + 12);
            u8g2.print("音乐ID更新成功！");
            Serial.println("Success Update Music!");
            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Music Configuration</title></head><body><h1>Music ID Updated!</h1><p>Music ID updated successfully.</p><p><a href='/'>Go Back</a></p></body></html>");
            preferences.end();
            return;
        }
    }

    preferences.putString(("musicName" + String(numMusic)).c_str(), musicName);
    preferences.putString(("musicId" + String(numMusic)).c_str(), musicId);
    preferences.putInt("numMusic", numMusic + 1);
    u8g2.setCursor(0, 11 + 12);
    u8g2.print("新音乐添加成功！");
    Serial.println("Success Save Music!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Music Configuration</title></head><body><h1>Music Saved!</h1><p>Music information added successfully.</p><p><a href='/'>Go Back</a></p></body></html>");
    preferences.end();
}
// 删除音乐信息逻辑
void handleDeleteMusic(AsyncWebServerRequest *request)
{
    tft.fillScreen(TFT_WHITE);
    u8g2.setCursor(0, 11);
    u8g2.print("进入音乐配置！");

    Serial.println("Start Delete Music!");
    String musicNameToDelete = request->arg("musicName");

    preferences.begin("music_store", false);
    int numMusic = preferences.getInt("numMusic", 0);

    for (int i = 0; i < numMusic; ++i)
    {
        String storedMusicName = preferences.getString(("musicName" + String(i)).c_str(), "");
        if (storedMusicName == musicNameToDelete)
        {
            preferences.remove(("musicName" + String(i)).c_str());
            preferences.remove(("musicId" + String(i)).c_str());

            for (int j = i; j < numMusic - 1; ++j)
            {
                String nextMusicName = preferences.getString(("musicName" + String(j + 1)).c_str(), "");
                String nextMusicId = preferences.getString(("musicId" + String(j + 1)).c_str(), "");

                preferences.putString(("musicName" + String(j)).c_str(), nextMusicName);
                preferences.putString(("musicId" + String(j)).c_str(), nextMusicId);
            }

            preferences.remove(("musicName" + String(numMusic - 1)).c_str());
            preferences.remove(("musicId" + String(numMusic - 1)).c_str());
            preferences.putInt("numMusic", numMusic - 1);
            u8g2.setCursor(0, 11 + 12);
            u8g2.print("音乐删除成功！");
            Serial.println("Success Delete Music!");

            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Music Configuration</title></head><body><h1>Music Deleted!</h1><p>The music has been deleted.</p><p><a href='/'>Go Back</a></p></body></html>");
            preferences.end();
            return;
        }
    }
    u8g2.setCursor(0, 11 + 12);
    u8g2.print("该音乐不存在！");
    Serial.println("Fail to Delete Music!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Music Configuration</title></head><body><h1>Music Not Found!</h1><p>The specified music was not found.</p><p><a href='/'>Go Back</a></p></body></html>");
    preferences.end();
}
// 显示已有音乐信息逻辑
void handleListMusic(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Music Configuration</title></head><body><h1>Saved Music</h1><ul>";

    preferences.begin("music_store", true);
    int numMusic = preferences.getInt("numMusic", 0);

    for (int i = 0; i < numMusic; ++i)
    {
        String musicName = preferences.getString(("musicName" + String(i)).c_str(), "");
        String musicId = preferences.getString(("musicId" + String(i)).c_str(), "");
        html += "<li>musicName" + String(i) + ": " + musicName + " " + musicId + "</li>";
    }

    html += "</ul><p><a href='/'>Go Back</a></p></body></html>";

    request->send(200, "text/html", html);
    preferences.end();
}