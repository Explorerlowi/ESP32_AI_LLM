#include "Web_Scr_set.h"

// 定义引脚
#define key 0       //boot按键引脚
#define led 8       //板载led引脚
#define light 38     // 灯光引脚
// 定义音频放大模块的I2S引脚定义
#define I2S_DOUT 5 // DIN引脚
#define I2S_BCLK 6 // BCLK引脚
#define I2S_LRC 7  // LRC引脚

//  优先事项！！！一定要做，不做的话麦克风会因为引脚冲突无法工作
//  找到.pio\libdeps\upesy_wroom\TFT_eSPI路径下的User_Setup.h文件，删除它，然后将根目录下的User_Setup.h文件剪切粘贴过去



int llm = 0;    // 大模型选择参数:0:豆包，1：讯飞星火，2：通义千问，3：通义千问智能体应用，4：Chatgpt（经常会请求失败），5：Dify（用官网提供的免费的API服务也经常会请求失败）
String llm_name[] = {"豆包大模型", "讯飞星火大模型", "通义千问大模型", "通义千问智能体应用", "Chatgpt大模型", "Dify"};

// 选哪个模型，就填哪个模型的参数
// 豆包大模型的参数
String model1 = "";   // 在线推理接入点名称，必填
const char* doubao_apiKey = "";     // 火山引擎API Key，必填
String apiUrl = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";

// 通义千问大模型的参数
String model2 = "";
const char* tongyi_apiKey = "";
String apiUrl_tongyi = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"; // 通义千问的API地址

// 通义千问智能体应用的参数
String tongyi_appid = "";
String apiUrl_tongyi_app = "https://dashscope.aliyuncs.com/api/v1/apps/" + tongyi_appid + "/completion"; // 通义千问智能体应用的API地址

// Chatgpt的参数
String model3 = "gpt-4o-mini";
const char* openai_apiKey = "";
String apiUrl_chatgpt = "https://api.oaipro.com/v1/chat/completions"; // Chatgpt的API代理地址

// Dify参数
const String dify_apiKey = "";
String apiUrl_dify = "https://api.dify.ai/v1/chat-messages"; // Dify的API代理地址

// 讯飞stt和大模型服务的参数
String APPID = "";                             // App ID,必填
String APISecret = ""; // API Secret，必填
String APIKey = "";    // API Key，必填
String appId1 = APPID;
String domain1 = "4.0Ultra";    // 根据需要更改4.0Ultra、max-32k、generalv3.5、pro-128k、generalv3、lite
String websockets_server = "ws://spark-api.xf-yun.com/v4.0/chat";   // 根据需要更改/v4.0/chat、/chat/max-32k、/v3.5/chat、/chat/pro-128k、/v3.1/chat、/v1.1/chat
String websockets_server1 = "ws://iat-api.xfyun.cn/v2/iat";         // 不需要更改
// 讯飞stt语种设置
String language = "zh_cn";     //zh_cn：中文（支持简单的英文识别）en_us：English

// 角色设定
String roleSet = "你是一个二次元魔法猫娘，你的名字叫小灵，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。";
String roleSet1 = "你是一个二次元魔法猫娘，你的名字叫小灵，你的性格可爱活泼，说话简短，同时带有攻击嘲讽意味，你喜欢在回答的末尾加一个“喵”字。";

// 定义一些全局变量
bool ledstatus = true;          // 控制led闪烁
bool startPlay = false;
unsigned long urlTime = 0;
int noise = 50;                 // 噪声门限值
int volume = 80;               // 初始音量大小（最小0，最大100）
//音乐播放
int mainStatus = 0;
int conStatus = 0;
int musicnum = 0;   //音乐位置下标
int musicplay = 0;  // 是否进入连续播放音乐状态
int cursorY = 0;
//语音唤醒
int awake_flag = 1;

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
int image_show = 0;
String conversation_id = "";
String session_id = "";

using namespace websockets; // 使用WebSocket命名空间
// 创建WebSocket客户端对象
WebsocketsClient webSocketClient;   //与llm通信
WebsocketsClient webSocketClient1;  //与stt通信

