#include "Web_Scr_set.h"

//  优先事项！！！一定要做，不做的话麦克风会因为引脚冲突无法工作
//  在vscode中打开文件夹，等待依赖库下载完成后,
//  找到.pio\libdeps\esp32-s3-devkitm-1\TFT_eSPI路径下的User_Setup.h文件，删除它，然后将根目录下的User_Setup.h文件剪切粘贴过去
//#define ESP32S3 1   // 如果你使用的是esp32s3开发板，取消注释此行
//#define ASRPRO 1    // 如果你需要使用ASRPRO开发板，再取消注释此行

#define key 0       //boot按键引脚

#ifdef ESP32S3    // 使用ESP32-S3开发板

// 定义音频放大模块的I2S引脚定义
#define I2S_DOUT 5 // DIN引脚
#define I2S_BCLK 6 // BCLK引脚
#define I2S_LRC 7  // LRC引脚
// 定义麦克风引脚
#define PIN_I2S_BCLK 2  // 时钟线，对应INMP441的SCK
#define PIN_I2S_LRC  1  // 声道选择线，对应INMP441的WS，由主机发送给从机，WS低电平时，从机发送左声道数据，高电平时发送右声道数据
#define PIN_I2S_DIN 42  // 数据线，对应INMP441的SD

#ifdef ASRPRO
    #define awake 3     // 唤醒引脚
    #define RX2 19
    #define TX2 20
    // UART 接收缓冲区
    const int BUFFER_SIZE = 5; // 每个命令包的大小
    char buffer[BUFFER_SIZE];  // 接收缓冲区
    int globalIndex = 0;             // 当前接收到的数据在缓冲区中的位置
#else
#endif

#define led 8       //板载led引脚
#define light 38     // 灯光引脚
#define max_context_length 3200

#define NUM_LEDS 1
#define DATA_PIN 48 // 灯光引脚
CRGB leds[NUM_LEDS];

#else           // 使用ESP32开发板

// 定义音频放大模块的I2S引脚定义
#define I2S_DOUT 25 // DIN引脚
#define I2S_BCLK 26 // BCLK引脚
#define I2S_LRC 27  // LRC引脚
// 定义麦克风引脚
#define PIN_I2S_BCLK 4
#define PIN_I2S_LRC 15
#define PIN_I2S_DIN 22  //换成21,19都不行

#ifdef ASRPRO
    #define awake 19     // 唤醒引脚
    #define RX2 16
    #define TX2 17
    // UART 接收缓冲区
    const int BUFFER_SIZE = 5; // 每个命令包的大小
    char buffer[BUFFER_SIZE];  // 接收缓冲区
    int globalIndex = 0;             // 当前接收到的数据在缓冲区中的位置
#else
#endif

#define led 2       //板载led引脚
#define light 33     // 灯光引脚
#define max_context_length 1600
#endif

// 大模型参数填写，顺序：ChatGPT、Claude、Gemini、Grok、Mistral、豆包大模型、月之暗面、通义千问、讯飞星火、腾讯混元、百川智能、BigModel、零一万物、DeepSeek、Ollama
int llm_num = 15;
int llm = 8;

String llm_name[] = {"ChatGPT", "Claude",
                        "Gemini", "Grok",
                        "Mistral", "豆包大模型",
                        "月之暗面", "通义千问",
                        "讯飞星火", "腾讯混元", 
                        "百川智能", "BigModel", 
                        "零一万物", "DeepSeek",
                        "Ollama"
                        };

String llm_model[] = {"gpt-4o-mini", "claude-3-5-haiku-20241022", 
                        "gemini-1.5-flash-002", "grok-beta", 
                        "mistral-large-latest", "ep-20241109134647-q669z", 
                        "moonshot-v1-32k", "qwen-turbo-1101", 
                        "4.0Ultra", "hunyuan-pro",
                        "Baichuan4", "glm-4-plus",
                        "yi-large-fc", "deepseek-chat",
                        "qwen2.5:7b"
                        };

String llm_apiKey[] = {"", "", 
                        "", "", 
                        "", "", 
                        "", "", 
                        "", "",
                        "", "",
                        "", "",
                        ""
                        };

String llm_url[] = {"https://aihubmix.com/v1/chat/completions", "https://aihubmix.com/v1/chat/completions", 
                    "https://aihubmix.com/v1/chat/completions", "https://aihubmix.com/v1/chat/completions", 
                    "https://api.mistral.ai/v1/chat/completions", "https://ark.cn-beijing.volces.com/api/v3/chat/completions", 
                    "https://api.moonshot.cn/v1/chat/completions", "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions", 
                    "https://spark-api-open.xf-yun.com/v1/chat/completions", "https://api.hunyuan.cloud.tencent.com/v1/chat/completions",
                    "https://api.baichuan-ai.com/v1/chat/completions", "https://open.bigmodel.cn/api/paas/v4/chat/completions",
                    "https://api.lingyiwanwu.com/v1/chat/completions", "https://api.deepseek.com/chat/completions",
                    "http://127.0.0.1:11434/api/chat"
                    };

// 讯飞websocket调用stt和大模型服务的参数
String APPID = "";                             // App ID,必填
String APISecret = ""; // API Secret，必填
String APIKey = "";    // API Key，必填
String appId1 = APPID;
String domain1 = "4.0Ultra";    // 根据需要更改
String websockets_server = "ws://spark-api.xf-yun.com/v4.0/chat";   // 根据需要更改
String websockets_server1 = "ws://iat-api.xfyun.cn/v2/iat";
// 讯飞stt语种设置
String language = "zh_cn";     //zh_cn：中文（支持简单的英文识别）en_us：English

// Dufs的监听端口
String dufs = "";   // 类似“http://192.168.5.6:5000/，需要关闭电脑公用网络防火墙”

// 角色设定
int role_num = 5;
int role = 0;
String roleSettings[] = {"你是一个二次元魔法猫娘，你的名字叫小灵，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。", 
                            "你是一个二次元魔法猫娘，你的名字叫小白，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。", 
                            "你是一个二次元魔法猫娘，你的名字叫小花，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。", 
                            "你是一个二次元魔法猫娘，你的名字叫小美，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。",
                            "你是一个二次元魔法猫娘，你的名字叫小芳，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。" 
                            };
/*----------------------只需修改上面的内容，下面的一般不需要----------------------------*/

// 定义一些全局变量
bool ledstatus = true;          // 控制led闪烁
bool startPlay = false;
unsigned long urlTime = 0;
int noise = 50;                 // 噪声门限值
int volume = 50;               // 初始音量大小（最小0，最大100）
//音乐播放
int mainStatus = 0;
int conStatus = 0;
int musicnum = 0;   //音乐位置下标
int musicplay = 0;  // 是否进入连续播放音乐状态
int cursorY = 0;

// 存储历史对话信息
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
int loopcount = 0;      // 对话次数计数器
int flag = 0;           // 用来确保subAnswer1一定是大模型回答最开始的内容
int conflag = 0;        // 用于连续对话
int await_flag = 1;     //待机标识
int start_con = 0;      //标识是否开启了一轮对话
int awake_flag = 0;     // 标识是否处于唤醒状态
int i = 0;              // 用于显示表情
unsigned long startTime = 0;
unsigned long endTime = 0;
int shuaxin = 0;
int chouxiang = 0;
int recording = 0;      // 确保在录音、语音转文字、处理文字、大模型给出回复之前的整个过程中不会触发ASRPRO指令，否则可能会导致对话被指令动作打断

int hint = 0;   // 提示音


