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

// 定义引脚
#define key 0       //boot按键引脚
#define led 2       //板载led引脚
#define light 32    //台灯控制引脚
#define awaken 33   //语音唤醒信号引脚
// 屏幕引脚定义
#define TFT_CS 5
#define TFT_RST 12
#define TFT_DC 16
#define TFT_SCLK 18
#define TFT_MOSI 23
// 定义音频放大模块的I2S引脚定义
#define I2S_DOUT 27 // DIN引脚
#define I2S_BCLK 26 // BCLK引脚
#define I2S_LRC 25  // LRC引脚

// AP模式的SSID和密码
const char *ap_ssid = "ESP32-Setup";
const char *ap_password = "12345678";
// Web服务器和Preferences对象
AsyncWebServer server(80);
Preferences preferences;

// 星火大模型的账号参数
String APPID = "";                             // 星火大模型的App ID,必填
String APISecret = ""; // API Secret，必填
String APIKey = "";    // API Key，必填

// 星火大模型参数
String appId1 = APPID;
String domain1 = "4.0Ultra";    // 根据需要更改
String websockets_server = "ws://spark-api.xf-yun.com/v4.0/chat";   // 根据需要更改
String websockets_server1 = "ws://iat-api.xfyun.cn/v2/iat";

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

/*/ 创建动态JSON文档对象和数组
StaticJsonDocument<2048> doc;
JsonArray text = doc.to<JsonArray>();*/
// 使用动态JSON文档存储历史对话信息占用的内存过多，故改用c++中的vector向量
std::vector<String> text;

// 定义字符串变量，用于存储鉴权参数
String url = "";
String url1 = "";
String Date = "";

String askquestion = "";        //存储stt语音转文字信息，即用户的提问信息
String Answer = "";             //存储llm回答，用于语音合成（较短的回答）
std::vector<String> subAnswers; //存储llm回答，用于语音合成（较长的回答，分段存储）
int subindex = 0;               //subAnswers的下标，用于voicePlay()
String text_temp = "";          //存储超出当前屏幕的文字，在下一屏幕显示
int loopcount = 0;      //对话次数计数器
int flag = 0;           //用来确保subAnswer1一定是大模型回答最开始的内容
int conflag = 0;        //用于连续对话
int await_flag = 1;     //待机标识
int start_con = 0;      //标识是否开启了一轮对话
int AP_status = 0;      //标识热点是否开启

using namespace websockets; // 使用WebSocket命名空间
// 创建WebSocket客户端对象
WebsocketsClient webSocketClient;   //与llm通信
WebsocketsClient webSocketClient1;  //与stt通信

HTTPClient https; // 创建一个HTTP客户端对象
hw_timer_t *timer = NULL; // 定义硬件定时器对象

// 创建屏幕对象
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// 创建音频对象
Audio1 audio1;
Audio2 audio2(false, 3, I2S_NUM_1); 
// 参数: 是否使用内部DAC（数模转换器）如果设置为true，将使用ESP32的内部DAC进行音频输出。否则，将使用外部I2S设备。
// 指定启用的音频通道。可以设置为1（只启用左声道）或2（只启用右声道）或3（启用左右声道）
// 指定使用哪个I2S端口。ESP32有两个I2S端口，I2S_NUM_0和I2S_NUM_1。可以根据需要选择不同的I2S端口。

// 函数声明
void handleRoot(AsyncWebServerRequest *request);
void handleWifiManagement(AsyncWebServerRequest *request);
void handleMusicManagement(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);
void handleSaveMusic(AsyncWebServerRequest *request);
void handleDeleteMusic(AsyncWebServerRequest *request);
void handleListMusic(AsyncWebServerRequest *request);

