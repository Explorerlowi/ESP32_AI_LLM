#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include "Audio1.h"
#include "Audio2.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "bizhi.h"
using namespace websockets;

// 定义引脚和常量
#define key 0   // 按键引脚
#define ADC 32  // ADC引脚
#define led3 21 // LED3引脚
#define led2 14 // LED2引脚
#define led1 19 // LED1引脚

// AP 模式的SSID和密码
const char *ap_ssid = "ESP32-Setup";
const char *ap_password = "12345678";
// Web服务器和Preferences对象
AsyncWebServer server(80);
Preferences preferences;

// 星火大模型的账号参数
String APPID = "";                             // 星火大模型的App ID
String APIKey = "";    // API Key
String APISecret = ""; // API Secret

// 定义一些全局变量
bool ledstatus = true;
bool startPlay = false;
bool lastsetence = false;
bool isReady = false;
unsigned long urlTime = 0;
unsigned long pushTime = 0;
int mainStatus = 0;
int receiveFrame = 0;
int noise = 50;
HTTPClient https; // 创建一个HTTP客户端对象

hw_timer_t *timer = NULL; // 定义硬件定时器对象

uint8_t adc_start_flag = 0;
uint8_t adc_complete_flag = 0;

// 屏幕引脚定义
#define TFT_CS 5
#define TFT_RST 12
#define TFT_DC 2
#define TFT_SCLK 18
#define TFT_MOSI 23

// 创建屏幕对象
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// 创建音频对象
Audio1 audio1;
Audio2 audio2(false, 3, I2S_NUM_1); // 参数: 是否使用SD卡, 音量, I2S端口号

// 定义I2S引脚
#define I2S_DOUT 27 // DIN引脚
#define I2S_BCLK 26 // BCLK引脚
#define I2S_LRC 25  // LRC引脚

// 函数声明
void handleRoot(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);
void gain_token(void);
void getText(String role, String content);
void checkLen(JsonArray textArray);
int getLength(JsonArray textArray);
float calculateRMS(uint8_t *buffer, int bufferSize);
void ConnServer();
void ConnServer1();

// 创建动态JSON文档对象和数组
DynamicJsonDocument doc(4096);
JsonArray text = doc.to<JsonArray>();

// 定义字符串变量
String url = "";
String url1 = "";
String Date = "";

// 函数声明
DynamicJsonDocument gen_params(const char *appid, const char *domain);
void displayWrappedText(const string &text1, int x, int y, int maxWidth);

String askquestion = "";
String Answer = ""; // 用于语音合成，要分段
// String answerTemp = "";     //用于文字输出，不分段
std::vector<String> subAnswers;
int subindex = 0;
String text_temp = "";

// 星火大模型参数
const char *appId1 = ""; // 替换为自己的星火大模型参数
const char *domain1 = "4.0Ultra";
const char *websockets_server = "ws://spark-api.xf-yun.com/v4.0/chat";
const char *websockets_server1 = "ws://ws-api.xfyun.cn/v2/iat";
using namespace websockets; // 使用WebSocket命名空间

// 创建WebSocket客户端对象
WebsocketsClient webSocketClient;
WebsocketsClient webSocketClient1;

int loopcount = 0; // 循环计数器
int flag = 0;       // 用来确保subAnswer1一定是大模型回答最开始的内容

// 移除讯飞星火回复中没用的符号
void removeChars(const char *input, char *output, const char *removeSet)
{
    int j = 0;
    for (int i = 0; input[i] != '\0'; ++i)
    {
        bool shouldRemove = false;
        for (int k = 0; removeSet[k] != '\0'; ++k)
        {
            if (input[i] == removeSet[k])
            {
                shouldRemove = true;
                break;
            }
        }
        if (!shouldRemove)
        {
            output[j++] = input[i];
        }
    }
    output[j] = '\0'; // 结束符
}