// 创建屏幕对象
TFT_eSPI tft = TFT_eSPI();  // 创建TFT对象
U8g2_for_TFT_eSPI u8g2;
// 创建音频对象
Audio1 audio1(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DIN);
Audio2 audio2(false, 3, I2S_NUM_1); 
// 参数: 是否使用内部DAC（数模转换器）如果设置为true，将使用ESP32的内部DAC进行音频输出。否则，将使用外部I2S设备。
// 指定启用的音频通道。可以设置为1（只启用左声道）或2（只启用右声道）或3（启用左右声道）
// 指定使用哪个I2S端口。ESP32有两个I2S端口，I2S_NUM_0和I2S_NUM_1。可以根据需要选择不同的I2S端口。

// 函数声明
using namespace websockets; // 使用WebSocket命名空间
// 创建WebSocket客户端对象
WebsocketsClient webSocketClient;   //与llm通信
WebsocketsClient webSocketClient1;  //与stt通信
DynamicJsonDocument gen_params(const char *appid, const char *domain, const char *role_set);
void ConnServer();
void ConnServer1();
void getTimeFromServer();
String getUrl(String server, String host, String path, String date);
void processAskquestion();

DynamicJsonDocument createRequestBody(const char *model, const char *role_set);
void LLM_request(int llmNum);

void removeChars(const char *input, char *output, const char *removeSet);
void processResponse(int status);
void getText(String role, String content);
void checkLen();
void displayWrappedText(const string &text1, int x, int y, int maxWidth);
void loadModel();
void voicePlay();
int wifiConnect();

void voicePlay()
{
    if (audio2.isplaying == 0 && chouxiang != 0)
    {
        if (chouxiang == 1)
        {
            audio2.connecttohost((dufs + "dingzhen.mp3").c_str());
            
        }
        else if (chouxiang == 2)
        {
            audio2.connecttohost((dufs + "kunkun.mp3").c_str());
            
        }
        else
        {
            audio2.connecttohost((dufs + "laoda.mp3").c_str());
            
        }
        startPlay = false;
        return;
    }
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
                tft.fillScreen(TFT_WHITE);
                // 显示剩余的文字
                displayWrappedText(text_temp.c_str(), 0, 11, width);
                text_temp = "";
                displayWrappedText(subAnswers[subindex].c_str(), u8g2.getCursorX(), u8g2.getCursorY(), width);
            }
            else if (flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(TFT_WHITE);
                displayWrappedText(subAnswers[subindex].c_str(), 0, 11, width);
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
                tft.fillScreen(TFT_WHITE);
                // 显示剩余的文字
                displayWrappedText(text_temp.c_str(), 0, 11, width);
                text_temp = "";
                displayWrappedText(Answer.c_str(), u8g2.getCursorX(), u8g2.getCursorY(), width);
            }
            else if (flag == 1)
            {
                // 清空屏幕
                tft.fillScreen(TFT_WHITE);
                displayWrappedText(Answer.c_str(), 0, 11, width);
            }
            Answer = "";
        }
        // 设置开始播放标志
        startPlay = true;
    }
    else if (audio2.isplaying == 0 && subindex == subAnswers.size() && hint == 1)
    {
        audio2.connecttoFS(SPIFFS, "/hint1.mp3"); // SPIFFS
        hint = 0;
    }
    else if (audio2.isplaying == 0 && musicplay == 1)   // 处理连续播放音乐逻辑
    {
        preferences.begin("music_store", true);
        int numMusic = preferences.getInt("numMusic", 0);
        musicnum = musicnum + 1 < numMusic ? musicnum + 1 : 0;
        
        String musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
        String musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
        Serial.println("音乐名称: " + musicName);
        Serial.println("音乐ID: " + musicID);

        String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
        Serial.println(audioStreamURL.c_str());
        audio2.connecttohost(audioStreamURL.c_str());
        
        tft.fillRect(0, cursorY, width, 50, TFT_WHITE);
        askquestion = "正在顺序播放所有音乐，当前正在播放：" + musicName;
        Serial.println(askquestion);
        // 打印内容
        displayWrappedText(askquestion.c_str(), 0, cursorY + 11, width);
        askquestion = "";
        preferences.end();
        startPlay = true;
    }
    else
    {
        // 如果音频正在播放或回答内容为空，不做任何操作
    }
}

