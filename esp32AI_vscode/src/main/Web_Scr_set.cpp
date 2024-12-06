#include "Web_Scr_set.h"

// AP模式的SSID和密码
const char *ap_ssid = "ESP32-Setup";
const char *ap_password = "12345678";
// Web服务器和Preferences对象
AsyncWebServer server(80);
Preferences preferences;

bool isWebServerStarted = false;
bool isSoftAPStarted = false;

void openWeb()
{
    // 网络连接失败，启动 AP 模式创建热点用于配网和音乐信息添加
    WiFi.softAP(ap_ssid, ap_password);
    isSoftAPStarted = true;
    Serial.println("Started Access Point");
    // 启动 Web 服务器
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiManagement);
    server.on("/music", HTTP_GET, handleMusicManagement);
    server.on("/model", HTTP_GET, handleLLMManagement);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/delete", HTTP_POST, handleDelete);
    server.on("/list", HTTP_GET, handleList);
    server.on("/saveMusic", HTTP_POST, handleSaveMusic);
    server.on("/deleteMusic", HTTP_POST, handleDeleteMusic);
    server.on("/listMusic", HTTP_GET, handleListMusic);
    server.on("/saveLLM", HTTP_POST, handleSaveLLM);
    server.on("/listLLM", HTTP_GET, handleListLLM);
    
    server.begin();
    isWebServerStarted = true;
    Serial.println("WebServer started, waiting for configuration...");
}
// 处理根路径的请求
void handleRoot(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32配置中心</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}</style></head><body><div class='container'><h1>ESP32配置中心</h1><a href='/wifi'class='button'>Wi-Fi配置</a><a href='/music'class='button'>音乐配置</a><a href='/model'class='button'>大模型配置</a></div></body></html>";
    request->send(200, "text/html", html);
}
// wifi配置界面
void handleWifiManagement(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>Wi-Fi配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password']{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}</style></head><body><div class='container'><h1>Wi-Fi 配置</h1><form action='/save'method='post'><label for='ssid'>Wi-Fi 名称:</label><input type='text'id='ssid'name='ssid'required><label for='password'>Wi-Fi 密码:</label><input type='password'id='password'name='password'required><input type='submit'value='保存 / 更新'class='button'></form><form action='/delete'method='post'><label for='ssid_delete'>要删除的 Wi-Fi 名称:</label><input type='text'id='ssid_delete'name='ssid'><input type='submit'value='删除'class='button'></form><a href='/list'class='button'>已保存的 Wi-Fi 网络</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";
    request->send(200, "text/html", html);
}
// 音乐信息配置界面
void handleMusicManagement(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>音乐配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password']{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}</style></head><body><div class='container'><h1>音乐配置</h1><form action='/saveMusic'method='post'><label for='musicName'>音乐名称:</label><input type='text'id='musicName'name='musicName'required><label for='musicId'>音乐 ID:</label><input type='text'id='musicId'name='musicId'required><input type='submit'value='保存 / 更新'class='button'></form><form action='/deleteMusic'method='post'><label for='musicName'>要删除的音乐名称:</label><input type='text'id='musicName'name='musicName'><input type='submit'value='删除'class='button'></form><a href='/listMusic'class='button'>已保存的音乐</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";
    request->send(200, "text/html", html);
}
// 大模型参数配置界面
void handleLLMManagement(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>大模型配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password']{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}select{padding:10px;height:40px;border:1px solid#ccc;border-radius:5px;-webkit-appearance:none}</style></head><body><div class='container'><h1>大模型配置</h1><form action='/saveLLM'method='post'><label for='modelSupplier'>模型供应商:</label><select id='modelSupplier'name='modelSupplier'><option value='ChatGPT'>ChatGPT</option><option value='Claude'>Claude</option><option value='Gemini'>Gemini</option><option value='Grok'>Grok</option><option value='Mistral'>Mistral</option><option value='豆包大模型'>豆包大模型</option><option value='月之暗面'>月之暗面</option><option value='通义千问'>通义千问</option><option value='讯飞星火'>讯飞星火</option><option value='腾讯混元'>腾讯混元</option><option value='百川智能'>百川智能</option><option value='BigModel'>BigModel</option><option value='零一万物'>零一万物</option><option value='DeepSeek'>DeepSeek</option><option value='Ollama'>Ollama</option></select><label for='apiUrl'>API URL:</label><input type='text'id='apiUrl'name='apiUrl'required><label for='apiKey'>API KEY:</label><input type='text'id='apiKey'name='apiKey'required><input type='submit'value='保存 / 更新'class='button'></form><a href='/listLLM'class='button'>已保存的LLM参数</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";
    request->send(200, "text/html", html);
}