// 将回复的文本转成语音
void onMessageCallback(WebsocketsMessage message)
{
    // 创建一个静态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节
    StaticJsonDocument<4096> jsonDocument;

    // 解析收到的JSON数据
    DeserializationError error = deserializeJson(jsonDocument, message.data());

    // 如果解析没有错误
    if (!error)
    {
        // 从JSON数据中获取返回码
        int code = jsonDocument["header"]["code"];

        // 如果返回码不为0，表示出错
        if (code != 0)
        {
            // 输出错误信息和完整的JSON数据
            Serial.print("sth is wrong: ");
            Serial.println(code);
            Serial.println(message.data());

            // 关闭WebSocket客户端
            webSocketClient.close();
        }
        else
        {
            // 增加接收到的帧数计数器
            receiveFrame++;
            Serial.print("receiveFrame:");
            Serial.println(receiveFrame);

            // 获取JSON数据中的payload部分
            JsonObject choices = jsonDocument["payload"]["choices"];

            // 获取status状态
            int status = choices["status"];

            // 获取文本内容
            const char *content = choices["text"][0]["content"];
            const char *removeSet = "\n*$"; // 定义需要移除的符号集
            // 计算新字符串的最大长度
            int length = strlen(content) + 1;
            char *cleanedContent = new char[length];
            removeChars(content, cleanedContent, removeSet);
            Serial.println(cleanedContent);

            // 将内容追加到Answer字符串中
            Answer += cleanedContent;
            content = "";
            // 释放分配的内存
            delete[] cleanedContent;
            // answerTemp += content;

            // 如果Answer的长度超过180且音频没有播放
            if (Answer.length() >= 180 && (audio2.isplaying == 0) && flag == 0)
            {
                if (Answer.length() >= 300)
                {
                    // 查找第一个句号的位置
                    int firstPeriodIndex = Answer.indexOf("。");
                    // 如果找到句号
                    if (firstPeriodIndex != -1)
                    {
                        // 提取完整的句子并播放
                        String subAnswer1 = Answer.substring(0, firstPeriodIndex + 3);
                        Serial.print("subAnswer1:");
                        Serial.println(subAnswer1);

                        // 获取最终转换的文本
                        getText("assistant", subAnswer1);
                        flag = 1;

                        // 将提取的句子转换为语音
                        audio2.connecttospeech(subAnswer1.c_str(), "zh");
                        // 更新Answer，去掉已处理的部分
                        Answer = Answer.substring(firstPeriodIndex + 3);
                        subAnswer1.clear();
                    }
                }
                else
                {
                    String subAnswer1 = Answer.substring(0, Answer.length());
                    Serial.print("subAnswer1:");
                    Serial.println(subAnswer1);
                    subAnswer1.clear();

                    // 获取最终转换的文本
                    getText("assistant", Answer);
                    flag = 1;

                    audio2.connecttospeech(Answer.c_str(), "zh");
                    Answer = Answer.substring(Answer.length());
                    // 设置播放开始标志
                    startPlay = true;
                }
            }
            // 存储多段子音频
            else if (Answer.length() >= 180)
            {
                if (Answer.length() >= 300)
                {
                    // 查找第一个句号的位置
                    int firstPeriodIndex = Answer.indexOf("。");
                    // 如果找到句号
                    if (firstPeriodIndex != -1)
                    {
                        subAnswers.push_back(Answer.substring(0, firstPeriodIndex + 3));
                        Serial.print("subAnswer");
                        Serial.print(subAnswers.size() + 1);
                        Serial.print("：");
                        Serial.println(subAnswers[subAnswers.size() - 1]);

                        Answer = Answer.substring(firstPeriodIndex + 3);
                    }
                }
                else
                {
                    subAnswers.push_back(Answer.substring(0, Answer.length()));
                    Serial.print("subAnswer");
                    Serial.print(subAnswers.size() + 1);
                    Serial.print("：");
                    Serial.println(subAnswers[subAnswers.size() - 1]);

                    Answer = Answer.substring(Answer.length());
                }
            }

            // 如果status为2（回复的内容接收完成），且回复的内容小于180字节
            if (status == 2 && flag == 0)
            {
                // 显示最终转换的文本
                getText("assistant", Answer);
                // 播放最终转换的文本
                audio2.connecttospeech(Answer.c_str(), "zh");
            }
        }
    }
}

// 问题发送给大模型
void onEventsCallback(WebsocketsEvent event, String data)
{
    // 当WebSocket连接打开时触发
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        // 向串口输出提示信息
        Serial.println("Send message to server0!");

        // 生成连接参数的JSON文档
        DynamicJsonDocument jsonData = gen_params(appId1, domain1);

        // 将JSON文档序列化为字符串
        String jsonString;
        serializeJson(jsonData, jsonString);

        // 向串口输出生成的JSON字符串
        Serial.println(jsonString);

        // 通过WebSocket客户端发送JSON字符串到服务器
        webSocketClient.send(jsonString);
    }
    // 当WebSocket连接关闭时触发
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        // 向串口输出提示信息
        Serial.println("Connnection0 Closed");
    }
    // 当收到Ping消息时触发
    else if (event == WebsocketsEvent::GotPing)
    {
        // 向串口输出提示信息
        Serial.println("Got a Ping!");
    }
    // 当收到Pong消息时触发
    else if (event == WebsocketsEvent::GotPong)
    {
        // 向串口输出提示信息
        Serial.println("Got a Pong!");
    }
}