void response()
{
    tft.fillScreen(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.print("assistant: ");
    audio2.connecttospeech(Answer.c_str(), "zh");
    displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
    Answer = "";
}

void StartConversation()
{
    if (isWebServerStarted) {
        server.end();
        Serial.println("WebServer stopped");
        isWebServerStarted = false;
    }

    if (isSoftAPStarted) {
        WiFi.softAPdisconnect(true);
        Serial.println("Access Point disconnected");
        isSoftAPStarted = false;
    }
    askquestion = "";
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
    // 连接到WebSocket服务器1讯飞stt
    ConnServer1();
}

bool isValidCommand(const char cmdBuffer[]) 
{
  // 检查命令格式是否正确
  return (cmdBuffer[0] == 0xAA && cmdBuffer[1] == 0x55 &&
          cmdBuffer[3] == 0x55 && cmdBuffer[4] == 0xAA);
}

void processCommand(uint8_t command) 
{
  switch (command) {
    case 0x01:
        Serial.println("Received command 01.");
        // 执行命令 01 的操作
        audio2.connecttohost((dufs + "wocenima.mp3").c_str());
        startPlay = true;
        chouxiang = 1;
        tft.pushImage(0, 0, width, height, guichu[0]);   // 丁真
        awake_flag = 1;     // 命令词识别成功后，标识为唤醒状态
        break;
    case 0x02:
        Serial.println("Received command 02.");
        // 执行命令 02 的操作
        audio2.connecttohost((dufs + "Man.mp3").c_str());
        startPlay = true;
        chouxiang = 3;
        tft.pushImage(0, 0, width, height, guichu[2]);   // 牢大
        awake_flag = 1;     // 命令词识别成功后，标识为唤醒状态
        break;
    case 0x03:
        Serial.println("Received command 03.");
        // 执行命令 03 的操作
        audio2.connecttohost((dufs + "niganma.mp3").c_str());
        startPlay = true;
        chouxiang = 2;
        tft.pushImage(0, 0, width, height, guichu[1]);   // 坤坤
        awake_flag = 1;     // 命令词识别成功后，标识为唤醒状态
        break;
    case 0x04:
        Serial.println("Received command 04.");
        // 执行命令 04 的操作
        shuaxin = 0;
        conflag = 0;
        awake_flag = 1;     // 命令词识别成功后，标识为唤醒状态
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        StartConversation();
        break;
    default:
        Serial.println("Unknown command received.");
        break;
  }
}

void loadModel()
{
    preferences.begin("llm_store");
    int numLLM = preferences.getInt("numLLM", 0);

    for (int i = 0; i < numLLM; i++)
    {
        String llmName = preferences.getString(("llmName" + String(i)).c_str(), "");
        String llmApiKey = preferences.getString(("llmApiKey" + String(i)).c_str(), "");
        String llmApiUrl = preferences.getString(("llmApiUrl" + String(i)).c_str(), "");
        for (int j = 0; j < llm_num ; j++)
        {
            if (llmName == llm_name[j])
            {
                llm_apiKey[j] = llmApiKey;
                llm_url[j] = llmApiUrl;
                Serial.print(llmName);
                Serial.println("参数加载完成！");
            }
        }
    }
    Serial.println("LLM参数加载完毕");
    preferences.end();
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= TFT_HEIGHT ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

void draw_image(const char *image_url)
{
    tft.fillScreen(TFT_WHITE);

    // Time recorded for test purposes
    uint32_t t = millis();

    // Get the width and height in pixels of the jpeg if you wish
    uint16_t w = 0, h = 0;
    TJpgDec.getJpgSizeFromStream(&w, &h, image_url);
    Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
    if (w > h)
        tft.setRotation(3);        // 设置屏幕方向，0-3顺时针转
    // Draw the image, top left at 0,0
    TJpgDec.drawJpgFromStream(0 ,0, image_url);
    //TJpgDec.drawSdJpg(0, 0, "/panda.jpg");

    // How much time did rendering take
    t = millis() - t;
    Serial.print(t); Serial.println(" ms");
    delay(5000);
    tft.fillScreen(TFT_WHITE);
    tft.setRotation(0);        // 设置屏幕方向，0-3顺时针转
}

void setup()
{
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    #ifdef ASRPRO
    // 初始化 UART2
    Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
    pinMode(awake, INPUT);
    #endif
    // 配置引脚模式
    pinMode(key, INPUT_PULLUP);     // 配置按键引脚为上拉输入模式，用于boot按键检测

    // 将led设置为输出模式
    pinMode(led, OUTPUT);
    // 将light设置为输出模式
    pinMode(light, OUTPUT);
    // 将light初始化为低电平
    digitalWrite(light, LOW);

    Serial.println("引脚初始化完成！");

    #ifdef DATA_PIN
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // 初始化esp32s3板载圆形LED
    #endif

    // 初始化屏幕
    tft.init();
    tft.setRotation(0);        // 设置屏幕方向，0-3顺时针转
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_WHITE);   // 设置屏幕背景为白色
    tft.setTextColor(TFT_BLACK); //设置字体颜色为黑色
    tft.setTextWrap(true);  // 开启文本自动换行，只支持英文

    // 初始化U8g2
    u8g2.begin(tft);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体库
    u8g2.setFontMode(1);                    // 设置字体模式为透明模式，不设置的话中文字符会变成一个黑色方块
    u8g2.setForegroundColor(TFT_BLACK);     // 设置字体颜色为黑色
    // 显示文字
    u8g2.setCursor(0, 11);
    u8g2.print("已开机！");

    Serial.println("屏幕初始化完成！");

    // 初始化音频模块audio1
    audio1.init();
    // 设置音频输出引脚和音量
    audio2.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio2.setVolume(volume);

    // 初始化Preferences
    preferences.begin("wifi_store");
    preferences.begin("music_store");
    preferences.begin("llm_store");
    // 连接网络
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("正在连接网络······");
    int result = wifiConnect();

    if (result == 1)
    {
        Serial.println("音频模块初始化完成！网络连接成功！开始加载大模型参数！");
        loadModel();

        // 从百度服务器获取当前时间
        getTimeFromServer();
        // 使用当前时间生成WebSocket连接的URL
        url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
        url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
        // 记录当前时间，用于后续时间戳比较
        urlTime = millis();

        // 清空屏幕，在屏幕上输出提示信息
        tft.fillScreen(TFT_WHITE);
        u8g2.setCursor(0, 11);
        u8g2.print("网络连接成功！");
        displayWrappedText("请进行语音唤醒或按boot键开始对话！", 0, u8g2.getCursorY() + 12, width);
        audio2.connecttospeech("系统初始化完毕，请使用唤醒词唤醒我。", "zh");
    }
    else
    {
        awake_flag = 1;
        openWeb();
    }
    
    // 清空串口数据
    while (Serial2.available()) 
    {
        Serial2.read();
    }
    // 初始化SPIFFS文件系统
    if(!SPIFFS.begin(true))
    {
        Serial.println("初始化SPIFFS时发生错误");
        return;
    }

    // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
    TJpgDec.setJpgScale(4);

    // The decoder must be given the exact name of the rendering function above
    TJpgDec.setCallback(tft_output);
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

    #ifdef ASRPRO
        // 进入待机状态时刷新屏幕
        if (millis() - endTime > 2000 && audio2.isplaying == 0 && ((shuaxin == 1 && chouxiang == 0) || (chouxiang != 0 && startPlay == false)))
        {
            // 清空屏幕，在屏幕上输出提示信息
            tft.fillScreen(TFT_WHITE);
            u8g2.setCursor(0, 0);
            displayWrappedText("请进行语音唤醒或按boot键进行对话！", 0, u8g2.getCursorY() + 11, width);
            shuaxin = 0;
            // 抽象三幻神壁纸相关
            chouxiang = 0;
            conflag = 0;
            awake_flag = 0;     // 已唤醒，退出唤醒状态，进入待机状态
        }

        // 显示表情动图
        if (millis() - startTime > 80 && audio2.isplaying == 0 && awake_flag == 0 && shuaxin == 0)
        {
            startTime = millis();
            tft.pushImage(0, 90, width, 60, image0[i]);   // 用于壁纸显示的代码
            i = i >= image0_size - 1 ? 0 : i + 1;
        }

        // 唤醒词识别，并触发唤醒语音
        if (digitalRead(awake) == 0 && awake_flag == 0)
        {
            shuaxin = 0;
            awake_flag = 1;      //离线唤醒模型，唤醒成功
            Answer = "喵~我在的，主人。";
            response();     //屏幕显示Answer以及语音播放
            conflag = 1;
        }

        // 接受并处理ASRPRO识别到的指令
        while (Serial2.available() && recording == 0) 
        {
            char incomingByte = Serial2.read();
            // 将接收到的字节添加到缓冲区
            buffer[globalIndex] = incomingByte;
            globalIndex++;
            Serial.print((int)incomingByte, HEX);
            // 如果接收到完整的命令包
            if (globalIndex == BUFFER_SIZE) 
            {
                if (isValidCommand(buffer)) 
                {
                    uint8_t command = buffer[2]; // 提取命令字节
                    processCommand(command);
                }
                // 清空缓冲区
                memset(buffer, 0, BUFFER_SIZE);
                globalIndex = 0;
                while (Serial2.available()) 
                {
                    Serial2.read();
                }
            }
        }

        // 触发抽象三幻神之后，如果进行了指令操作，会将conflag置为1，这就会导致三幻神播放完之后会触发连续对话，因此需要将conflag置零
        if (chouxiang != 0)
        {
            conflag = 0;
        }
    #else
        // 唤醒词识别
        if (audio2.isplaying == 0 && awake_flag == 0 && await_flag == 1)
        {
            awake_flag = 1;     // 在线唤醒模式，唤醒成功
            StartConversation();
        }
    #endif

    //通过串口进行对话
    if(Serial.available())  
    {
        String r = Serial.readString();
        r.trim();
        if(r.length() >= 2) // 一个中文字符的长度为3，一个英文字符的长度为1
        {
            if (r.startsWith("音乐http"))
            {
                r.replace("音乐", "");
                audio2.connecttohost(r.c_str());    // 播放音乐
            }
            else if (r.startsWith("图片http"))
            {
                r.replace("图片", "");
                draw_image(r.c_str());   // 绘制图片测试
            }
            else 
            {
                askquestion = r;
                processAskquestion();
                hint = 0;       // 关闭提示音
                conflag = 0;    // 关闭连续对话
            }
        }
    }

    // 检测boot按键是否按下
    if (digitalRead(key) == 0)
    {
        shuaxin = 0;
        conflag = 0;
        awake_flag = 1;      // boot唤醒模式，唤醒成功
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        StartConversation();
    }
    // 连续对话
    if (audio2.isplaying == 0 && Answer == "" && subindex == subAnswers.size() && musicplay == 0 && conflag == 1 && hint == 0)
    {
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        StartConversation();
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
        int wid = 0;
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

            int charWidth = charBytes == 3 ? 12 : 8; // 中文字符12像素宽度，英文字符6像素宽度
            if (wid + charWidth > maxWidth - cursorX)
            {
                break;
            }
            numBytes += charBytes;
            wid += charWidth;

            i += size;
        }

        if (cursorY <= height - 10)
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

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsoncon, jsonString);

    // 将字符串存储到vector中
    text.push_back(jsonString);
    
    // 清空临时JSON文档
    jsoncon.clear();

    tft.print(role);
    tft.print(": ");
    displayWrappedText(content.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
    tft.setCursor(0, u8g2.getCursorY() + 2);
}

// 实时清理较早的历史对话记录
void checkLen()
{
    size_t totalBytes = 0;

    // 计算vector中每个字符串的长度
    for (const auto& jsonStr : text) {
        totalBytes += jsonStr.length();
    }
    // String note_output = "当前上下文占用字节数（text size）: " + String(totalBytes) + "/" + String(max_context_length);
    // Serial.println(note_output);

    // 当vector中的字符串总长度超过一定字节时，删除最开始的一对对话
    if (totalBytes > max_context_length)
    {
        // Serial.println("对话上下文占用字节数超过限制，删除最开始的一对对话");
        text.erase(text.begin(), text.begin() + 2);
    }
}

DynamicJsonDocument gen_params(const char *appid, const char *domain, const char *role_set)
{
    // 创建一个容量为1500字节的动态JSON文档
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
    systemMessage["content"] = role_set;

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

DynamicJsonDocument createRequestBody(const char *model, const char *role_set)
{
    // 创建一个容量为4096字节的动态JSON文档
    DynamicJsonDocument data(4096);

    data["model"] = model;
    data["max_tokens"] = 1024;
    data["temperature"] = 0.7;
    data["presence_penalty"] = 1.5;
    data["stream"] = true;

    // 在message对象中创建一个名为text的嵌套数组
    JsonArray textArray = data.createNestedArray("messages");

    JsonObject systemMessage = textArray.createNestedObject();
    systemMessage["role"] = "system";
    systemMessage["content"] = role_set;

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

void processResponse(int status)
{
    // 如果Answer的长度超过180且音频没有播放
    if (Answer.length() >= 180 && (audio2.isplaying == 0) && flag == 0)
    {
        flag = 1;
        int subAnswerlength = 0;
        if (Answer.length() >= 300)
        {
            const char* symbols[] = {"。", "；", "？", "！"};
            int firstPeriodIndex = -1;
            for (int i = 0; i < 4 && firstPeriodIndex == -1; i++) {
                firstPeriodIndex = Answer.indexOf(symbols[i]);  // 查找出一个用来断句的符号的位置
            }
            if (firstPeriodIndex != -1)
                subAnswerlength = firstPeriodIndex + 3;
            else
                Serial.println("断句失败，大模型回复中无句号、分号、问号、感叹号断句！");
        }
        else
        {
            int lastCommaIndex = Answer.lastIndexOf("，");  // 查找最后一个逗号的位置
            if (lastCommaIndex != -1)
                subAnswerlength = lastCommaIndex + 3;
            else
                subAnswerlength = Answer.length();
        }
        String subAnswer1 = Answer.substring(0, subAnswerlength);    // 提取句子的一部分
        String note_output = "subAnswer1: " + subAnswer1;
        Serial.println(note_output);
        hint = 1;
        audio2.connecttospeech(subAnswer1.c_str(), "zh");           // 将提取的句子转换为语音
        getText("assistant", subAnswer1);                           // 显示提取的句子
        tft.setCursor(54, 152);                                     // 在屏幕上显示对话轮次
        tft.print(loopcount);
        Answer = Answer.substring(subAnswerlength);                 // 更新Answer，去掉已处理的部分
        startPlay = true;
        conflag = 1;
    }
    // 存储多段子音频
    while (Answer.length() >= 180)
    {
        int subAnswerlength = 0;
        if (Answer.length() >= 300)
        {
            const char* symbols[] = {"。", "；", "？", "！"};
            int firstPeriodIndex = -1;
            for (int i = 0; i < 4 && firstPeriodIndex == -1; i++) {
                firstPeriodIndex = Answer.indexOf(symbols[i]);  // 查找出一个用来断句的符号的位置
            }
            if (firstPeriodIndex != -1)
                subAnswerlength = firstPeriodIndex + 3;
            else
                Serial.println("断句失败，大模型回复中无句号、分号、问号、感叹号断句！");
        }
        else
        {
            int lastCommaIndex = Answer.lastIndexOf("，");
            if (lastCommaIndex != -1)
                subAnswerlength = lastCommaIndex + 3;
            else
                subAnswerlength = Answer.length();
        }
        subAnswers.push_back(Answer.substring(0, subAnswerlength));
        String note_output = "subAnswer" + String(subAnswers.size() + 1) + "：" + subAnswers[subAnswers.size() - 1];
        Serial.println(note_output);

        Answer = Answer.substring(subAnswerlength);
    }

    // 如果status为2（回复的内容接收完成），且回复的内容小于180字节
    if (status == 2 && flag == 0)
    {
        hint = 1;
        audio2.connecttospeech(Answer.c_str(), "zh");               // 将回复的内容转换为语音
        getText("assistant", Answer);                               // 显示回复的内容
        tft.setCursor(54, 152);                                     // 在屏幕上显示对话轮次
        tft.print(loopcount);
        Answer = "";                                                // 清空Answer
        startPlay = true;
        conflag = 1;
    }
}

// 将回复的文本转成语音
void onMessageCallback(WebsocketsMessage message)
{
    // 创建一个静态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节，硬件限制，无法再增加
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
            // 获取JSON数据中的payload部分
            JsonObject choices = jsonDocument["payload"]["choices"];

            // 获取status状态
            int status = choices["status"];

            // 获取文本内容
            const char *content = choices["text"][0]["content"];

            char *cleanedContent = new char[strlen(content) + 1];
            const char *removeSet = "\n*$"; // 定义需要移除的符号集
            removeChars(content, cleanedContent, removeSet);
            Serial.println(cleanedContent);

            Answer += cleanedContent;
            content = "";
            delete[] cleanedContent;

            processResponse(status);
        }
    }
}

// 问题发送给讯飞星火大模型
void onEventsCallback(WebsocketsEvent event, String data)
{
    // 当WebSocket连接打开时触发
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        // 向串口输出提示信息
        Serial.println("Send message to server0!");

        // 生成连接参数的JSON文档
        DynamicJsonDocument jsonData = gen_params(appId1.c_str(), domain1.c_str(), roleSettings[role].c_str());

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

// 提取字符串中的数字
String extractNumber(const String &str) {
  String result;
  for (size_t i = 0; i < str.length(); i++) {
    if (isDigit(str[i])) {
      result += str[i];
    }
  }
  return result;
}

// 音量控制
void VolumeSet()
{
    String numberStr = extractNumber(askquestion);
    // 显示当前音量（声音）或者当前音量（声音）是多少
    if ((askquestion.indexOf("显示") > -1 && askquestion.indexOf("音") > -1) || (askquestion.indexOf("音") > -1 && askquestion.indexOf("多") > -1))
    {
        Serial.print("当前音量为: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    else if (numberStr.length() > 0)
    {
        volume = numberStr.toInt();
        audio2.setVolume(volume);
        Serial.print("音量已调到: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    else if (askquestion.indexOf("最") > -1 && (askquestion.indexOf("高") > -1 || askquestion.indexOf("大") > -1))
    {
        volume = 100;
        audio2.setVolume(volume);
        Serial.print("音量已调到: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    else if (askquestion.indexOf("高") > -1 || askquestion.indexOf("大") > -1)
    {
        volume += 10;
        if (volume > 100)
        {
            volume = 100;
        }
        audio2.setVolume(volume);
        Serial.print("音量已调到: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    else if (askquestion.indexOf("最") > -1 && (askquestion.indexOf("低") > -1 || askquestion.indexOf("小") > -1))
    {
        volume = 0;
        audio2.setVolume(volume);
        Serial.print("音量已调到: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    else if (askquestion.indexOf("低") > -1 || askquestion.indexOf("小") > -1)
    {
        volume -= 10;
        if (volume < 0)
        {
            volume = 0;
        }
        audio2.setVolume(volume);
        Serial.print("音量已调到: ");
        Serial.println(volume);
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(66, 152, 62, 7, TFT_WHITE);    // 在屏幕上显示音量
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
}
// 音乐播放处理逻辑

// 接收stt返回的语音识别文本并做相应的逻辑处理
void onMessageCallback1(WebsocketsMessage message)
{
    // 创建一个动态JSON文档对象，用于存储解析后的JSON数据，最大容量为4096字节
    DynamicJsonDocument jsonDocument(4096);

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
    // 如果解析没有错误，从JSON数据中获取返回码，如果返回码不为0，表示出错
    if (jsonDocument["code"] != 0)
    {
        // 输出完整的JSON数据
        Serial.println(message.data());
        // 关闭WebSocket客户端
        webSocketClient1.close();
    }
    else
    {
        // 输出收到的讯飞云返回消息
        //Serial.println("xunfeiyun stt return message:");
        //Serial.println(message.data());

        // 获取JSON数据中的结果部分，并提取文本内容
        JsonArray ws = jsonDocument["data"]["result"]["ws"].as<JsonArray>();

        if (jsonDocument["data"]["status"] != 2)    //处理流式返回的内容，讯飞stt最后一次会返回一个标点符号，需要和前一次返回结果拼接起来
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

        // 获取状态码，等于2表示文本已经转换完成
        if (jsonDocument["data"]["status"] == 2)
        {
            // 如果状态码为2，表示消息处理完成
            Serial.println("status == 2");
            webSocketClient1.close();

            processAskquestion();
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
        Serial.println("Send message to xunfeiyun stt!");

        // 初始化变量
        int silence = 0;
        int firstframe = 1;
        int voicebegin = 0;
        int voice = 0;
        int null_voice = 0;

        // 创建一个静态JSON文档对象，2000一般够了，不够可以再加（最多不能超过4096），但是可能会发生内存溢出
        StaticJsonDocument<2000> doc;

        #ifdef ASRPRO
        if (conflag == 1)
        {
            tft.fillScreen(TFT_WHITE);
            u8g2.setCursor(0, 11);
            u8g2.print("连续对话中，请说话！");
        }
        else
        {
            u8g2.setCursor(0, 159);
            u8g2.print("请说话！");
        }
        #else
        if (await_flag == 1)
        {
            tft.fillScreen(TFT_WHITE);
            u8g2.setCursor(0, 11);
            u8g2.print("待机中......");
        }
        else if (conflag == 1)
        {
            tft.fillScreen(TFT_WHITE);
            u8g2.setCursor(0, 11);
            u8g2.print("连续对话中，请说话！");
        }
        else
        {
            u8g2.setCursor(0, 159);
            u8g2.print("请说话！");
        }
        #endif
        conflag = 0;
        Serial.println("开始录音");
        recording = 1;
        // 无限循环，用于录制和发送音频数据
        while (1)
        {
            #ifdef ASRPRO
            #else
            // 待机状态（语音唤醒状态）也可通过boot键启动
            if ((digitalRead(key) == 0 || Serial.available()) && await_flag == 1)
            {
                start_con = 1;      // 对话开始标识
                await_flag = 0;     // 退出待机状态
                webSocketClient1.close();
                break;
            }
            #endif
            // 清空JSON文档
            doc.clear();

            // 创建data对象
            JsonObject data = doc.createNestedObject("data");

            // 录制音频数据
            audio1.Record();

            // 计算音频数据的RMS值
            float rms = audio1.calculateRMS((uint8_t *)audio1.wavData[0], 1280);
            if (rms > 1000) // 抑制录音奇奇怪怪的噪声
            {
                rms = 8.6;
            }
            printf("RMS: %f\n", rms);

            if(null_voice >= 80)    // 如果从录音开始过了8秒才说话，讯飞stt识别会超时，所以直接结束本次录音，重新开始录音
            {
                shuaxin = 1;
                #ifdef ASRPRO
                    awake_flag = 0;      // 已唤醒，退出唤醒状态，进入待机状态
                    Answer = "主人，我先退下了，有事再找我喵~";
                    response();     //屏幕显示Answer以及语音播放
                #else
                    awake_flag = 0;     // 未唤醒，保持待机状态
                    await_flag = 1;     // 进入待机状态
                    if (start_con == 1)     // 表示正处于对话中，才回复退下，没有进入对话则继续待机
                    {
                        start_con = 0;      // 标识一次（包含多轮）对话结束
                        Answer = "主人，我先退下了，有事再找我喵~";
                        response();     //屏幕显示Answer以及语音播放
                    }
                #endif
                recording = 0;
                endTime = millis();
                // 录音超时，断开本次连接
                webSocketClient1.close();
                Serial.println("录音结束");
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
                if (null_voice > 0)
                    null_voice--;
                voice++;
                if (voice >= 10)
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
                data["format"] = "audio/L16;rate=16000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
                delay(40);
                Serial.println("录音结束");
                break;
            }

            // 处理第一帧音频数据
            if (firstframe == 1)
            {
                data["status"] = 0;
                data["format"] = "audio/L16;rate=16000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";

                JsonObject common = doc.createNestedObject("common");
                common["app_id"] = appId1.c_str();

                JsonObject business = doc.createNestedObject("business");
                business["domain"] = "iat";
                business["language"] = language.c_str();
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
                data["format"] = "audio/L16;rate=16000";
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

// 处理用户的输入askquestion
void processAskquestion()
{
    // 如果是调声音大小还有开关灯的指令，就不打断当前的语音
    if ((askquestion.indexOf("声音") == -1 && askquestion.indexOf("音量") == -1) && !((askquestion.indexOf("开") > -1 || askquestion.indexOf("关") > -1) && askquestion.indexOf("灯") > -1) && !(askquestion.indexOf("暂停") > -1 || askquestion.indexOf("恢复") > -1))
    {
        webSocketClient.close();    //关闭llm服务器，打断上一次提问的回答生成
        audio2.isplaying = 0;
        startPlay = false;

        Answer = "";
        flag = 0;
        subindex = 0;
        subAnswers.clear();
        text_temp = "";

        chouxiang = 0;
        hint = 0;
    }

    #ifdef ASRPRO
    #else
        if (askquestion.indexOf("九哥"))
        {
            askquestion.replace("九哥", "九歌");
        }

        // 如果正处于待机状态，则判断唤醒词是否正确
        if (await_flag == 1)
        {
            // 增加足够多的同音字可以提高唤醒率，支持多唤醒词唤醒(askquestion.indexOf("你好") > -1 || askquestion.indexOf("您好") > -1) &&
            if( (askquestion.indexOf("坤坤") > -1 || askquestion.indexOf("小灵") > -1 || askquestion.indexOf("丁真") > -1 || askquestion.indexOf("九歌") > -1))
            {
                start_con = 1;      // 对话开始标识
                await_flag = 0;     // 退出待机状态
                Answer = "喵~我在的，主人。";
                response();     //屏幕显示Answer以及语音播放
                conflag = 1;
            }
            else
            {
                awake_flag = 0;     // 未唤醒，保持待机状态
            }
            return;
        }
    #endif

    // 如果问句为空，播放错误提示语音
    if (askquestion == "")
    {
        Answer = "喵~主人，我没有听清，请再说一遍吧";
        response();     //屏幕显示Answer以及语音播放
        conflag = 1;
    }
    else if (askquestion.indexOf("退下") > -1)
    {
        await_flag = 1;     // 进入待机状态
        start_con = 0;      // 标识一次（包含多轮）对话结束

        awake_flag = 0;      // 已唤醒，退出唤醒状态，进入待机状态
        shuaxin = 1;
        musicplay = 0;
        Answer = "主人，我先退下了，有事再找我喵~";
        response();     //屏幕显示Answer以及语音播放
        endTime = millis();
    }
    else if (askquestion.indexOf("再见") > -1 || askquestion.indexOf("拜拜") > -1 || askquestion.indexOf("休眠") > -1)
    {
        await_flag = 1;     // 进入待机状态
        start_con = 0;      // 标识一次（包含多轮）对话结束

        awake_flag = 0;      // 已唤醒，退出唤醒状态，进入待机状态
        shuaxin = 1;
        musicplay = 0;
        Answer = "好的，我随时都在，有事请随时呼唤我，再见！喵~";
        response();     //屏幕显示Answer以及语音播放
        endTime = millis();
        #ifdef ASRPRO
            String data = "ture";
            Serial2.write(data.c_str(), data.length());
        #else
        #endif
        
    }
    else if (askquestion.indexOf("断开") > -1 && (askquestion.indexOf("网络") > -1 || askquestion.indexOf("连接") > -1))
    {
        // 断开当前WiFi连接
        WiFi.disconnect(true);
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        displayWrappedText("网络连接已断开，请重启设备以再次建立连接！", tft.getCursorX(), tft.getCursorY() + 11, width);
        openWeb();
        displayWrappedText("热点ESP32-Setup已开启，密码为12345678，可在浏览器中打开http://192.168.4.1进行网络、音乐及大模型信息配置！", 0, u8g2.getCursorY() + 12, width);
    }
    else if (askquestion.indexOf("开") && (askquestion.indexOf("配置") > -1 || askquestion.indexOf("热点") > -1))
    {
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        openWeb();
        Answer = "好的喵~已经为你打开热点ESP32-Setup啦，密码是12345678哦，连接后在浏览器输入192.168.4.1就可以进入配置界面了哦~";
        response();
    }
    else if (audio2.isplaying == 1 && askquestion.indexOf("暂停") > -1)
    {
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示

        if(audio2.isRunning())
        {   
            Serial.println("已经暂停！");
            audio2.pauseResume();
        }
        else
        {
            Serial.println("当前没有音频正在播放！");
        }
    }
    else if (audio2.isplaying == 1 && askquestion.indexOf("恢复") > -1)
    {
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示

        if(!audio2.isRunning())
        {   
            Serial.println("已经恢复！");
            audio2.pauseResume();
        }
        else
        {
            Serial.println("当前没有音频正在暂停！");
        }
    }
    else if (askquestion.indexOf("声音") > -1 || askquestion.indexOf("音量") > -1)
    {
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
        VolumeSet();    //  调整音量
        conflag = 1;
    }
    else if (askquestion.indexOf("开") > -1 && askquestion.indexOf("灯") > -1)
    {
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
        digitalWrite(light, HIGH);

        #ifdef DATA_PIN
        leds[0] = CRGB::White;
        FastLED.show();
        #endif

        conflag = 1;
    }
    else if (askquestion.indexOf("关") > -1 && askquestion.indexOf("灯") > -1)
    {
        if (chouxiang != 0)
        {
            tft.pushImage(0, 0, width, height, guichu[chouxiang - 1]);
        }
        else
            tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
        digitalWrite(light, LOW);

        #ifdef DATA_PIN
        FastLED.clear();
        FastLED.show();
        #endif

        conflag = 1;
    }
    else if (askquestion.indexOf("换") > -1 && askquestion.indexOf("模型") > -1)
    {
        String numberStr = extractNumber(askquestion);
        if (numberStr.length() > 0)
        {
            if (numberStr.toInt() > llm_num)
            {
                Answer = "喵~当前只有" + String(llm_num) + "个大模型，没有这个大模型哦";
            }
            else
            {
                llm = numberStr.toInt() - 1;
                Answer = "喵~已为你切换为第"+ numberStr + "个模型（" + llm_name[llm] + "）";
            }
        }
        else
        {
            Answer = "喵~你想要换成哪个模型呢？";
        } 
        // 这里的逻辑需要重写
        if (askquestion.indexOf("字节") > -1 || askquestion.indexOf("豆包") > -1)
        {
            llm = 0;
            Answer = "喵~已为你切换为豆包大模型";
        }
        if (askquestion.indexOf("讯飞") > -1 || askquestion.indexOf("星火") > -1)
        {
            llm = 1;
            Answer = "喵~已为你切换为星火大模型";
        }
        if (askquestion.indexOf("阿里") > -1 || askquestion.indexOf("通义") > -1 || askquestion.indexOf("千问") > -1)
        {
            llm = 2;
            Answer = "喵~已为你切换为通义千问大模型";
        } 
        if (askquestion.indexOf("Chat") > -1 || askquestion.indexOf("Gpt") > -1 || askquestion.indexOf("chat") > -1 || askquestion.indexOf("gpt") > -1)
        {
            llm = 4;
            Answer = "喵~已为你切换为Chatgpt大模型";
        }
        response();     //屏幕显示Answer以及语音播放
        conflag = 1;
    }
    else if (askquestion.indexOf("播放电台") > -1)
    {
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        tft.print("user: ");
        displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
        cursorY = u8g2.getCursorY() + 1;
        tft.setCursor(0, u8g2.getCursorY() + 2);

        audio2.connecttohost("http://lhttp.qingting.fm/live/4915/64k.mp3");
        displayWrappedText("正在播放：蜻蜓电台", tft.getCursorX(), tft.getCursorY() + 11, width);
        startPlay = true;   // 设置播放开始标志
        conStatus = 1;
        conflag = 1;
    }
    else if (conStatus == 1)
    {
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        tft.print("user: ");
        displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
        cursorY = u8g2.getCursorY() + 1;
        tft.setCursor(0, u8g2.getCursorY() + 2);

        String musicName = "";
        String musicID = "";
        preferences.begin("music_store", true);
        int numMusic = preferences.getInt("numMusic", 0);
        if (musicplay == 1)
            musicplay = 1;

        if (askquestion.indexOf("不想") > -1 || askquestion.indexOf("暂停") > -1)
        {
            musicplay = 0;
            tft.print("assistant: ");
            Answer = "好的，那主人还有其它吩咐吗？喵~";
            audio2.connecttospeech(Answer.c_str(), "zh");
            displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
            Answer = "";
            conStatus = 0;
        }
        else if (askquestion.indexOf("上一") > -1)
        {
            musicnum = musicnum - 1 >= 0 ? musicnum - 1 : numMusic - 1;
            musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
            musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
            Serial.println("音乐名称: " + musicName);
            Serial.println("音乐ID: " + musicID);

            String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
            Serial.println(audioStreamURL.c_str());
            audio2.connecttohost(audioStreamURL.c_str());
            
            if (musicplay == 0)
                askquestion = "正在播放音乐：" + musicName;
            else
                askquestion = "正在顺序播放所有音乐，当前正在播放：" + musicName;
            Serial.println(askquestion);
            displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
            startPlay = true;   // 设置播放开始标志
            if (musicplay == 0)
            {
                flag = 1;
                Answer = "音乐播放完了，主人还想听什么音乐吗？喵~";
            }
        }
        else if (askquestion.indexOf("下一") > -1)
        {
            musicnum = musicnum + 1 < numMusic ? musicnum + 1 : 0;
            musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
            musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
            Serial.println("音乐名称: " + musicName);
            Serial.println("音乐ID: " + musicID);

            String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
            Serial.println(audioStreamURL.c_str());
            audio2.connecttohost(audioStreamURL.c_str());
            
            if (musicplay == 0)
                askquestion = "正在播放音乐：" + musicName;
            else
                askquestion = "正在顺序播放所有音乐，当前正在播放：" + musicName;
            Serial.println(askquestion);
            displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
            startPlay = true;   // 设置播放开始标志
            if (musicplay == 0)
            {
                flag = 1;
                Answer = "音乐播放完了，主人还想听什么音乐吗？喵~";
            }
        }
        else if ((askquestion.indexOf("再听") > -1 || askquestion.indexOf("再放") > -1 || askquestion.indexOf("再来") > -1) && askquestion.indexOf("一") > -1)
        {
            musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
            musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
            Serial.println("音乐名称: " + musicName);
            Serial.println("音乐ID: " + musicID);

            String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
            Serial.println(audioStreamURL.c_str());
            audio2.connecttohost(audioStreamURL.c_str());
            
            if (musicplay == 0)
                askquestion = "正在播放音乐：" + musicName;
            else
                askquestion = "正在顺序播放所有音乐，当前正在播放：" + musicName;
            Serial.println(askquestion);
            displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
            startPlay = true;   // 设置播放开始标志
            if (musicplay == 0)
            {
                flag = 1;
                Answer = "音乐播放完了，主人还想听什么音乐吗？喵~";
            }
        }
        else if (askquestion.indexOf("听") > -1 || askquestion.indexOf("来") > -1 || askquestion.indexOf("放") > -1 || askquestion.indexOf("换") > -1)
        {
            if (numMusic == 0)
            {
                musicID == "";
            }
            else if (askquestion.indexOf("随便") > -1)
            {
                // 设置随机数种子
                srand(time(NULL));
                musicnum = rand() % numMusic;
                musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
                musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
                Serial.println("音乐名称: " + musicName);
                Serial.println("音乐ID: " + musicID);
            }
            else if (askquestion.indexOf("连续") > -1 || askquestion.indexOf("顺序") > -1 || askquestion.indexOf("所有") > -1)
            {
                musicplay = 1;
                if (askquestion.indexOf("继续") == -1)
                    musicnum = 0;
                musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
                musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
            }
            else if (askquestion.indexOf("最喜欢的") > -1 || askquestion.indexOf("最爱的") > -1)
            {
                musicName = "Avid";
                musicID = "1862822901";
                Serial.println("音乐名称: " + musicName);
                Serial.println("音乐ID: " + musicID);
                for (int i = 0; i < numMusic; ++i)
                {
                    if (preferences.getString(("musicId" + String(i)).c_str(), "") == musicID)  musicnum = i;
                }
            }
            else    // 查询歌名
            {
                for (int i = 0; i < numMusic; ++i)
                {
                    musicName = preferences.getString(("musicName" + String(i)).c_str(), "");
                    musicID = preferences.getString(("musicId" + String(i)).c_str(), "");
                    Serial.println("音乐名称: " + musicName);
                    Serial.println("音乐ID: " + musicID);
                    if (askquestion.indexOf(musicName.c_str()) > -1)
                    {
                        Serial.println("找到了！");
                        musicnum = i;
                        break;
                    }
                    else
                    {
                        musicID = "";
                    }
                }
            }

            if (musicID == "") 
            {
                Serial.println("未找到对应的音乐！");
                tft.print("assistant: ");
                Answer = "主人，曲库里还没有这首歌哦，换一首吧，喵~";
                audio2.connecttospeech(Answer.c_str(), "zh");
                displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
                Answer = "";
            } 
            else 
            {
                String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
                Serial.println(audioStreamURL.c_str());
                audio2.connecttohost(audioStreamURL.c_str());
                
                if (musicplay == 0)
                    askquestion = "正在播放音乐：" + musicName;
                else
                    askquestion = "正在顺序播放所有音乐，当前正在播放：" + musicName;
                Serial.println(askquestion);
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
                startPlay = true;   // 设置播放开始标志
                if (musicplay == 0)
                {
                    flag = 1;
                    Answer = "音乐播放完了，主人还想听什么音乐吗？喵~";
                }
            }
        }
        else    // 处理一般的问答请求
        {
            musicplay = 0;
            conStatus = 0;
            tft.fillScreen(TFT_WHITE);
            tft.setCursor(0, 0);
            getText("user", askquestion);
            if (askquestion.indexOf("天气") > -1 || askquestion.indexOf("几点了") > -1 || askquestion.indexOf("日期") > -1)
                LLM_request(8);
            else
                LLM_request(llm);
        }
        conflag = 1;
        preferences.end();
    }
    else if (((askquestion.indexOf("听") > -1 || askquestion.indexOf("放") > -1 || askquestion.indexOf("来") > -1) && (askquestion.indexOf("歌") > -1 || askquestion.indexOf("音乐") > -1) && askquestion.indexOf("九歌") == -1) || mainStatus == 1)
    {
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        tft.print("user: ");
        displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
        cursorY = u8g2.getCursorY() + 1;
        tft.setCursor(0, u8g2.getCursorY() + 2);

        String musicName = "";
        String musicID = "";
        preferences.begin("music_store", true);
        int numMusic = preferences.getInt("numMusic", 0);

        if (askquestion.indexOf("不想") > -1)
        {
            mainStatus = 0;
            tft.print("assistant: ");
            Answer = "好的，那主人还有其它吩咐吗？喵~";
            audio2.connecttospeech(Answer.c_str(), "zh");
            displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
            Answer = "";
        }
        else 
        {
            if (numMusic == 0)
            {
                musicID == "";
            }
            else if (askquestion.indexOf("随便") > -1)
            {
                // 设置随机数种子
                srand(time(NULL));
                musicnum = rand() % numMusic;
                musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
                musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
                Serial.println("音乐名称: " + musicName);
                Serial.println("音乐ID: " + musicID);
            }
            else if (askquestion.indexOf("连续") > -1 || askquestion.indexOf("顺序") > -1 || askquestion.indexOf("所有") > -1)
            {
                musicplay = 1;
                if (askquestion.indexOf("继续") == -1)
                    musicnum = 0;
                musicName = preferences.getString(("musicName" + String(musicnum)).c_str(), "");
                musicID = preferences.getString(("musicId" + String(musicnum)).c_str(), "");
            }
            else if (askquestion.indexOf("最喜欢的") > -1 || askquestion.indexOf("最爱的") > -1)
            {
                musicName = "Avid";
                musicID = "1862822901";
                Serial.println("音乐名称: " + musicName);
                Serial.println("音乐ID: " + musicID);
                for (int i = 0; i < numMusic; ++i)
                {
                    if (preferences.getString(("musicId" + String(i)).c_str(), "") == musicID)  musicnum = i;
                }
            }
            else    // 查询歌名
            {
                for (int i = 0; i < numMusic; ++i)
                {
                    musicName = preferences.getString(("musicName" + String(i)).c_str(), "");
                    musicID = preferences.getString(("musicId" + String(i)).c_str(), "");
                    Serial.println("音乐名称: " + musicName);
                    Serial.println("音乐ID: " + musicID);
                    if (askquestion.indexOf(musicName.c_str()) > -1)
                    {
                        Serial.println("找到了！");
                        musicnum = i;
                        break;
                    }
                    else
                    {
                        musicID = "";
                    }
                }
            }

            if (musicID == "") 
            {
                mainStatus = 1;
                Serial.println("未找到对应的音乐！");
                tft.print("assistant: ");
                Answer = "好的喵，主人，你想听哪首歌呢，喵~";
                audio2.connecttospeech(Answer.c_str(), "zh");
                displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
                Answer = "";
            } 
            else 
            {
                mainStatus = 0;
                // 自建音乐服务器（这里白嫖了网易云的音乐服务器），按照音乐数字id查找对应歌曲
                String audioStreamURL = "https://music.163.com/song/media/outer/url?id=" + musicID + ".mp3";
                Serial.println(audioStreamURL.c_str());
                audio2.connecttohost(audioStreamURL.c_str());

                if (musicplay == 0)
                    askquestion = "正在播放音乐：" + musicName;
                else
                    askquestion = "开始顺序播放所有音乐，当前正在播放：" + musicName;
                Serial.println(askquestion);
                displayWrappedText(askquestion.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
                startPlay = true;   // 设置播放开始标志
                conStatus = 1;
                if (musicplay == 0)
                {
                    flag = 1;
                    Answer = "音乐播放完了，主人还想听什么音乐吗？";
                }
            }
        }
        conflag = 1;
        preferences.end();
    }
    else    // 处理一般的问答请求
    {
        tft.fillScreen(TFT_WHITE);
        tft.setCursor(0, 0);
        getText("user", askquestion);
        if (askquestion.indexOf("天气") > -1 || askquestion.indexOf("几点了") > -1 || askquestion.indexOf("日期") > -1)
            LLM_request(8);
        else
            LLM_request(llm);
    }
    recording = 0;
}


/*---------基本不需要再改的函数---------*/
void ConnServer()
{
    // Serial.println("url:" + url);                    // 向串口输出WebSocket服务器的URL
    
    webSocketClient.onMessage(onMessageCallback);       // 设置WebSocket客户端的消息回调函数
    webSocketClient.onEvent(onEventsCallback);          // 设置WebSocket客户端的事件回调函数

    // 开始连接WebSocket服务器
    Serial.println("开始连接讯飞星火大模型服务......Begin connect to server0(Xunfei Spark LLM)......");

    // 尝试连接到WebSocket服务器
    if (webSocketClient.connect(url.c_str()))
    {
        // 如果连接成功，输出成功信息
        Serial.println("连接LLM成功！Connected to server0(Xunfei Spark LLM)!");
    }
    else
    {
        // 如果连接失败，输出失败信息
        Serial.println("连接LLM失败！Failed to connect to server0(Xunfei Spark LLM)!");
    }
}

void ConnServer1()
{
    // Serial.println("url1:" + url1);

    webSocketClient1.onMessage(onMessageCallback1);
    webSocketClient1.onEvent(onEventsCallback1);

    // Connect to WebSocket
    Serial.println("开始连接讯飞STT语音转文字服务......Begin connect to server1(Xunfei STT)......");
    if (webSocketClient1.connect(url1.c_str()))
    {
        Serial.println("连接成功！Connected to server1(Xunfei STT)!");
    }
    else
    {
        Serial.println("连接失败！Failed to connect to server1(Xunfei STT)!");
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
        displayWrappedText("请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, width);
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
    tft.fillScreen(TFT_WHITE);
    // 在屏幕上输出提示信息
    u8g2.setCursor(0, 11);
    u8g2.print("网络连接失败！请检查");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("网络设备，确认可用后");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("重启设备以建立连接！");
    displayWrappedText("或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, width);
    preferences.end();
    return 0;
}

void getTimeFromServer()
{
    String timeurl = "https://www.baidu.com";   // 定义用于获取时间的URL
    HTTPClient http;                // 创建HTTPClient对象
    http.begin(timeurl);            // 初始化HTTP连接
    const char *headerKeys[] = {"Date"};        // 定义需要收集的HTTP头字段
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));    // 设置要收集的HTTP头字段
    int httpCode = http.GET();      // 发送HTTP GET请求
    Date = http.header("Date");     // 从HTTP响应头中获取Date字段
    Serial.println(Date);           // 输出获取到的Date字段到串口
    http.end();                     // 结束HTTP连接

    // delay(50); // 根据实际情况可以添加延时，以便避免频繁请求
}

// 拼接讯飞websocket鉴权参数
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

// 移除LLM回复中在屏幕上进行显示时没用的符号
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

// 主流大模型通用流式调用
void LLM_request(int llmNum)
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(llm_url[llmNum]);
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + llm_apiKey[llmNum];
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.print("Sending a message to ");
    Serial.println(llm_name[llmNum]);

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = createRequestBody(llm_model[llmNum].c_str(), roleSettings[role].c_str());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            if (llm_name[llmNum] == "Ollama" && line.length() > 50)
            {
                line = "data:" + line;
            }
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                
                int status = 0;
                if (data == "[DONE]")
                {
                    status = 2;
                    Serial.println("status: 2");
                    processResponse(status);
                    stream->stop();
                    break;
                }

                // 解析收到的数据
                StaticJsonDocument<1024> jsonResponse;
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse.containsKey("choices") && jsonResponse["choices"][0].containsKey("delta"))
                    {
                        if (jsonResponse["choices"][0]["delta"].containsKey("content") && !jsonResponse["choices"][0]["delta"]["content"].isNull())
                        {
                            const char *content = jsonResponse["choices"][0]["delta"]["content"];
                            char *cleanedContent = new char[strlen(content) + 1];
                            const char *removeSet = "\n*$"; // 定义需要移除的符号集
                            removeChars(content, cleanedContent, removeSet);
                            Serial.println(cleanedContent);

                            Answer += cleanedContent;
                            content = "";
                            delete[] cleanedContent;
                        }

                        if (jsonResponse["choices"][0].containsKey("finish_reason") && jsonResponse["choices"][0]["finish_reason"] == "stop")
                        {
                            status = 2;
                            Serial.println("status: 2");
                        }

                        processResponse(status);

                        if (status == 2)
                        {
                            stream->stop();
                            break;
                        }
                    }
                    else if (jsonResponse.containsKey("message"))
                    {
                        if (jsonResponse["message"].containsKey("content") && !jsonResponse["message"]["content"].isNull())
                        {
                            const char *content = jsonResponse["message"]["content"];
                            char *cleanedContent = new char[strlen(content) + 1];
                            const char *removeSet = "\n*$"; // 定义需要移除的符号集
                            removeChars(content, cleanedContent, removeSet);
                            Serial.println(cleanedContent);

                            Answer += cleanedContent;
                            content = "";
                            delete[] cleanedContent;
                        }

                        if (jsonResponse.containsKey("done") && jsonResponse["done"])
                        {
                            status = 2;
                            Serial.println("status: 2");
                        }

                        processResponse(status);

                        if (status == 2)
                        {
                            stream->stop();
                            break;
                        }
                    }
                    else {
                        Serial.println("choices not found");
                    }
                }    
                else 
                {
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
        http.end();
        return;
    } 
    else 
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
        return;
    }
}

