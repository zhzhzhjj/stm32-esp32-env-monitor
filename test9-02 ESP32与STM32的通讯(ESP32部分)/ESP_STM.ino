void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // 显式指定 RX=16, TX=17
  Serial.println("ESP32 is ready!");
}

void loop()
{
  // 只要缓冲区里够 8 个字节，就尝试解析一帧
  if (Serial2.available() >= 9) {
    uint8_t buf[9];
    for (int i = 0; i < 9; i++) {
      buf[i] = Serial2.read();
    }
    
    // 校验帧头和帧尾
    if (buf[0] == 0xAA && buf[8] == 0x55) {
      // 提取数据
      float temp = buf[1] - 40.0f;                            // 温度直接取整数
      float humi = buf[2];                                 // 湿度
      uint16_t lux = buf[3] * 10;                          // 光照乘以10还原
      uint16_t eco2 = (buf[4] << 8) | buf[5];              // eCO2高低字节拼接
      uint8_t alarm = buf[6];
      
      // 组装 JSON
      String jsonStr = "{\"temp\":" + String(temp, 1) + 
                       ",\"humi\":" + String(humi, 1) + 
                       ",\"lux\":" + String(lux) + 
                       ",\"eco2\":" + String(eco2) + 
                       ",\"alarm\":" + String(alarm) + "}";
      
      // 打印到电脑串口
      Serial.println("Sent: " + jsonStr);

    }
  }
}