// 将接收到的语音转成文本
void onMessageCallback1(WebsocketsMessage message)
{
    // 创建一个静态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节
    StaticJsonDocument<4096> jsonDocument;

    // 解析收到的JSON数据
    DeserializationError error = deserializeJson(jsonDocument, message.data());

    if (error)
    {
        // 如果解析出错，输出错误信息和收到的消息数据
        Serial.println("error:");
        Serial.println(error.c_str());
        Serial.println(message.data());
        return;
    }
    // 如果解析没有错误

    // 从JSON数据中获取返回码
    int code = jsonDocument["code"];
    // 如果返回码不为0，表示出错
    if (code != 0)
    {
        // 输出错误码和完整的JSON数据
        Serial.println(code);
        Serial.println(message.data());

        // 关闭WebSocket客户端
        webSocketClient1.close();
    }
    else
    {
        // 输出收到的讯飞云返回消息
        Serial.println("xunfeiyun return message:");
        Serial.println(message.data());

        // 获取JSON数据中的结果部分，并提取文本内容
        JsonArray ws = jsonDocument["data"]["result"]["ws"].as<JsonArray>();

        for (JsonVariant i : ws)
        {
            for (JsonVariant w : i["cw"].as<JsonArray>())
            {
                askquestion += w["w"].as<String>();
            }
        }

        // 输出提取的问句
        Serial.println(askquestion);

        // 获取状态码
        if (jsonDocument["data"]["status"] == 2)
        {
            // 如果状态码为2，表示消息处理完成
            Serial.println("status == 2");
            webSocketClient1.close();

            // 如果问句为空，播放错误提示语音
            if (askquestion == "")
            {
                askquestion = "sorry, i can't hear you";
                audio2.connecttospeech(askquestion.c_str(), "zh");
            }
            else
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                // 显示图片
                // tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);
                tft.setCursor(0, 0);
                // 处理一般的问答请求
                getText("user", askquestion);
                Serial.print("text:");
                Serial.println(text);
                Answer = "";
                lastsetence = false;
                isReady = true;
                ConnServer();
            }
        }
    }
}

// 录音
void onEventsCallback1(WebsocketsEvent event, String data)
{
    // 当WebSocket连接打开时触发
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        // 向串口输出提示信息
        Serial.println("Send message to xunfeiyun");

        // 打开LED指示灯
        digitalWrite(led2, HIGH);

        // 初始化变量
        int silence = 0;
        int firstframe = 1;
        int j = 0;
        int voicebegin = 0;
        int voice = 0;

        // 创建一个JSON文档对象
        DynamicJsonDocument doc(2500);

        // 清空屏幕
        tft.fillScreen(ST77XX_WHITE);
        // 显示图片
        tft.drawRGBBitmap(0, 0, kunkun_0, 128, 160);
        u8g2.setCursor(0, 11);
        u8g2.print("请说话。");

        // 无限循环，用于录制和发送音频数据
        while (1)
        {
            // 清空JSON文档
            doc.clear();

            // 创建data对象
            JsonObject data = doc.createNestedObject("data");

            // 录制音频数据
            audio1.Record();

            // 计算音频数据的RMS值
            float rms = calculateRMS((uint8_t *)audio1.wavData[0], 1280);
            printf("%d %f\n", 0, rms);

            // 判断是否为噪音
            if (rms < noise)
            {
                if (voicebegin == 1)
                {
                    silence++;
                }
            }
            else
            {
                voice++;
                if (voice >= 5)
                {
                    voicebegin = 1;
                }
                else
                {
                    voicebegin = 0;
                }
                silence = 0;
            }

            // 如果静音达到8个周期，发送结束标志的音频数据
            if (silence == 8)
            {
                data["status"] = 2;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
                digitalWrite(led2, LOW);
                delay(40);
                break;
            }

            // 处理第一帧音频数据
            if (firstframe == 1)
            {
                data["status"] = 0;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";
                j++;

                JsonObject common = doc.createNestedObject("common");
                common["app_id"] = appId1;

                JsonObject business = doc.createNestedObject("business");
                business["domain"] = "iat";
                business["language"] = "zh_cn";
                business["accent"] = "mandarin";
                business["vinfo"] = 1;
                business["vad_eos"] = 1000;

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
                firstframe = 0;
                delay(40);
            }
            else
            {
                // 处理后续帧音频数据
                data["status"] = 1;
                data["format"] = "audio/L16;rate=8000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
                delay(40);
            }
        }
    }
    // 当WebSocket连接关闭时触发
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        // 向串口输出提示信息
        Serial.println("Connnection1 Closed");
    }
    // 当收到Ping消息时触发
    else if (event == WebsocketsEvent::GotPing)
    {
        // 向串口输出提示信息
        Serial.println("Got a Ping!");
    }
    // 当收到Pong消息时触发
    else if (event == WebsocketsEvent::GotPong)
    {
        // 向串口输出提示信息
        Serial.println("Got a Pong!");
    }
}