// 创建音频对象
Audio1 audio1;
Audio2 audio2(false, 3, I2S_NUM_1); 
// 参数: 是否使用内部DAC（数模转换器）如果设置为true，将使用ESP32的内部DAC进行音频输出。否则，将使用外部I2S设备。
// 指定启用的音频通道。可以设置为1（只启用左声道）或2（只启用右声道）或3（启用左右声道）
// 指定使用哪个I2S端口。ESP32有两个I2S端口，I2S_NUM_0和I2S_NUM_1。可以根据需要选择不同的I2S端口。

// 函数声明
DynamicJsonDocument gen_params(const char *appid, const char *domain);
DynamicJsonDocument gen_params_http(const char *model, const char *role_set);
DynamicJsonDocument gen_params_dify();
DynamicJsonDocument gen_params_tongyi_app();
void processResponse(int status);
void displayWrappedText(const string &text1, int x, int y, int maxWidth);
void getText(String role, String content);
void checkLen();
void removeChars(const char *input, char *output, const char *removeSet);
float calculateRMS(uint8_t *buffer, int bufferSize);
void ConnServer();
void ConnServer1();
void voicePlay();
int wifiConnect();
void getTimeFromServer();
String getUrl(String server, String host, String path, String date);
void doubao();
void tongyi();
void tongyi_app();
void chatgpt();
void dify();

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
            conflag = 1;
        }
        // 设置开始播放标志
        startPlay = true;
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

