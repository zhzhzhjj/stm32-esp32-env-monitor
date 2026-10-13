#define MQTT_MAX_PACKET_SIZE 1024

#include <WiFi.h>
#include <PubSubClient.h>

// 配置区
const char* ssid = "TP-LINK_EE82_507";
const char* password = "1234567890";

const char* mqtt_server = "218.201.45.2";   // OneNET 服务器 IP
const int   mqtt_port     = 1883;
const char* product_id    = "eeo2979N2r";
const char* device_name   = "ESP32";        // 确保控制台设备名也为 ESP32
const char* device_secret = "version=2018-10-31&res=products%2Feeo2979N2r%2Fdevices%2FESP32&et=2104842622&method=md5&sign=9hXe7QRI0vzBgJxZvsdh2A%3D%3D";

// 全局对象
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT 回调函数，用于接收平台下发的数据（包含错误回复）
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println("Received: " + msg);

  // 1. 提取云端下发的 "id"（回复时必须带上，否则云端不认）
  String requestId = "";
  int idPos = msg.indexOf("\"id\"");
  if (idPos > 0) {
    int idColon = msg.indexOf(':', idPos);
    int idStart = msg.indexOf('"', idColon);
    int idEnd = msg.indexOf('"', idStart + 1);
    if (idStart > 0 && idEnd > 0) {
      requestId = msg.substring(idStart + 1, idEnd);
    }
  }
  if (requestId == "") requestId = "123"; // 兜底

  // 2. 提取 cmd 值（兼容 "cmd":"F" 和 "cmd":{"value":"F"} 两种格式）
  int cmdPos = msg.indexOf("\"cmd\"");
  if (cmdPos < 0) return;
  
  String cmd = "";
  int valPos = msg.indexOf("\"value\"", cmdPos);
  
  // 如果 cmd 后面跟着 "value"（嵌套格式）
  if (valPos > 0 && valPos < msg.indexOf('}', cmdPos)) {
    int vColon = msg.indexOf(':', valPos);
    int vStart = msg.indexOf('"', vColon);
    int vEnd = msg.indexOf('"', vStart + 1);
    if (vStart > 0 && vEnd > 0) cmd = msg.substring(vStart + 1, vEnd);
  } else {
    // 否则直接取 cmd 后面的冒号并提取字符串（扁平格式）
    int colonPos = msg.indexOf(':', cmdPos);
    int cStart = msg.indexOf('"', colonPos);
    int cEnd = msg.indexOf('"', cStart + 1);
    if (cStart > 0 && cEnd > 0) cmd = msg.substring(cStart + 1, cEnd);
  }
  
  Serial.println("Cmd extracted: " + cmd);

  // 3. 转发给 STM32
  if (cmd == "F") {
    Serial2.print('F');
    Serial.println("Forwarded F to STM32");
  } else if (cmd == "S") {
    Serial2.print('S');
    Serial.println("Forwarded S to STM32");
  }

  // 4. 回复云端确认收到属性设置（带上正确的 id）
  String replyTopic = String("$sys/") + product_id + "/" + device_name + "/thing/property/set_reply";
  String replyPayload = "{\"id\":\"" + requestId + "\",\"code\":200,\"msg\":\"success\"}";
  if (client.publish(replyTopic.c_str(), replyPayload.c_str())) {
    Serial.println("Replied to set_reply: " + replyPayload);
  } else {
    Serial.println("Reply FAILED!");
  }
}

// 连接 WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// 重连 MQTT
void reconnect() {
  Serial.print("Attempting MQTT connection...");
  
  if (client.connect(device_name, product_id, device_secret)) {
    Serial.println("connected");
    
    // 订阅上报回复主题
    String replyTopic = String("$sys/") + product_id + "/" + device_name + "/thing/property/post/reply";
    client.subscribe(replyTopic.c_str());
    
    // 订阅属性设置下行主题（用于接收云端指令）
    String setTopic = String("$sys/") + product_id + "/" + device_name + "/thing/property/set";
    client.subscribe(setTopic.c_str());
    Serial.println("Subscribed to: " + setTopic);
    
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    espClient.stop();
    delay(1000);
  }
}


// 上报数据
void publishTelemetry(float temp, float humi, int lux, int eco2, int alarm) {
  if (!client.connected()) {
    return;
  }
  
  // 构建 JSON 数据
  String payload = String("{\"id\":\"") + String(millis()) +
                 "\",\"version\":\"1.0\",\"params\":{" +
                 "\"temp\":{\"value\":"  + String(temp, 1) + "}," +
                 "\"humi\":{\"value\":" + String(humi, 1) + "}," +
                 "\"lux\":{\"value\":"  + String(lux) + "}," +
                 "\"eco2\":{\"value\":" + String(eco2) + "}," +
                 "\"alarm\":{\"value\":"+ String(alarm) + "}}}";

  // OneNET Studio 新版标准属性上报 Topic（去掉了 event）
  String topic = String("$sys/") + product_id + "/" + device_name + "/thing/property/post";

  // 发布数据
  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Sent: " + payload);
  } else {
    Serial.println("Publish FAILED!");
  }
}

// Setup
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(1024);
  client.setKeepAlive(60);
  client.setCallback(callback);   // 确保这行在
}

// Loop
void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 60000) {
    lastPrint = millis();
    Serial.print("WiFi RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  
  static unsigned long lastReconnect = 0;
  // 确保 client.loop() 持续被调用，处理心跳和接收消息
  if (!client.connected()) {
    if (millis() - lastReconnect > 6000) {
      lastReconnect = millis();
      reconnect();
    }
  } else {
    client.loop();
  }
  
  // 读取串口数据
  if (Serial2.available() >= 9) {
    uint8_t buf[9];
    for (int i = 0; i < 9; i++) {
      buf[i] = Serial2.read();
    }
    
    if (buf[0] == 0xAA && buf[8] == 0x55) {
      // 每 2 秒才上报一次（满足 OneNET 1次/秒的限制）
      static unsigned long lastPublish = 0;
      if (millis() - lastPublish > 2000) {
        lastPublish = millis();
        
        float temp  = (float)buf[1] - 40.0f;
        float humi  = buf[2];
        int   lux   = buf[3] * 10;
        int   eco2  = (buf[4] << 8) | buf[5];
        int   alarm = buf[6];
        
        publishTelemetry(temp, humi, lux, eco2, alarm);
      }
    }
  }
}