// 添加或更新wifi信息逻辑
void handleSave(AsyncWebServerRequest *request)
{
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

            Serial.println("Succeess Update!");

            String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi配置</title><script>alert('更新成功！');window.location.href='/wifi';</script></head><body></body></html>";
            request->send(200, "text/html", html);
            preferences.end();
            return;
        }
    }

    preferences.putString(("ssid" + String(numNetworks)).c_str(), ssid);
    preferences.putString(("password" + String(numNetworks)).c_str(), password);
    preferences.putInt("numNetworks", numNetworks + 1);
    Serial.println("Succeess Save!");

    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi配置</title><script>alert('添加成功！');window.location.href='/wifi';</script></head><body></body></html>";
    request->send(200, "text/html", html);
    preferences.end();
}
// 删除wifi信息逻辑
void handleDelete(AsyncWebServerRequest *request)
{
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
            Serial.println("Succeess Delete!");

            String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi配置</title><script>alert('删除成功！');window.location.href='/wifi';</script></head><body></body></html>";
            request->send(200, "text/html", html);
            preferences.end();
            return;
        }
    }
    Serial.println("Fail to Delete!");

    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi配置</title><script>alert('删除失败，该wifi不存在！');window.location.href='/wifi';</script></head><body></body></html>";
    request->send(200, "text/html", html);
    preferences.end();
}
// 显示已有wifi信息逻辑
void handleList(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 Wifi配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;position:relative}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px;height:60vh;overflow-y:auto;margin-bottom:60px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}ul{list-style-type:none;padding:0;margin:0}li{background-color:#f9f9f9;margin:5px 0;padding:10px;border-radius:5px;box-shadow:0 0 5px rgba(0,0,0,0.1)}.fixed-buttons{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000;display:flex;gap:10px}</style></head><body><div class='container'><h1>已保存的Wi-Fi网络</h1>";

    preferences.begin("wifi_store", true);
    int numNetworks = preferences.getInt("numNetworks", 0);

    for (int i = 0; i < numNetworks; ++i)
    {
        String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
        String password = preferences.getString(("password" + String(i)).c_str(), "");
        html += "<div><label for='apiKey'>wifi:</label><input type='text'id='apiKey'value='" + ssid + "'disabled style='width: 120px;'><label for='apiUrl'>密码:</label><input type='text'id='apiUrl'value='" + password + "'disabled style='width: 120px;'></div>";
    }

    html += "</div><div class='fixed-buttons'><a href='/wifi'class='button'>返回上一页</a><a href='/'class='button'>返回首页</a></div></body></html>";

    request->send(200, "text/html", html);
    preferences.end();
}

// 添加或更新音乐信息逻辑
void handleSaveMusic(AsyncWebServerRequest *request)
{
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
            Serial.println("Success Update Music!");

            String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 音乐配置</title><script>alert('更新成功！');window.location.href='/music';</script></head><body></body></html>";
            request->send(200, "text/html", html);
            preferences.end();
            return;
        }
    }

    preferences.putString(("musicName" + String(numMusic)).c_str(), musicName);
    preferences.putString(("musicId" + String(numMusic)).c_str(), musicId);
    preferences.putInt("numMusic", numMusic + 1);
    Serial.println("Success Save Music!");

    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 音乐配置</title><script>alert('添加成功！');window.location.href='/music';</script></head><body></body></html>";
    request->send(200, "text/html", html);
    preferences.end();
}
// 删除音乐信息逻辑
void handleDeleteMusic(AsyncWebServerRequest *request)
{
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
            Serial.println("Success Delete Music!");

            String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 音乐配置</title><script>alert('删除成功！');window.location.href='/music';</script></head><body></body></html>";
            request->send(200, "text/html", html);
            preferences.end();
            return;
        }
    }
    Serial.println("Fail to Delete Music!");

    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 音乐配置</title><script>alert('删除失败，该音乐不存在！');window.location.href='/';</script></head><body></body></html>";
    request->send(200, "text/html", html);
    preferences.end();
}
// 显示已有音乐信息逻辑
void handleListMusic(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32音乐配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;position:relative}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px;height:60vh;overflow-y:auto;margin-bottom:60px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}ul{list-style-type:none;padding:0;margin:0}li{background-color:#f9f9f9;margin:5px 0;padding:10px;border-radius:5px;box-shadow:0 0 5px rgba(0,0,0,0.1)}.fixed-buttons{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000;display:flex;gap:10px}</style></head><body><div class='container'><h1>已保存的音乐</h1>";

    preferences.begin("music_store", true);
    int numMusic = preferences.getInt("numMusic", 0);

    for (int i = 0; i < numMusic; ++i)
    {
        String musicName = preferences.getString(("musicName" + String(i)).c_str(), "");
        String musicId = preferences.getString(("musicId" + String(i)).c_str(), "");
        html += "<div><label for='apiKey'>名称:</label><input type='text'id='apiKey'value='" + musicName + "'disabled style='width: 120px;'><label for='apiUrl'>ID:</label><input type='text'id='apiUrl'value='" + musicId + "'disabled style='width: 120px;'></div>";
    }

    html += "</div><div class='fixed-buttons'><a href='/music'class='button'>返回上一页</a><a href='/'class='button'>返回首页</a></div></body></html>";

    request->send(200, "text/html", html);
    preferences.end();
}