void ConnServer()
{
    // 向串口输出WebSocket服务器的URL
    Serial.println("url:" + url);

    // 设置WebSocket客户端的消息回调函数
    webSocketClient.onMessage(onMessageCallback);

    // 设置WebSocket客户端的事件回调函数
    webSocketClient.onEvent(onEventsCallback);

    // 开始连接WebSocket服务器
    Serial.println("Begin connect to server0......");

    // 尝试连接到WebSocket服务器
    if (webSocketClient.connect(url.c_str()))
    {
        // 如果连接成功，输出成功信息
        Serial.println("Connected to server0!");
    }
    else
    {
        // 如果连接失败，输出失败信息
        Serial.println("Failed to connect to server0!");
    }
}

void ConnServer1()
{
    // Serial.println("url1:" + url1);
    webSocketClient1.onMessage(onMessageCallback1);
    webSocketClient1.onEvent(onEventsCallback1);
    // Connect to WebSocket
    Serial.println("Begin connect to server1......");
    if (webSocketClient1.connect(url1.c_str()))
    {
        Serial.println("Connected to server1!");
    }
    else
    {
        Serial.println("Failed to connect to server1!");
    }
}

void voicePlay()
{
    // 检查音频是否正在播放以及回答内容是否为空
    if ((audio2.isplaying == 0) && (Answer != "" || subindex < subAnswers.size()))
    {
        if (subindex < subAnswers.size())
        {
            if (text_temp != "" && flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                // 显示图片
                // tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);
                // 显示剩余的文字
                displayWrappedText(text_temp.c_str(), 0, 11, 128);
                text_temp = "";
                displayWrappedText(subAnswers[subindex].c_str(), u8g2.getCursorX(), u8g2.getCursorY(), 128);
            }
            else if (flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                displayWrappedText(subAnswers[subindex].c_str(), 0, 11, 128);
            }
            audio2.connecttospeech(subAnswers[subindex].c_str(), "zh");
            subindex++;
        }
        else
        {
            if (text_temp != "" && flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                // 显示图片
                // tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);
                // 显示剩余的文字
                displayWrappedText(text_temp.c_str(), 0, 11, 128);
                text_temp = "";
                displayWrappedText(Answer.c_str(), u8g2.getCursorX(), u8g2.getCursorY(), 128);
            }
            else if (flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                displayWrappedText(Answer.c_str(), 0, 11, 128);
            }
            audio2.connecttospeech(Answer.c_str(), "zh");
            Answer = "";
        }
        // 设置开始播放标志
        startPlay = true;
    }
    else
    {
        // 如果音频正在播放或回答内容为空，不做任何操作
    }
}