void StartConversation()
{
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

void imageshow()
{
    tft.fillScreen(TFT_WHITE);
    int count = 2;
    while (count)
    {
        for (int i = 0;i < bizhi_size;i++)
        {
            tft.pushImage(0, 0, width, height, bizhi[i]);   // 用于壁纸显示的代码
            for (int j=0;j<100;j++)     // 每隔一秒显示一张，同时保证显示壁纸时可以正常播放语音
            {
                audio2.loop();
                delay(10);
            }
        }
        count--;
    }
    image_show = 0;
}

void setup()
{
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 初始化 UART2
    Serial2.begin(115200, SERIAL_8N1, 19, 20);
    // 配置引脚模式
    // 配置按键引脚为上拉输入模式，用于boot按键检测
    pinMode(key, INPUT_PULLUP);

    // 将led设置为输出模式
    pinMode(led, OUTPUT);
    // 将light设置为输出模式
    pinMode(light, OUTPUT);
    // 将light初始化为低电平
    digitalWrite(light, LOW);

    Serial.println("引脚初始化完成！");

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
    // 连接网络
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("正在连接网络······");
    int result = wifiConnect();

    if (result == 1)
    {
        Serial.println("音频模块初始化完成！网络连接成功！");
        // 从百度服务器获取当前时间
        getTimeFromServer();
        // 使用当前时间生成WebSocket连接的URL
        url = getUrl(websockets_server, "spark-api.xf-yun.com", websockets_server.substring(25), Date);
        url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);

        // 清空屏幕，在屏幕上输出提示信息
        tft.fillScreen(TFT_WHITE);
        u8g2.setCursor(0, 11);
        u8g2.print("网络连接成功！");
        displayWrappedText("请进行语音唤醒或按boot键开始对话！", 0, u8g2.getCursorY() + 12, width);
        awake_flag = 0;
    }
    else
    {
        openWeb();
    }
    // 记录当前时间，用于后续时间戳比较
    urlTime = millis();
    // 延迟1000毫秒，便于用户查看屏幕显示的信息，同时使设备充分初始化
    delay(1000);
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
    if (audio2.isplaying == 0 && awake_flag == 0 && await_flag == 1)
    {
        awake_flag = 1;
        StartConversation();
    }

    // 检测boot按键是否按下
    if (digitalRead(key) == 0)
    {
        conflag = 0;
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        StartConversation();
    }
    // 连续对话
    if (audio2.isplaying == 0 && Answer == "" && subindex == subAnswers.size() && musicplay == 0 && conflag == 1 && image_show == 0)
    {
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        StartConversation();
    }

    if (audio2.isplaying == 1 && image_show == 1)
    {
        imageshow();
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

            int charWidth = charBytes == 3 ? 12 : 6; // 中文字符12像素宽度，英文字符6像素宽度
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
    // Serial.print("jsoncon：");
    // Serial.println(jsoncon.as<String>());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsoncon, jsonString);

    // 将字符串存储到vector中
    text.push_back(jsonString);

    // 输出vector中的内容
    Serial.println("对话上下文（Conversation context）：");
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
    displayWrappedText(content.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
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
    Serial.print("当前上下文占用字节数（text size）: ");
    Serial.println(totalBytes);
    // 当vector中的字符串总长度超过800字节时，删除最开始的一对对话
    if (totalBytes > 800)
    {
        Serial.println("totalBytes大于800,删除最开始的一对对话");
        text.erase(text.begin(), text.begin() + 2);
    }
    // 函数没有返回值，直接修改传入的JSON数组
    // return textArray; // 注释掉的代码，表明此函数不返回数组
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

DynamicJsonDocument gen_params_http(const char *model, const char *role_set)
{
    // 创建一个容量为1500字节的动态JSON文档
    DynamicJsonDocument data(1500);

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

DynamicJsonDocument gen_params_dify()
{
    // 创建一个容量为1500字节的动态JSON文档
    DynamicJsonDocument data(500);

    JsonObject inputsObj = data.createNestedObject("inputs"); // 创建空对象
    data["query"] = askquestion.c_str();
    data["response_mode"] = "streaming";
    data["conversation_id"] = conversation_id.c_str();
    data["user"] = "abc-123456";

    // 返回构建好的JSON文档
    return data;
}

DynamicJsonDocument gen_params_tongyi_app()
{
    // 创建一个容量为1500字节的动态JSON文档
    DynamicJsonDocument data(500);

    JsonObject inputObj = data.createNestedObject("input"); // 创建空对象
    inputObj["prompt"] = askquestion.c_str();
    inputObj["session_id"] = session_id.c_str();
    JsonObject parametersObj = data.createNestedObject("parameters"); // 创建空对象
    parametersObj["incremental_output"] = true;
    JsonObject debugObj = data.createNestedObject("debug"); // 创建空对象

    // 返回构建好的JSON文档
    return data;
}

void processResponse(int status)
{
    // 如果Answer的长度超过180且音频没有播放
    if (Answer.length() >= 180 && (audio2.isplaying == 0) && flag == 0)
    {
        if (Answer.length() >= 300)
        {
            // 查找第一个句号的位置
            int firstPeriodIndex = Answer.indexOf("。");
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到句号，则查找第一个分号的位置
                int firstPeriodIndex = Answer.indexOf("；");
            }
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到分号，则查找第一个问号的位置
                int firstPeriodIndex = Answer.indexOf("？");
            }
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到问号，则查找第一个感叹号的位置
                int firstPeriodIndex = Answer.indexOf("！");
            }
            // 如果找到
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
                tft.setCursor(54, 152);
                tft.print(loopcount);
                flag = 1;

                // 更新Answer，去掉已处理的部分
                Answer = Answer.substring(firstPeriodIndex + 3);
                subAnswer1.clear();
                // 设置播放开始标志
                startPlay = true;
            }
            else
            {
                Serial.println("问题里面句号、分号、问号、感叹号断句都没有！");
            }
        }
        else
        {
            // 查找最后一个逗号的位置
            int lastCommaIndex = Answer.lastIndexOf("，");
            if (lastCommaIndex != -1)
            {
                String subAnswer1 = Answer.substring(0, lastCommaIndex + 3);
                Serial.print("subAnswer1:");
                Serial.println(subAnswer1);
                audio2.connecttospeech(subAnswer1.c_str(), "zh");
                getText("assistant", subAnswer1);
                tft.setCursor(54, 152);
                tft.print(loopcount);
                flag = 1;
                Answer = Answer.substring(lastCommaIndex + 3);
                subAnswer1.clear();
                startPlay = true;
            }
            else
            {  
                String subAnswer1 = Answer.substring(0, Answer.length());
                Serial.print("subAnswer1:");
                Serial.println(subAnswer1);
                audio2.connecttospeech(subAnswer1.c_str(), "zh");
                getText("assistant", subAnswer1);
                tft.setCursor(54, 152);
                tft.print(loopcount);
                flag = 1;
                Answer = Answer.substring(Answer.length());
                subAnswer1.clear();
                startPlay = true;
            }
        }
        conflag = 1;
    }
    // 存储多段子音频
    while (Answer.length() >= 180)
    {
        if (Answer.length() >= 300)
        {
            // 查找第一个句号的位置
            int firstPeriodIndex = Answer.indexOf("。");
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到句号，则查找第一个分号的位置
                int firstPeriodIndex = Answer.indexOf("；");
            }
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到分号，则查找第一个问号的位置
                int firstPeriodIndex = Answer.indexOf("？");
            }
            if (firstPeriodIndex == -1)
            {
                // 如果没有找到问号，则查找第一个感叹号的位置
                int firstPeriodIndex = Answer.indexOf("！");
            }
            // 如果找到
            if (firstPeriodIndex != -1)
            {
                subAnswers.push_back(Answer.substring(0, firstPeriodIndex + 3));
                Serial.print("subAnswer");
                Serial.print(subAnswers.size() + 1);
                Serial.print("：");
                Serial.println(subAnswers[subAnswers.size() - 1]);

                Answer = Answer.substring(firstPeriodIndex + 3);
            }
            else
            {
                Serial.println("问题里面句号、分号、问号、感叹号断句都没有！");
            }
        }
        else
        {
            int lastCommaIndex = Answer.lastIndexOf("，");
            if (lastCommaIndex != -1)
            {
                subAnswers.push_back(Answer.substring(0, lastCommaIndex + 3));
                Serial.print("subAnswer");
                Serial.print(subAnswers.size() + 1);
                Serial.print("：");
                Serial.println(subAnswers[subAnswers.size() - 1]);

                Answer = Answer.substring(lastCommaIndex + 3);
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
    }

    // 如果status为2（回复的内容接收完成），且回复的内容小于180字节
    if (status == 2 && flag == 0)
    {
        // 播放最终转换的文本
        audio2.connecttospeech(Answer.c_str(), "zh");
        // 显示最终转换的文本
        getText("assistant", Answer);
        tft.setCursor(54, 152);
        tft.print(loopcount);
        Answer = "";
        conflag = 1;
        startPlay = true;
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
        DynamicJsonDocument jsonData = gen_params(appId1.c_str(), domain1.c_str(), roleSet1.c_str());

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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
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
        // 在屏幕上显示音量
        tft.fillRect(66, 152, 62, 7, TFT_WHITE);
        tft.setCursor(66, 152);
        tft.print("volume:");
        tft.print(volume);
    }
    conflag = 1;
}
// 音乐播放处理逻辑