DynamicJsonDocument gen_params(const char *appid, const char *domain);
void displayWrappedText(const string &text1, int x, int y, int maxWidth);
void getText(String role, String content);
void checkLen();
float calculateRMS(uint8_t *buffer, int bufferSize);
void ConnServer();
void ConnServer1();

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
    // 创建一个静态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节,硬件限制，无法再增加
    StaticJsonDocument<1024> jsonDocument;

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

                        // 将提取的句子转换为语音
                        audio2.connecttospeech(subAnswer1.c_str(), "zh");

                        // 获取最终转换的文本
                        getText("assistant", subAnswer1);
                        flag = 1;

                        // 更新Answer，去掉已处理的部分
                        Answer = Answer.substring(firstPeriodIndex + 3);
                        subAnswer1.clear();
                        // 设置播放开始标志
                        startPlay = true;
                    }
                }
                else
                {
                    String subAnswer1 = Answer.substring(0, Answer.length());
                    Serial.print("subAnswer1:");
                    Serial.println(subAnswer1);
                    subAnswer1.clear();

                    audio2.connecttospeech(Answer.c_str(), "zh");
                    // 获取最终转换的文本
                    getText("assistant", Answer);
                    flag = 1;

                    Answer = Answer.substring(Answer.length());
                    // 设置播放开始标志
                    startPlay = true;
                }
                conflag = 1;
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
                // 播放最终转换的文本
                audio2.connecttospeech(Answer.c_str(), "zh");
                // 显示最终转换的文本
                getText("assistant", Answer);
                Answer = "";
                conflag = 1;
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
        DynamicJsonDocument jsonData = gen_params(appId1.c_str(), domain1.c_str());

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