int wifiConnect()
{
    // 断开当前WiFi连接
    WiFi.disconnect(true);

    int numNetworks = preferences.getInt("numNetworks", 0);
    if (numNetworks == 0)
    {
        // 在屏幕上输出提示信息
        u8g2.setCursor(0, u8g2.getCursorY() + 12);
        u8g2.print("无任何wifi存储信息！");
        displayWrappedText("请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, 128);
        return 0;
    }

    // 获取存储的 WiFi 配置
    for (int i = 0; i < numNetworks; ++i)
    {
        String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
        String password = preferences.getString(("password" + String(i)).c_str(), "");

        // 尝试连接到存储的 WiFi 网络
        if (ssid.length() > 0 && password.length() > 0)
        {
            Serial.print("Connecting to ");
            Serial.println(ssid);
            Serial.print("password:");
            Serial.println(password);
            // 在屏幕上显示每个网络的连接情况
            u8g2.setCursor(0, u8g2.getCursorY()+12);
            u8g2.print(ssid);

            uint8_t count = 0;
            WiFi.begin(ssid.c_str(), password.c_str());
            // 等待WiFi连接成功
            while (WiFi.status() != WL_CONNECTED)
            {
                // 闪烁LED以指示连接状态
                digitalWrite(led1, ledstatus);
                ledstatus = !ledstatus;
                count++;

                // 如果尝试连接超过30次，则认为连接失败
                if (count >= 30)
                {
                    Serial.printf("\r\n-- wifi connect fail! --\r\n");
                    // 在屏幕上显示连接失败信息
                    u8g2.setCursor(u8g2.getCursorX()+6, u8g2.getCursorY());
                    u8g2.print("Failed!");
                    break;
                }

                // 等待100毫秒
                vTaskDelay(100);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                // 向串口输出连接成功信息和IP地址
                Serial.printf("\r\n-- wifi connect success! --\r\n");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());

                // 输出当前空闲堆内存大小
                Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
                // 在屏幕上显示连接成功信息
                u8g2.setCursor(u8g2.getCursorX()+6, u8g2.getCursorY());
                u8g2.print("Connected!");
                return 1;
            }
        }
    }
    // 清空屏幕
    tft.fillScreen(ST77XX_WHITE);
    // 在屏幕上输出提示信息
    u8g2.setCursor(0, 11);
    u8g2.print("网络连接失败！请检查");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("网络设备，确认可用后");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("重启设备以建立连接！");
    displayWrappedText("或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, 128);
    return 0;
}

// 处理根路径的请求
void handleRoot(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title><style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; } h1 { color: #333; } form { display: inline-block; margin-top: 20px; } input[type='text'], input[type='password'] { padding: 10px; margin: 10px 0; width: 200px; } input[type='submit'], input[type='button'] { padding: 10px 20px; margin: 10px 5px; border: none; background-color: #333; color: white; cursor: pointer; } input[type='submit']:hover, input[type='button']:hover { background-color: #555; }</style></head><body><h1>ESP32 Wi-Fi Configuration</h1><form action='/save' method='post'><label for='ssid'>Wi-Fi SSID:</label><br><input type='text' id='ssid' name='ssid'><br><label for='password'>Password:</label><br><input type='password' id='password' name='password'><br><input type='submit' value='Save'></form><form action='/delete' method='post'><label for='ssid'>Wi-Fi SSID to Delete:</label><br><input type='text' id='ssid' name='ssid'><br><input type='submit' value='Delete'></form><a href='/list'><input type='button' value='List Wi-Fi Networks'></a></body></html>";
    request->send(200, "text/html", html);
}

// 处理保存 WiFi 配置的请求
void handleSave(AsyncWebServerRequest *request)
{
    // 清空屏幕
    tft.fillScreen(ST77XX_WHITE);
    // 显示图片
    // tft.drawRGBBitmap(0, 0, kunkun_0, 128, 160);
    u8g2.setCursor(0, 11);
    u8g2.print("进入网络配置！");
    
    Serial.println("Start Save!");
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    int numNetworks = preferences.getInt("numNetworks", 0);

    // 检查是否已经存在相同的网络
    for (int i = 0; i < numNetworks; ++i)
    {
        String storedSsid = preferences.getString(("ssid" + String(i)).c_str(), "");
        if (storedSsid == ssid)
        {
            // 如果存在相同的网络，更新密码
            preferences.putString(("password" + String(i)).c_str(), password);
            u8g2.setCursor(0, 11+12);
            u8g2.print("wifi密码更新成功！");
            Serial.println("Succeess Update!");
            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Configuration Updated!</h1><p>The device will restart and attempt to connect to the updated network.</p><p><a href='/'>Go Back</a></p></body></html>");

            return;
        }
    }

    // 如果不存在相同的网络，添加新的网络
    preferences.putString(("ssid" + String(numNetworks)).c_str(), ssid);
    preferences.putString(("password" + String(numNetworks)).c_str(), password);
    preferences.putInt("numNetworks", numNetworks + 1);
    u8g2.setCursor(0, 11+12);
    u8g2.print("新wifi添加成功！");
    Serial.println("Succeess Save!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Configuration Saved!</h1><p>The device will restart and attempt to connect to the new network.</p><p><a href='/'>Go Back</a></p></body></html>");
}

// 处理删除 WiFi 配置的请求
void handleDelete(AsyncWebServerRequest *request)
{
    // 清空屏幕
    tft.fillScreen(ST77XX_WHITE);
    // 显示图片
    // tft.drawRGBBitmap(0, 0, kunkun_0, 128, 160);
    u8g2.setCursor(0, 11);
    u8g2.print("进入网络配置！");

    Serial.println("Start Delete!");
    String ssidToDelete = request->arg("ssid");

    int numNetworks = preferences.getInt("numNetworks", 0);

    // 查找并删除指定的网络
    for (int i = 0; i < numNetworks; ++i)
    {
        String storedSsid = preferences.getString(("ssid" + String(i)).c_str(), "");
        if (storedSsid == ssidToDelete)
        {
            // 删除网络
            preferences.remove(("ssid" + String(i)).c_str());
            preferences.remove(("password" + String(i)).c_str());

            // 将后面的网络信息往前移动
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
            u8g2.setCursor(0, 11+12);
            u8g2.print("wifi删除成功！");
            Serial.println("Succeess Delete!");

            request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Network Deleted!</h1><p>The network has been deleted. The device will restart to apply changes.</p><p><a href='/'>Go Back</a></p></body></html>");

            return;
        }
    }
    u8g2.setCursor(0, 11+12);
    u8g2.print("该wifi不存在！");
    Serial.println("Fail to Delete!");

    request->send(200, "text/html", "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Network Not Found!</h1><p>The specified network was not found.</p><p><a href='/'>Go Back</a></p></body></html>");
}

// 处理列出已保存的 WiFi 配置的请求
void handleList(AsyncWebServerRequest *request)
{
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Wi-Fi Configuration</title></head><body><h1>Saved Wi-Fi Networks</h1><ul>";

    int numNetworks = preferences.getInt("numNetworks", 0);

    for (int i = 0; i < numNetworks; ++i)
    {
        String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
        String password = preferences.getString(("password" + String(i)).c_str(), "");
        html += "<li>ssid" + String(i) + ": " + ssid + " " + password + "</li>";
    }

    html += "</ul><p><a href='/'>Go Back</a></p></body></html>";

    request->send(200, "text/html", html);
}

String getUrl(String Spark_url, String host, String path, String Date)
{
    // 拼接签名原始字符串
    String signature_origin = "host: " + host + "\n";
    signature_origin += "date: " + Date + "\n";
    signature_origin += "GET " + path + " HTTP/1.1";
    // 示例：signature_origin="host: spark-api.xf-yun.com\ndate: Mon, 04 Mar 2024 19:23:20 GMT\nGET /v3.5/chat HTTP/1.1";

    // 使用 HMAC-SHA256 进行加密
    unsigned char hmac[32];                                 // 存储HMAC结果
    mbedtls_md_context_t ctx;                               // HMAC上下文
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;          // 使用SHA256哈希算法
    const size_t messageLength = signature_origin.length(); // 签名原始字符串的长度
    const size_t keyLength = APISecret.length();            // 密钥的长度

    // 初始化HMAC上下文
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    // 设置HMAC密钥
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)APISecret.c_str(), keyLength);
    // 更新HMAC上下文
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)signature_origin.c_str(), messageLength);
    // 完成HMAC计算
    mbedtls_md_hmac_finish(&ctx, hmac);
    // 释放HMAC上下文
    mbedtls_md_free(&ctx);

    // 将HMAC结果进行Base64编码
    String signature_sha_base64 = base64::encode(hmac, sizeof(hmac) / sizeof(hmac[0]));

    // 替换Date字符串中的特殊字符
    Date.replace(",", "%2C");
    Date.replace(" ", "+");
    Date.replace(":", "%3A");

    // 构建Authorization原始字符串
    String authorization_origin = "api_key=\"" + APIKey + "\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"" + signature_sha_base64 + "\"";

    // 将Authorization原始字符串进行Base64编码
    String authorization = base64::encode(authorization_origin);

    // 构建最终的URL
    String url = Spark_url + '?' + "authorization=" + authorization + "&date=" + Date + "&host=" + host;

    // 向串口输出生成的URL
    Serial.println(url);

    // 返回生成的URL
    return url;
}