void response()
{
    tft.fillScreen(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.print("assistant: ");
    audio2.connecttospeech(Answer.c_str(), "zh");
    displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY() + 11, width);
    Answer = "";
}

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
        Serial.println("xunfeiyun stt return message:");
        Serial.println(message.data());

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
            }

            if (askquestion.indexOf("九哥"))
            {
                askquestion.replace("九哥", "九歌");
            }

            // 如果正处于待机状态，则判断唤醒词是否正确
            if (await_flag == 1)
            {
                // 增加足够多的同音字可以提高唤醒率，支持多唤醒词唤醒(askquestion.indexOf("你好") > -1 || askquestion.indexOf("您好") > -1) &&
                if( (askquestion.indexOf("坤坤") > -1 || askquestion.indexOf("小白") > -1 || askquestion.indexOf("丁真") > -1 || askquestion.indexOf("九歌") > -1))
                {
                    await_flag = 0;     //退出待机状态
                    start_con = 1;      //对话开始标识
                    Answer = "喵~我在的，主人。";
                    response();     //屏幕显示Answer以及语音播放
                    conflag = 1;
                    return;
                }
                else
                {
                    // 将awake_flag置为0，继续进行唤醒词识别
                    awake_flag = 0;
                    return;
                }
            }

            // 如果问句为空，播放错误提示语音
            if (askquestion == "")
            {
                Answer = "喵~主人，我没有听清，请再说一遍吧";
                response();     //屏幕显示Answer以及语音播放
                conflag = 1;
            }
            else if (askquestion.indexOf("退下") > -1 || askquestion.indexOf("再见") > -1 || askquestion.indexOf("拜拜") > -1)
            {
                start_con = 0;      // 标识一轮对话结束
                musicplay = 0;
                Answer = "喵~主人，我先退下了，有事再叫我。";
                response();     //屏幕显示Answer以及语音播放
                await_flag = 1;     // 进入待机状态
                awake_flag = 0;     // 继续进行唤醒词识别
            }
            else if (askquestion.indexOf("断开") > -1 && (askquestion.indexOf("网络") > -1 || askquestion.indexOf("连接") > -1))
            {
                // 断开当前WiFi连接
                WiFi.disconnect(true);
                tft.fillScreen(TFT_WHITE);
                tft.setCursor(0, 0);
                displayWrappedText("网络连接已断开，请重启设备以再次建立连接！", tft.getCursorX(), tft.getCursorY() + 11, width);
                openWeb();
                displayWrappedText("热点ESP32-Setup已开启，密码为12345678，可在浏览器中打开http://192.168.4.1进行网络和音乐信息配置！", 0, u8g2.getCursorY() + 12, width);
            }
            else if (audio2.isplaying == 1 && askquestion.indexOf("暂停") > -1)
            {
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
                tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
                VolumeSet();    //  调整音量
            }
            else if (askquestion.indexOf("开") > -1 && askquestion.indexOf("灯") > -1)
            {
                tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
                digitalWrite(light, HIGH);
                conflag = 1;
            }
            else if (askquestion.indexOf("关") > -1 && askquestion.indexOf("灯") > -1)
            {
                tft.fillRect(0, 148, 50, 12, TFT_WHITE);     // 清空左下角的“请说话！”提示
                digitalWrite(light, LOW);
                conflag = 1;
            }
            else if (askquestion.indexOf("换") > -1 && askquestion.indexOf("模型") > -1)
            {
                String numberStr = extractNumber(askquestion);
                if (numberStr.length() > 0)
                {
                    if (numberStr.toInt() > llm_name->length())
                    {
                        Answer = "喵~当前只有" + String(llm_name->length()) + "个大模型，没有这个大模型哦";
                    }
                    else
                    {
                        llm = numberStr.toInt() - 1;
                        Answer = "喵~已为你切换为第"+ numberStr + "个模型（" + llm_name[llm] + "）";
                    }
                }
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
                    if (askquestion.indexOf("智能体") > -1 || askquestion.indexOf("应用") > -1)
                    {
                        llm = 3;
                        Answer = "喵~已为你切换为通义千问智能体应用";
                    }
                    else
                    {
                        llm = 2;
                        Answer = "喵~已为你切换为通义千问大模型";
                    }
                } 
                if (askquestion.indexOf("Chat") > -1 || askquestion.indexOf("Gpt") > -1 || askquestion.indexOf("chat") > -1 || askquestion.indexOf("gpt") > -1)
                {
                    llm = 4;
                    Answer = "喵~已为你切换为Chatgpt大模型";
                }
                if (askquestion.indexOf("Dify") > -1 || askquestion.indexOf("dify") > -1)
                {
                    llm = 5;
                    Answer = "喵~已为你切换为Dify";
                }   
                response();     //屏幕显示Answer以及语音播放
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
                    conflag = 1;
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
                    conflag = 1;
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
                    conflag = 1;
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
                    conflag = 1;
                }
                else if (askquestion.indexOf("听") > -1 || askquestion.indexOf("来") > -1 || askquestion.indexOf("放") > -1 || askquestion.indexOf("换") > -1)
                {
                    if (askquestion.indexOf("随便") > -1)
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
                        conflag = 1;
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
                        conflag = 1;
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
                        ConnServer();
                    else
                    {
                        switch (llm)
                        {
                        case 0:
                            doubao();       // 豆包
                            break;
                        case 1:
                            ConnServer();   // 讯飞星火
                            break;
                        case 2:
                            tongyi();       // 通义千问
                            break;
                        case 3:
                            tongyi_app();   // 通义千问智能体应用
                            break;
                        case 4:
                            chatgpt();      // chatgpt
                            break;
                        case 5:
                            dify();         //  Dify
                            break;
                        default:
                            ConnServer();   // 讯飞星火
                            break;
                        }
                    }
                }
                preferences.end();
            }
            else if (((askquestion.indexOf("听") > -1 || askquestion.indexOf("放") > -1) && (askquestion.indexOf("歌") > -1 || askquestion.indexOf("音乐") > -1) && askquestion.indexOf("九歌") == -1) || mainStatus == 1)
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
                    conflag = 1;
                    return;
                }

                if (askquestion.indexOf("随便") > -1)
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
                    conflag = 1;
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
                    conflag = 1;
                }
                preferences.end();
            }
            else if (askquestion.indexOf("放") > -1 && (askquestion.indexOf("图片") > -1 || askquestion.indexOf("幻灯片") > -1))
            {
                tft.fillScreen(TFT_WHITE);
                tft.setCursor(0, 0);
                getText("user", askquestion);
                Answer = "这就开始放映主人喜欢的图片，喵~";
                audio2.connecttospeech(Answer.c_str(), "zh");
                getText("assistant", Answer);
                Answer = "";
                image_show = 1;
                conflag = 1;
            }
            else    // 处理一般的问答请求
            {
                tft.fillScreen(TFT_WHITE);
                tft.setCursor(0, 0);
                getText("user", askquestion);
                if (askquestion.indexOf("天气") > -1 || askquestion.indexOf("几点了") > -1 || askquestion.indexOf("日期") > -1)
                    ConnServer();
                else
                {
                    switch (llm)
                    {
                    case 0:
                        doubao();       // 豆包
                        break;
                    case 1:
                        ConnServer();   // 讯飞星火
                        break;
                    case 2:
                        tongyi();       // 通义千问
                        break;
                    case 3:
                        tongyi_app();   // 通义千问智能体应用
                        break;
                    case 4:
                        chatgpt();      // chatgpt
                        break;
                    case 5:
                        dify();         //  Dify
                        break;  
                    default:
                        ConnServer();   // 讯飞星火
                        break;
                    }
                }
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
        Serial.println("Send message to xunfeiyun stt!");

        // 初始化变量
        int silence = 0;
        int firstframe = 1;
        int voicebegin = 0;
        int voice = 0;
        int null_voice = 0;

        // 创建一个静态JSON文档对象，2000一般够了，不够可以再加（最多不能超过4096），但是可能会发生内存溢出
        StaticJsonDocument<2000> doc;

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
        conflag = 0;

        Serial.println("开始录音 Start recording......");
        // 无限循环，用于录制和发送音频数据
        while (1)
        {
            // 待机状态（语音唤醒状态）也可通过boot键启动
            if (digitalRead(key) == 0 && await_flag == 1)
            {
                start_con = 1;      //对话开始标识
                await_flag = 0;
                webSocketClient1.close();
                break;
            }
            // 清空JSON文档
            doc.clear();

            // 创建data对象
            JsonObject data = doc.createNestedObject("data");

            // 录制音频数据
            audio1.Record();

            // 计算音频数据的RMS值
            float rms = calculateRMS((uint8_t *)audio1.wavData[0], 1280);
            if (null_voice < 10 && rms > 1000) // 抑制录音初期奇奇怪怪的噪声
            {
                rms = 8.6;
            }
            printf("RMS: %f\n", rms);

            if(null_voice >= 80)    // 如果从录音开始过了8秒才说话，讯飞stt识别会超时，所以直接结束本次录音，重新开始录音
            {
                if (start_con == 1)     // 表示正处于对话中，才回复退下，没有进入对话则继续待机
                {
                    start_con = 0;      // 退出对话
                    Answer = "主人，我先退下了，有事再找我喵~";
                    response();     //屏幕显示Answer以及语音播放
                }
                // 标识正处于待机状态
                await_flag = 1;
                // 将awake_flag置为0,继续进行唤醒词识别
                awake_flag = 0;
                // 录音超时，断开本次连接
                webSocketClient1.close();
                Serial.println("录音结束 End of recording.");
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

                String jsonString;
                serializeJson(doc, jsonString);

                webSocketClient1.send(jsonString);
                delay(40);
                Serial.println("录音结束 End of recording.");
                break;
            }

            // 处理第一帧音频数据
            if (firstframe == 1)
            {
                data["status"] = 0;
                data["format"] = "audio/L16;rate=8000";
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

// 问题发送给豆包大模型并接受回答，然后转成语音
void doubao()
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + doubao_apiKey;
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.println("Send message to doubao!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = gen_params_http(model1.c_str(), roleSet.c_str());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        Serial.print("httpResponseCode: ");
        Serial.println(httpResponseCode);
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据，不建议，因为太多了，一次才一两个字
                //Serial.print("data: ");
                //Serial.println(data);

                int status = 0;
                StaticJsonDocument<400> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    const char *content = jsonResponse["choices"][0]["delta"]["content"];
                    if (jsonResponse["choices"][0]["delta"]["content"] != "")
                    {
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
                    }
                    else
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
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
        /*/ 非流式调用，不推荐，因为没有足够大小的DynamicJsonDocument来存储一次性返回的长文本回复
        String response = http.getString();
        http.end();
        Serial.println(response);

        // Parse JSON response
        int status = 0;
        DynamicJsonDocument jsonDoc(1024);
        deserializeJson(jsonDoc, response);
        const char *content = jsonDoc["choices"][0]["message"]["content"];
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
        while (Answer != "")
        {
            if (Answer.length() < 180)
                status = 2;
            processResponse(status);
        }*/
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

// 问题发送给通义千问大模型并接受回答，然后转成语音
void tongyi()
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(apiUrl_tongyi);
    String token_key = String("Bearer ") + tongyi_apiKey;
    http.addHeader("Authorization", token_key);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-DashScope-SSE", "enable");

    // 向串口输出提示信息
    Serial.println("Send message to tongyiqianwen!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = gen_params_http(model2.c_str(), roleSet1.c_str());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        Serial.print("httpResponseCode: ");
        Serial.println(httpResponseCode);
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据
                //Serial.print("data: ");
                //Serial.println(data);

                int status = 0;
                StaticJsonDocument<1024> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    const char *content = jsonResponse["choices"][0]["delta"]["content"];
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

                    if (jsonResponse["choices"][0]["finish_reason"] == "stop")
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
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
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

// 问题发送给Dify并接受回答，然后转成语音
void tongyi_app()
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(apiUrl_tongyi_app);
    String token_key = String("Bearer ") + tongyi_apiKey;
    http.addHeader("Authorization", token_key);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-DashScope-SSE", "enable");

    // 向串口输出提示信息
    Serial.println("Send message to Tongyi_app!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = gen_params_tongyi_app();

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        Serial.print("httpResponseCode: ");
        Serial.println(httpResponseCode);
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据
                // Serial.print("data: ");
                // Serial.println(data);

                int status = 0;
                StaticJsonDocument<1024> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse.containsKey("output"))
                    {
                        if (jsonResponse["output"]["finish_reason"] != "stop")
                        {
                            const char *content = jsonResponse["output"]["text"];
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
                        }
                        else
                        {
                            session_id = "";
                            const char *content = jsonResponse["output"]["session_id"];
                            session_id += content;
                            content = "";
                            status = 2;
                            Serial.println("status: 2");
                        }
                    }

                    processResponse(status);

                    if (status == 2)
                    {
                        stream->stop();
                        break;
                    }
                }
                else {
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
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

// 问题发送给Chatgpt并接受回答，然后转成语音
void chatgpt()
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(apiUrl_chatgpt);
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.println("Send message to chatgpt!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = gen_params_http(model3.c_str(), roleSet1.c_str());

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        Serial.print("httpResponseCode: ");
        Serial.println(httpResponseCode);
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据
                //Serial.print("data: ");
                //Serial.println(data);

                int status = 0;
                StaticJsonDocument<1024> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse["choices"][0]["finish_reason"] == "stop")
                    {
                        status = 2;
                        Serial.println("status: 2");
                    }
                    else
                    {
                        const char *content = jsonResponse["choices"][0]["delta"]["content"];
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
                    }

                    processResponse(status);

                    if (status == 2)
                    {
                        stream->stop();
                        break;
                    }
                }
                else {
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
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

// 问题发送给Dify并接受回答，然后转成语音
void dify()
{
    HTTPClient http;
    http.setTimeout(20000);     // 设置请求超时时间
    http.begin(apiUrl_dify);
    http.addHeader("Authorization", "Bearer " + dify_apiKey);
    http.addHeader("Content-Type", "application/json");

    // 向串口输出提示信息
    Serial.println("Send message to Dify!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = gen_params_dify();

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200) {
        Serial.print("httpResponseCode: ");
        Serial.println(httpResponseCode);
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr();   // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected()) {   // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n');    // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:")) {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据
                // Serial.print("data: ");
                // Serial.println(data);

                int status = 0;
                StaticJsonDocument<1024> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse.containsKey("answer"))
                    {
                        if (jsonResponse["answer"] != "")
                        {
                            const char *content = jsonResponse["answer"];
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
                        }
                        else
                        {
                            conversation_id = "";
                            const char *content = jsonResponse["conversation_id"];
                            conversation_id += content;
                            content = "";
                            status = 2;
                            Serial.println("status: 2");
                        }
                    }

                    processResponse(status);

                    if (status == 2)
                    {
                        stream->stop();
                        break;
                    }
                }
                else {
                    Serial.print("解析错误（Parsing Error）: ");
                    Serial.println(error.c_str());
                }
            }
        }
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
