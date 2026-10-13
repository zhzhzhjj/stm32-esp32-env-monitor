#define MQTT_MAX_PACKET_SIZE 1024

#include <WiFi.h>
#include <PubSubClient.h>

// 配置区
const char* ssid = "Myphone";
const char* password = "zhzhzhjj";

const char* mqtt_server = "mqtt.heclouds.com";   // 改回域名
const int   mqtt_port     = 1883;
const char* product_id    = "eeo2979N2r";
const char* device_name   = "ESP_32";
const char* device_secret = "version=2018-10-31&res=products%2Feeo2979N2r%2Fdevices%2FESP_32&et=2104842622&method=md5&sign=T6BQq8EOBfmomMmbnKqG0w%3D%3D";

// 全局对象
WiFiClient espClient;
PubSubClient client(espClient);

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
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // OneNET 的 ClientID = 设备名称，Username = 产品ID，Password = 设备密钥
    if (client.connect(device_name, product_id, device_secret)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// 上报数据
void publishTelemetry(float temp, float humi, int lux, int eco2, int alarm) {
  String payload = String("{\"id\":\"") + String(millis()) +
                   "\",\"version\":\"1.0\",\"params\":{" +
                   "\"temp\":"  + String(temp, 1) +
                   ",\"humi\":" + String(humi, 1) +
                   ",\"lux\":"  + String(lux) +
                   ",\"eco2\":" + String(eco2) +
                   ",\"alarm\":"+ String(alarm) + "}}";

  String topic = String("$sys/") + product_id + "/" + device_name + "/thing/event/property/post";

  client.publish(topic.c_str(), payload.c_str());
  Serial.println("Sent: " + payload);
}

// Setup
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);   // 与 STM32 通信

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

// Loop
void loop() {
    // 每分钟打印一次 WiFi 信号强度
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 60000) {
    lastPrint = millis();
    Serial.print("WiFi RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 解析 STM32 发来的 9 字节数据帧
  if (Serial2.available() >= 9) {
    uint8_t buf[9];
    for (int i = 0; i < 9; i++) {
      buf[i] = Serial2.read();
    }

    if (buf[0] == 0xAA && buf[8] == 0x55) {
      float temp  = (float)buf[1] - 40.0f;   // 温度还原
      float humi  = buf[2];
      int   lux   = buf[3] * 10;             // 光照还原
      int   eco2  = (buf[4] << 8) | buf[5];
      int   alarm = buf[6];

      publishTelemetry(temp, humi, lux, eco2, alarm);
    }
  }
}