void getTimeFromServer()
{
    // 定义用于获取时间的URL
    String timeurl = "https://www.baidu.com";

    // 创建HTTPClient对象
    HTTPClient http;

    // 初始化HTTP连接
    http.begin(timeurl);

    // 定义需要收集的HTTP头字段
    const char *headerKeys[] = {"Date"};

    // 设置要收集的HTTP头字段
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));

    // 发送HTTP GET请求
    int httpCode = http.GET();

    // 从HTTP响应头中获取Date字段
    Date = http.header("Date");

    // 输出获取到的Date字段到串口
    Serial.println(Date);

    // 结束HTTP连接
    http.end();

    // 根据实际情况可以添加延时，以便避免频繁请求
    // delay(50); // 可以根据实际情况调整延时时间
}

void setup()
{
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 配置引脚模式
    // 配置按键引脚为上拉输入模式
    pinMode(key, INPUT_PULLUP);
    // 配置GPIO34为上拉输入模式
    pinMode(34, INPUT_PULLUP);
    // 配置GPIO35为上拉输入模式
    pinMode(35, INPUT_PULLUP);
    // 配置led1引脚为输出模式
    pinMode(led1, OUTPUT);
    // 配置led2引脚为输出模式
    pinMode(led2, OUTPUT);
    // 配置led3引脚为输出模式
    pinMode(led3, OUTPUT);

    // 初始化屏幕
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_WHITE);

    // 初始化U8g2
    u8g2.begin(tft);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
    u8g2.setFontMode(1);                    // 设置字体模式为1，以支持中文字符
    u8g2.setForegroundColor(ST77XX_BLACK);  // 设置字体颜色为黑色

    // 显示图片
    // tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);

    // 显示文字
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextWrap(true);
    u8g2.setCursor(0, 11);
    u8g2.print("已开机！");

    // 初始化音频模块audio1
    audio1.init();

    // 启动 AP 模式创建热点
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Started Access Point");
    // 启动 Web 服务器
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/delete", HTTP_POST, handleDelete);
    server.on("/list", HTTP_GET, handleList);
    server.begin();
    Serial.println("WebServer started, waiting for configuration...");

    // 初始化 Preferences
    preferences.begin("wifi-config");

    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("正在连接网络······");
    int result = wifiConnect();

    // 从服务器获取当前时间
    getTimeFromServer();

    // 设置音频输出引脚和音量
    audio2.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio2.setVolume(50);

    // 使用当前日期生成WebSocket连接的URL
    url = getUrl("ws://spark-api.xf-yun.com/v4.0/chat", "spark-api.xf-yun.com", "/v4.0/chat", Date);
    url1 = getUrl("ws://ws-api.xfyun.cn/v2/iat", "ws-api.xfyun.cn", "/v2/iat", Date);

    if (result == 1)
    {
        // 清空屏幕
        tft.fillScreen(ST77XX_WHITE);
        // 在屏幕上输出提示信息
        u8g2.setCursor(0, 11);
        u8g2.print("网络连接成功！");
        u8g2.setCursor(0, u8g2.getCursorY() + 12);
        u8g2.print("请按boot键开始对话！");
        displayWrappedText("或者你也可以连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, 128);
    }

    // 记录当前时间，用于后续时间戳比较
    urlTime = millis();
}