// 接收stt返回的语音识别文本并做相应的逻辑处理
void onMessageCallback1(WebsocketsMessage message)
{
    // 创建一个静态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节
    StaticJsonDocument<2000> jsonDocument;

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
    // 如果解析没有错误，从JSON数据中获取返回码
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

        if (jsonDocument["data"]["status"] != 2)
        {
            askquestion = "";
        }

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

            if (await_flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                u8g2.setCursor(0, 11);
                u8g2.print("正在识别唤醒词。。。");
                // 增加足够多的同音字可以提高唤醒率，支持多唤醒词唤醒
                if((askquestion.indexOf("你好") > -1 || askquestion.indexOf("您好") > -1) && (askquestion.indexOf("坤坤") > -1 || askquestion.indexOf("小白") > -1 || askquestion.indexOf("丁真") > -1))
                {
                    await_flag = 0;
                    start_con = 1;      //对话开始标识
                    // 清空屏幕
                    tft.fillScreen(ST77XX_WHITE);
                    tft.setCursor(0, 0);
                    // 打印角色
                    tft.print("assistant");
                    tft.print(": ");

                    askquestion = "喵~你好主人，有什么我可以帮你的吗？";
                    audio2.connecttospeech(askquestion.c_str(), "zh");
                    // 打印内容
                    displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                    askquestion = "";
                    conflag = 1;
                    return;
                }
                else
                {
                    u8g2.setCursor(0, 11+12);
                    u8g2.print("识别错误，请再次唤醒！");
                    // 给awaken引脚提供低电平信号,继续进行唤醒词识别
                    digitalWrite(awaken, LOW);
                    return;
                }
            }

            // 如果问句为空，播放错误提示语音
            if (askquestion == "")
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                // 打印角色
                tft.print("assistant");
                tft.print(": ");

                askquestion = "喵~对不起，我没有听清，可以再说一遍吗？";
                audio2.connecttospeech(askquestion.c_str(), "zh");
                // 打印内容
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                askquestion = "";
                conflag = 1;
            }
            else if (askquestion.indexOf("退下") > -1 || askquestion.indexOf("再见") > -1 || askquestion.indexOf("拜拜") > -1)
            {
                start_con = 0;      // 标识一轮对话结束
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                tft.print("user");
                tft.print(": ");
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                tft.setCursor(0, u8g2.getCursorY() + 2);
                tft.print("assistant");
                tft.print(": ");

                askquestion = "喵~主人，我先退下了，有事再找我。"; 
                audio2.connecttospeech(askquestion.c_str(), "zh");
                // 打印内容
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                askquestion = "";
                await_flag = 1;
                digitalWrite(awaken, LOW);
            }
            else if (askquestion.indexOf("断开") > -1 && (askquestion.indexOf("网络") > -1 || askquestion.indexOf("连接") > -1))
            {
                // 断开当前WiFi连接
                WiFi.disconnect(true);
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                // 打印内容
                displayWrappedText("网络连接已断开，请重启设备以再次建立连接！", tft.getCursorX(), tft.getCursorY() + 11, 128);
                askquestion = "";
            }
            else if (((askquestion.indexOf("听") > -1 || askquestion.indexOf("放") > -1) && (askquestion.indexOf("歌") > -1 || askquestion.indexOf("音乐") > -1)) || mainStatus == 1)
            {
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                tft.print("user");
                tft.print(": ");
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                tft.setCursor(0, u8g2.getCursorY() + 2);

                String musicName = "";
                String musicID = "";

                preferences.begin("music_store", true);

                // 查找音乐名称对应的ID
                int numMusic = preferences.getInt("numMusic", 0);
                for (int i = 0; i < numMusic; ++i)
                {
                    musicName = preferences.getString(("musicName" + String(i)).c_str(), "");
                    musicID = preferences.getString(("musicId" + String(i)).c_str(), "");
                    Serial.println("音乐名称: " + musicName);
                    Serial.println("音乐ID: " + musicID);
                    if (askquestion.indexOf(musicName.c_str()) > -1)
                    {
                        Serial.println("找到了！");
                        break;
                    }
                    else
                    {
                        musicID = "";
                    }
                }
                if (askquestion.indexOf("最喜欢的") > -1)
                {
                    musicName = "Lilas";
                    musicID = "1993906158";
                }

                // 输出结果
                if (musicID == "") {
                    mainStatus = 1;
                    Serial.println("未找到对应的音乐");
                    // 打印角色
                    tft.print("assistant");
                    tft.print(": ");

                    askquestion = "好的，主人，你想听哪首歌呢~";
                    audio2.connecttospeech(askquestion.c_str(), "zh");
                    // 打印内容
                    displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                    askquestion = "";
                    conflag = 1;
                } else {
                    // 自建音乐服务器，按照文件名查找对应歌曲
                    mainStatus = 0;
                    String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
                    Serial.println(audioStreamURL.c_str());
                    audio2.connecttohost(audioStreamURL.c_str());
                    
                    if (musicID == "2155422573")  askquestion = "正在播放音乐：使一颗心免于哀伤";
                    else    askquestion = "正在播放音乐：" + musicName;
                    Serial.println(askquestion);
                    // 打印内容
                    displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                    askquestion = "";
                    // 设置播放开始标志
                    startPlay = true;
                    flag = 1;
                    Answer = "音乐播放完了，主人还想听什么音乐吗？";
                    conflag = 1;
                }
                preferences.end();
            }
            else if (askquestion.indexOf("开") > -1 && askquestion.indexOf("台灯") > -1)
            {
                // 给light引脚提供高电平信号
                digitalWrite(light, HIGH);
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                tft.print("user");
                tft.print(": ");
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                tft.setCursor(0, u8g2.getCursorY() + 2);
                // 打印角色
                tft.print("assistant");
                tft.print(": ");

                askquestion = "已为你打开台灯。";
                audio2.connecttospeech(askquestion.c_str(), "zh");
                // 打印内容
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                askquestion = "";
                conflag = 1;
            }
            else if (askquestion.indexOf("关") > -1 && askquestion.indexOf("台灯") > -1)
            {
                // 给light引脚提供低电平信号
                digitalWrite(light, LOW);
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                tft.print("user");
                tft.print(": ");
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                tft.setCursor(0, u8g2.getCursorY() + 2);
                // 打印角色
                tft.print("assistant");
                tft.print(": ");

                askquestion = "已为你关闭台灯。";
                audio2.connecttospeech(askquestion.c_str(), "zh");
                // 打印内容
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                askquestion = "";
                conflag = 1;
            }
            else
            {
                // 清空屏幕
                tft.fillScreen(ST77XX_WHITE);
                tft.setCursor(0, 0);
                // 处理一般的问答请求
                getText("user", askquestion);
                askquestion = "";
                //Serial.print("text:");
                //Serial.println(text);
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

        // 初始化变量
        int silence = 0;
        int firstframe = 1;
        int j = 0;
        int voicebegin = 0;
        int voice = 0;
        int null_voice = 0;

        // 创建一个JSON文档对象
        StaticJsonDocument<2000> doc;

        if (await_flag == 1)
        {
            // 清空屏幕
            tft.fillScreen(ST77XX_WHITE);
            u8g2.setCursor(0, 11);
            u8g2.print("待机中。。。");
        }
        else
        {
            // 清空屏幕
            tft.fillScreen(ST77XX_WHITE);
            u8g2.setCursor(0, 11);
            u8g2.print("请说话。");
        }

        // 无限循环，用于录制和发送音频数据
        while (1)
        {
            // 待机状态（语音唤醒状态）也可通过boot键启动
            if (digitalRead(key) == 0 && await_flag == 1)
            {
                start_con = 1;      //对话开始标识
                await_flag = 0;
                webSocketClient1.close();
                return;
            }
            // 清空JSON文档
            doc.clear();

            // 创建data对象
            JsonObject data = doc.createNestedObject("data");

            // 录制音频数据
            audio1.Record();

            // 计算音频数据的RMS值
            float rms = calculateRMS((uint8_t *)audio1.wavData[0], 1280);
            printf("%d %f\n", 0, rms);

            if(null_voice >= 80)
            {
                if (start_con == 1)     // 表示正处于对话中，才回复退下，没有进入对话则继续待机
                {
                    start_con = 0;
                    // 清空屏幕
                    tft.fillScreen(ST77XX_WHITE);
                    // 显示图片
                    // tft.drawRGBBitmap(0, 0, liuying1_0, 128, 160);
                    tft.setCursor(0, 0);
                    // 打印角色
                    tft.print("assistant");
                    tft.print(": ");

                    askquestion = "主人，我先退下了，有事再找我。";
                    audio2.connecttospeech(askquestion.c_str(), "zh");
                    // 打印内容
                    displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
                    askquestion = "";
                }

                await_flag = 1;
                digitalWrite(awaken, LOW);

                webSocketClient1.close();
                return;
            }

            // 判断是否为噪音
            if (rms < noise)
            {
                null_voice++;
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
                common["app_id"] = appId1.c_str();

                JsonObject business = doc.createNestedObject("business");
                business["domain"] = "iat";
                business["language"] = "zh_cn";
                business["accent"] = "mandarin";
                // 不使用动态修正
                // business["vinfo"] = 1;
                // 使用动态修正
                business["dwa"] = "wpgs";
                business["vad_eos"] = 2000;

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
            audio2.connecttospeech(subAnswers[subindex].c_str(), "zh");
            // 在屏幕上显示文字
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
            subindex++;
        }
        else
        {
            audio2.connecttospeech(Answer.c_str(), "zh");
            // 在屏幕上显示文字
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
            Answer = "";
            conflag = 1;
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

    preferences.begin("wifi_store");
    int numNetworks = preferences.getInt("numNetworks", 0);
    if (numNetworks == 0)
    {
        // 在屏幕上输出提示信息
        u8g2.setCursor(0, u8g2.getCursorY() + 12);
        u8g2.print("无任何wifi存储信息！");
        displayWrappedText("请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, 128);
        preferences.end();
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
                // 闪烁板载LED以指示连接状态
                digitalWrite(led, ledstatus);
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
                preferences.end();
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
    preferences.end();
    return 0;
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

void StartConversation()
{
    conflag = 0;
    // 停止播放音频
    audio2.isplaying = 0;
    startPlay = false;
    isReady = false;
    Answer = "";
    flag = 0;
    subindex = 0;
    subAnswers.clear();
    Serial.printf("Start recognition\r\n\r\n");
    // 如果距离上次时间同步超过4分钟
    if (urlTime + 240000 < millis()) // 超过4分钟，重新做一次鉴权
    {
        // 更新时间戳
        urlTime = millis();
        // 从服务器获取当前时间
        getTimeFromServer();
        // 更新WebSocket连接的URL
        url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
        url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
    }
    // 连接到WebSocket服务器1
    ConnServer1();
}

void setup()
{
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 配置引脚模式
    // 配置按键引脚为上拉输入模式，用于boot按键检测
    pinMode(key, INPUT_PULLUP);

    // 设置awaken引脚为输出模式
    pinMode(awaken, OUTPUT);
    // 将led设置为输出模式
    pinMode(led, OUTPUT);
    // 将light设置为输出模式
    pinMode(light, OUTPUT);

    // 初始化屏幕
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_WHITE);   // 设置屏幕背景为白色
    tft.setTextColor(ST77XX_BLACK); //设置字体颜色为黑色
    tft.setTextWrap(true);  // 开启文本自动换行，只支持英文

    // 初始化U8g2
    u8g2.begin(tft);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体库
    u8g2.setFontMode(1);                    // 设置字体模式为1，以支持中文字符
    u8g2.setForegroundColor(ST77XX_BLACK);  // 设置字体颜色为黑色
    // 显示文字
    u8g2.setCursor(0, 11);
    u8g2.print("已开机！");

    // 初始化音频模块audio1
    audio1.init();
    // 设置音频输出引脚和音量
    audio2.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio2.setVolume(40);

    // 初始化Preferences
    preferences.begin("wifi_store");
    preferences.begin("music_store");
    // 连接网络
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("正在连接网络······");
    int result = wifiConnect();

    // 从百度服务器获取当前时间
    getTimeFromServer();
    // 使用当前时间生成WebSocket连接的URL
    url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
    url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);

    if (result == 1)
    {
        // 清空屏幕，在屏幕上输出提示信息
        tft.fillScreen(ST77XX_WHITE);
        u8g2.setCursor(0, 11);
        u8g2.print("网络连接成功！");
        displayWrappedText("请进行语音唤醒或按boot键开始对话！", 0, u8g2.getCursorY() + 12, 128);
        digitalWrite(awaken, LOW);
    }
    else
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
    // 记录当前时间，用于后续时间戳比较
    urlTime = millis();
    // 延迟2000毫秒，便于用户查看屏幕显示的信息，同时使设备充分初始化
    delay(2000);
}

void loop()
{
    // 轮询处理WebSocket客户端消息
    webSocketClient.poll();
    webSocketClient1.poll();

    // 如果有多段语音需要播放
    if (startPlay)  voicePlay();    // 调用voicePlay函数播放后续的语音

    // 音频处理循环
    audio2.loop();

    // 如果音频正在播放
    if (audio2.isplaying == 1)  digitalWrite(led, HIGH);    // 点亮板载LED指示灯
    else    digitalWrite(led, LOW);     // 熄灭板载LED指示灯
    
    // 唤醒词识别
    if (audio2.isplaying == 0 && digitalRead(awaken) == 0 && await_flag == 1)
    {
        digitalWrite(awaken, HIGH);
        StartConversation();
    }

    // 检测boot按键是否按下
    if (digitalRead(key) == 0)
    {
        webSocketClient.close();    //关闭llm服务器，打断上一次提问的回答生成
        loopcount++;
        Serial.println("loopcount：" + loopcount);
        StartConversation();
    }
    
    // 连续对话
    if (audio2.isplaying == 0 && Answer == "" && subindex == subAnswers.size() && conflag == 1)
    {
        loopcount++;
        Serial.println("loopcount：" + loopcount);
        StartConversation();
    }

    // 主动断开连接后，打开热点，可进行网络和音乐配置
    if (WiFi.status() != WL_CONNECTED && AP_status == 0)
    {
        AP_status = 1;
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
    checkLen();

    // 创建一个静态JSON文档，容量为512字节
    StaticJsonDocument<512> jsoncon;

    // 设置JSON文档中的角色和内容
    jsoncon["role"] = role;
    jsoncon["content"] = content;
    Serial.print("jsoncon：");
    Serial.println(jsoncon.as<String>());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsoncon, jsonString);

    // 将字符串存储到vector中
    text.push_back(jsonString);

    // 输出vector中的内容
    for (const auto& jsonStr : text) {
        Serial.println(jsonStr);
    }
    
    /*/ 将生成的JSON文档添加到全局变量text中
    text.add(jsoncon);

    // 序列化全局变量text中的内容为字符串
    String serialized;
    serializeJson(text, serialized);

    // 输出序列化后的JSON字符串到串口
    Serial.print("text: ");
    Serial.println(serialized);*/

    // 清空临时JSON文档
    jsoncon.clear();

    // 打印角色
    tft.print(role);
    tft.print(": ");

    // 打印内容
    displayWrappedText(content.c_str(), tft.getCursorX(), tft.getCursorY() + 11, 128);
    tft.setCursor(0, u8g2.getCursorY() + 2);

    // 也可以使用格式化的方式输出JSON，以下代码被注释掉了
    // serializeJsonPretty(text, Serial);
}

// 实时清理较早的历史对话记录
void checkLen()
{
    /*Serial.print("text size:");
    Serial.println(text.memoryUsage());
    // 计算jsonVector占用的字节数
    // 当JSON数组中的字符串总长度超过1600字节时，进入循环
    if (text.memoryUsage() > 1600)
    {
        // 移除数组中的第一对问答
        text.remove(0);
        text.remove(0);
    }*/
    size_t totalBytes = 0;

    // 计算vector中每个字符串的长度
    for (const auto& jsonStr : text) {
        totalBytes += jsonStr.length();
    }
    Serial.print("text size:");
    Serial.println(totalBytes);
    if (totalBytes > 800)
    {
        text.erase(text.begin(), text.begin() + 2);
    }
    // 函数没有返回值，直接修改传入的JSON数组
    // return textArray; // 注释掉的代码，表明此函数不返回数组
}

DynamicJsonDocument gen_params(const char *appid, const char *domain)
{
    // 创建一个容量为3000字节的动态JSON文档
    DynamicJsonDocument data(1500);

    // 创建一个名为header的嵌套JSON对象，并添加app_id和uid字段
    JsonObject header = data.createNestedObject("header");
    header["app_id"] = appid;
    header["uid"] = "1234";

    // 创建一个名为parameter的嵌套JSON对象
    JsonObject parameter = data.createNestedObject("parameter");

    // 在parameter对象中创建一个名为chat的嵌套对象，并添加domain, temperature和max_tokens字段
    JsonObject chat = parameter.createNestedObject("chat");
    chat["domain"] = domain;
    chat["temperature"] = 0.6;
    chat["max_tokens"] = 1024;

    // 创建一个名为payload的嵌套JSON对象
    JsonObject payload = data.createNestedObject("payload");

    // 在payload对象中创建一个名为message的嵌套对象
    JsonObject message = payload.createNestedObject("message");

    // 在message对象中创建一个名为text的嵌套数组
    JsonArray textArray = message.createNestedArray("text");

    JsonObject systemMessage = textArray.createNestedObject();
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个可爱活泼的陪伴型AI，你的名字是小白，你的情感丰富，性格粘人，说话简洁明了。";

    // 遍历全局变量text中的每个元素，并将其添加到text数组中
    /*for (const auto &item : text)
    {
        textArray.add(item);
    }*/
    // 将jsonVector中的内容添加到JsonArray中
    for (const auto& jsonStr : text) {
        DynamicJsonDocument tempDoc(512);
        DeserializationError error = deserializeJson(tempDoc, jsonStr);
        if (!error) {
            textArray.add(tempDoc.as<JsonVariant>());
        } else {
            Serial.print("反序列化失败: ");
            Serial.println(error.c_str());
        }
    }
    // 返回构建好的JSON文档
    return data;
}

// 计算录音数据的均方根值
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
    tft.fillScreen(ST77XX_WHITE);
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
    tft.fillScreen(ST77XX_WHITE);
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
    tft.fillScreen(ST77XX_WHITE);
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
    tft.fillScreen(ST77XX_WHITE);
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