void handleSaveLLM(AsyncWebServerRequest *request)
{
    Serial.println("Start Save LLM!");
    String llmName = request->arg("modelSupplier");
    String apiKey = request->arg("apiKey");
    String apiUrl = request->arg("apiUrl");

    preferences.begin("llm_store", false);
    int numLLM = preferences.getInt("numLLM", 0);

    for (int i = 0; i < numLLM; ++i)
    {
        String storedLLmName = preferences.getString(("llmName" + String(i)).c_str(), "");
        if (storedLLmName == llmName)
        {
            preferences.putString(("llmApiKey" + String(i)).c_str(), apiKey);
            preferences.putString(("llmApiUrl" + String(i)).c_str(), apiUrl);
            Serial.println("Success Update LLM!");

            String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 大模型配置</title><script>alert('更新成功！');window.location.href='/model';</script></head><body></body></html>";
            request->send(200, "text/html", html);
            preferences.end();
            return;
        }
    }
    
    preferences.putString(("llmName" + String(numLLM)).c_str(), llmName);
    preferences.putString(("llmApiKey" + String(numLLM)).c_str(), apiKey);
    preferences.putString(("llmApiUrl" + String(numLLM)).c_str(), apiUrl);
    preferences.putInt("numLLM", numLLM + 1);
    Serial.println("Success Save LLM!");

    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32 大模型配置</title><script>alert('添加成功！');window.location.href='/model';</script></head><body></body></html>";
    request->send(200, "text/html", html);
    preferences.end();
}

void handleListLLM(AsyncWebServerRequest *request) 
{
    String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32大模型配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;position:relative}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px;height:60vh;overflow-y:auto;margin-bottom:60px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}ul{list-style-type:none;padding:0;margin:0}li{background-color:#f9f9f9;margin:5px 0;padding:10px;border-radius:5px;box-shadow:0 0 5px rgba(0,0,0,0.1)}.fixed-buttons{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000;display:flex;gap:10px}</style></head><body><div class='container'><h1>已保存的LLM参数</h1>";

    preferences.begin("llm_store", true);
    int numLLM = preferences.getInt("numLLM", 0);

    for (int i = 0; i < numLLM; ++i)
    {
        String llmName = preferences.getString(("llmName" + String(i)).c_str(), "");
        String llmApiKey = preferences.getString(("llmApiKey" + String(i)).c_str(), "");
        String llmApiUrl = preferences.getString(("llmApiUrl" + String(i)).c_str(), "");
        html += "<li>" + llmName + "</li><div><label for='apiKey'>API_KEY:</label><input type='text'id='apiKey'value='" + llmApiKey + "'disabled style='width: 300px;'></div><div><label for='apiUrl'>API_URL:</label><input type='text'id='apiUrl'value='" + llmApiUrl + "'disabled style='width: 300px;'></div>";
    }

    html += "</div><div class='fixed-buttons'><a href='/model'class='button'>返回上一页</a><a href='/'class='button'>返回首页</a></div></body></html>";

    request->send(200, "text/html", html);
    preferences.end();
}