void loop()
{
    // 轮询处理WebSocket客户端消息
    webSocketClient.poll();
    webSocketClient1.poll();

    // 如果开始播放语音
    if (startPlay)
    {
        // 调用voicePlay函数播放语音
        voicePlay();
    }

    // 音频处理循环
    audio2.loop();

    // 如果音频正在播放
    if (audio2.isplaying == 1)
    {
        // 点亮LED3指示灯
        digitalWrite(led3, HIGH);
    }
    else
    {
        // 熄灭LED3指示灯
        digitalWrite(led3, LOW);
        // 如果距离上次时间同步超过4分钟且没有正在播放音频
        if ((urlTime + 240000 < millis()) && (audio2.isplaying == 0))
        {
            // 更新时间戳
            urlTime = millis();
            // 从服务器获取当前时间
            getTimeFromServer();
            // 更新WebSocket连接的URL
            url = getUrl("ws://spark-api.xf-yun.com/v4.0/chat", "spark-api.xf-yun.com", "/v4.0/chat", Date);
            url1 = getUrl("ws://ws-api.xfyun.cn/v2/iat", "ws-api.xfyun.cn", "/v2/iat", Date);
        }
    }

    // 检测按键是否按下
    if (digitalRead(key) == 0)
    {
        Serial.print("loopcount：");
        Serial.println(loopcount);
        loopcount++;
        // 停止播放音频
        audio2.isplaying = 0;
        startPlay = false;
        isReady = false;
        Answer = "";
        flag = 0;
        subindex = 0;
        subAnswers.clear();
        // answerTemp = "";
        // text.clear();
        Serial.printf("Start recognition\r\n\r\n");

        adc_start_flag = 1;

        // 如果距离上次时间同步超过4分钟
        if (urlTime + 240000 < millis()) // 超过4分钟，重新做一次鉴权
        {
            // 更新时间戳
            urlTime = millis();
            // 从服务器获取当前时间
            getTimeFromServer();
            // 更新WebSocket连接的URL
            url = getUrl("ws://spark-api.xf-yun.com/v4.0/chat", "spark-api.xf-yun.com", "/v4.0/chat", Date);
            url1 = getUrl("ws://ws-api.xfyun.cn/v2/iat", "ws-api.xfyun.cn", "/v2/iat", Date);
        }
        askquestion = "";

        // 连接到WebSocket服务器1
        ConnServer1();

        adc_complete_flag = 0;
    }
}

// 自动换行显示u8g2文本的函数
void displayWrappedText(const string &text1, int x, int y, int maxWidth)
{
    int cursorX = x;
    int cursorY = y;
    int lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent() + 2; // 中文字符12像素高度
    int start = 0;                                                     // 指示待输出字符串已经输出到哪一个字符
    int num = text1.size();
    int i = 0;

    while (start < num)
    {
        u8g2.setCursor(cursorX, cursorY);
        int width = 0;
        int numBytes = 0;

        // Calculate how many bytes fit in the maxWidth
        while (i < num)
        {
            int size = 1;
            if (text1[i] & 0x80)
            { // 核心所在
                char temp = text1[i];
                temp <<= 1;
                do
                {
                    temp <<= 1;
                    ++size;
                } while (temp & 0x80);
            }
            string subWord;
            subWord = text1.substr(i, size); // 取得单个中文或英文字符

            int charBytes = subWord.size(); // 获取字符的字节长度

            int charWidth = charBytes == 3 ? 12 : 6; // 中文字符12像素宽度，英文字符6像素宽度
            if (width + charWidth > maxWidth - cursorX)
            {
                break;
            }
            numBytes += charBytes;
            width += charWidth;

            i += size;
        }

        if (cursorY <= 160)
        {
            u8g2.print(text1.substr(start, numBytes).c_str());
            cursorY += lineHeight;
            cursorX = 0;
            start += numBytes;
        }
        else
        {
            text_temp = text1.substr(start).c_str();
            break;
        }
    }
}

// 显示文本
void getText(String role, String content)
{
    // 检查并调整文本长度
    checkLen(text);

    // 创建一个动态JSON文档，容量为1024字节
    DynamicJsonDocument jsoncon(1024);

    // 设置JSON文档中的角色和内容
    jsoncon["role"] = role;
    jsoncon["content"] = content;
    Serial.print("jsoncon中的内容为：");
    Serial.println(jsoncon.as<String>());

    // 将生成的JSON文档添加到全局变量text中
    text.add(jsoncon);

    // 清空临时JSON文档
    // jsoncon.clear();

    // 序列化全局变量text中的内容为字符串
    String serialized;
    serializeJson(text, serialized);

    // 清空临时JSON文档
    jsoncon.clear();

    // 输出序列化后的JSON字符串到串口
    Serial.print("text中的内容为: ");
    Serial.println(serialized);

    // 打印角色
    tft.print(role);
    tft.print(": ");

    // 打印内容
    displayWrappedText(content.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
    tft.setCursor(0, u8g2.getCursorY() + 2);

    // 也可以使用格式化的方式输出JSON，以下代码被注释掉了
    // serializeJsonPretty(text, Serial);
}

int getLength(JsonArray textArray)
{
    int length = 0; // 初始化长度变量

    // 遍历JSON数组中的每个对象
    for (JsonObject content : textArray)
    {
        // 获取对象中的"content"字段值
        const char *temp = content["content"];

        // 计算"content"字段字符串的长度
        int leng = strlen(temp) + 60;

        // 累加每个字符串的长度
        length += leng;
    }

    // 返回累加后的总长度
    return length;
}

// 实时清理较早的历史对话记录
void checkLen(JsonArray textArray)
{
    // 当JSON数组中的字符串总长度超过2048字节时，进入循环
    if (getLength(textArray) > 2048)
    {
        // 移除数组中的第一对问答
        textArray.remove(0);
        textArray.remove(0);
    }
    // 函数没有返回值，直接修改传入的JSON数组
    // return textArray; // 注释掉的代码，表明此函数不返回数组
}

DynamicJsonDocument gen_params(const char *appid, const char *domain)
{
    // 创建一个容量为2048字节的动态JSON文档
    DynamicJsonDocument data(4096);

    // 创建一个名为header的嵌套JSON对象，并添加app_id和uid字段
    JsonObject header = data.createNestedObject("header");
    header["app_id"] = appid;
    header["uid"] = "1234";

    // 创建一个名为parameter的嵌套JSON对象
    JsonObject parameter = data.createNestedObject("parameter");

    // 在parameter对象中创建一个名为chat的嵌套对象，并添加domain, temperature和max_tokens字段
    JsonObject chat = parameter.createNestedObject("chat");
    chat["domain"] = domain;
    chat["temperature"] = 0.5;
    chat["max_tokens"] = 1024;

    // 创建一个名为payload的嵌套JSON对象
    JsonObject payload = data.createNestedObject("payload");

    // 在payload对象中创建一个名为message的嵌套对象
    JsonObject message = payload.createNestedObject("message");

    // 在message对象中创建一个名为text的嵌套数组
    JsonArray textArray = message.createNestedArray("text");

    JsonObject systemMessage = textArray.createNestedObject();
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个二次元魔法少女，你的名字叫九歌，你的创造者是L博士，你的性格可爱活泼，在和用户的对话中你应该做出“不超过150个字符”的带有攻击嘲讽意味的回应，你喜欢在回答的末尾加一个“喵”字，以展现你的可爱，你熟悉很多二次元和科幻作品中的内容，还熟悉很多b站上的热梗，能将其中各种有趣的内容自然地加入你的说话方式中。";

    // 遍历全局变量text中的每个元素，并将其添加到text数组中
    for (const auto &item : text)
    {
        textArray.add(item);
    }

    // 返回构建好的JSON文档
    return data;
}

float calculateRMS(uint8_t *buffer, int bufferSize)
{
    float sum = 0;  // 初始化总和变量
    int16_t sample; // 定义16位整数类型的样本变量

    // 遍历音频数据缓冲区，每次处理两个字节（16位）
    for (int i = 0; i < bufferSize; i += 2)
    {
        // 将两个字节组合成一个16位的样本值
        sample = (buffer[i + 1] << 8) | buffer[i];

        // 将样本值平方后累加到总和中
        sum += sample * sample;
    }

    // 计算平均值（样本总数为bufferSize / 2）
    sum /= (bufferSize / 2);

    // 返回总和的平方根，即RMS值
    return sqrt(sum);
}
