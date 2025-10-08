#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#define DHTPIN 32
#define DHTTYPE DHT22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define IR_RECV_PIN 18
#define IR_SEND_PIN 5

#define TEMP_OFFSET -1.5
#define HUM_OFFSET 0.0

// 温度制御の閾値
#define TEMP_HIGH 26.0
#define TEMP_LOW 25.5
#define HUM_HIGH 60.0
#define HUM_LOW 56.0

// 制御状態の管理
enum ACMode {
  MODE_NONE,
  MODE_COOLING_20,
  MODE_AUTO_PLUS1,
  MODE_DEHUMID_MINUS1_5
};

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IRrecv irrecv(IR_RECV_PIN);
IRsend irsend(IR_SEND_PIN);
decode_results results;

ACMode currentMode = MODE_NONE;
unsigned long lastControlTime = 0;
const unsigned long CONTROL_INTERVAL = 60000; // 1分ごとに制御判定

// 冷房20度のフレーム分割
uint16_t frame1[] = {
  270, 586, 282, 586, 284, 558, 308, 534, 336, 586, 282
};

uint16_t frame2[] = {
  1842, 308, 1442, 296, 588, 280, 588, 282, 584, 284, 
  1430, 308, 564, 306, 564, 304, 590, 280, 556, 312, 
  1460, 282, 590, 276, 1456, 284, 1428, 310, 560, 310, 
  1428, 310, 1454, 284, 1456, 282, 1460, 280, 1432, 308, 
  556, 310, 588, 282, 1460, 280, 582, 284, 588, 282, 
  558, 308, 566, 304, 584, 284, 564, 306, 584, 284, 
  558, 312, 558, 310, 564, 306, 580, 286, 1430, 312, 
  558, 308, 592, 280, 556, 312, 586, 284, 560, 308, 
  564, 308, 560, 308, 564, 306, 558, 308, 562, 310, 
  530, 338, 558, 312, 556, 310, 558, 312, 554
};

uint16_t frame3[] = {
  316, 530, 340, 526, 342, 552, 318, 550, 318, 554, 
  316, 1394, 344, 524, 344, 1394, 346, 550, 316, 528, 
  342, 526, 342, 526, 346, 526, 340, 526, 346, 522, 
  344, 526, 344, 526, 342, 526, 344, 524, 344, 530, 
  338, 524, 344, 524, 346, 522, 346, 524, 346, 522, 
  344, 524, 346, 520, 348, 526, 344, 524, 344, 524, 
  346, 522, 346, 526, 344, 524, 344, 524, 346, 522, 
  346, 524, 346, 522, 344, 524, 346, 522, 346, 526, 
  344, 522, 346, 1390, 348, 1372, 370, 520, 346, 524, 
  346, 1390, 348, 1392, 346, 1350, 388, 496, 374
};

uint16_t frame4[] = {
  530, 340, 554, 312, 558, 314, 552, 314, 556, 314, 
  552, 314, 534, 336, 552, 316, 554, 316, 580, 288, 
  554, 316, 550, 318, 554, 316, 1422, 316, 554, 316, 
  552, 316, 1424, 314, 1428, 310, 1430, 312, 550, 318, 
  554, 316, 1400, 338, 554, 314, 556, 314, 554, 314, 
  1426, 312, 1400, 340, 578, 288, 532, 338, 554, 314, 
  552, 318, 552, 316, 528, 342, 550, 318, 554, 316, 
  552, 316, 528, 342, 552, 316, 526, 344, 550, 318, 
  526, 344, 522, 344, 1400, 338, 528, 342, 1422, 316, 
  554, 316, 554, 314, 554, 316, 526, 342, 530
};

uint16_t frame5[] = {
  348, 522, 348, 494, 374, 496, 374, 516, 350, 520, 
  350, 518, 350, 522, 350, 518, 350, 494, 376, 492, 
  374, 476, 394, 1364, 374, 1386, 350, 496, 376, 518, 
  350, 520, 350, 1342, 396, 494, 374
};

// 自動+1度のフレーム分割（エアコン自動）
uint16_t auto_frame1[] = {
  494, 310, 554, 316, 554, 308, 560, 368, 502, 308, 560
};

uint16_t auto_frame2[] = {
  1596, 556, 1234, 504, 314, 552, 312, 558, 364, 552,
  1130, 560, 366, 506, 302, 562, 308, 562, 312, 556,
  1238, 504, 362, 504, 1174, 564, 1184, 554, 308, 564,
  1188, 548, 1234, 504, 1186, 552, 1178, 612, 1184, 502,
  306, 562, 366, 504, 1176, 562, 306, 562, 314, 558,
  304, 562, 368, 502, 312, 560, 310, 558, 310, 554,
  370, 502, 312, 556, 316, 556, 314, 552, 1238, 502,
  366, 502, 314, 556, 366, 502, 366, 504, 364, 504,
  312, 558, 314, 552, 312, 558, 366, 502, 316, 556,
  310, 554, 322, 550, 314, 554, 368, 500, 306
};

uint16_t auto_frame3[] = {
  316, 554, 320, 548, 316, 554, 316, 550, 318, 554,
  1182, 556, 370, 498, 1244, 498, 316, 552, 316, 552,
  320, 548, 316, 552, 314, 552, 316, 556, 322, 544,
  318, 550, 320, 548, 324, 546, 316, 550, 322, 548,
  314, 554, 318, 550, 314, 554, 352, 472, 360, 508,
  366, 550, 320, 502, 364, 506, 362, 506, 362, 508,
  364, 504, 364, 504, 364, 504, 364, 506, 362, 504,
  368, 504, 370, 498, 374, 498, 358, 506, 366, 508,
  368, 500, 368, 502, 1238, 500, 362, 506, 362, 508,
  1236, 502, 1236, 500, 1232, 506, 364, 506
};

uint16_t auto_frame4[] = {
  448, 420, 450, 418, 450, 426, 444, 424, 442, 422,
  448, 420, 448, 422, 450, 418, 450, 422, 448, 420,
  448, 452, 418, 426, 442, 1288, 450, 424, 448, 426,
  442, 1290, 452, 422, 446, 426, 444, 420, 446, 424,
  446, 420, 448, 1288, 450, 424, 448, 422, 446, 422,
  448, 448, 418, 1288, 452, 1288, 452, 424, 442, 424,
  446, 422, 446, 424, 448, 424, 444, 424, 446, 450,
  418, 1288, 450, 424, 446, 422, 446, 424, 446, 424,
  442, 424, 446, 1288, 450, 426, 444, 1290, 448, 426,
  442, 428, 442, 426, 442, 424, 446, 424, 444
};

uint16_t auto_frame5[] = {
  424, 446, 426, 444, 422, 446, 424, 446, 420, 446,
  424, 446, 418, 450, 426, 444, 420, 448, 1288, 450,
  1288, 450, 1290, 452, 420, 448, 422, 498, 1238, 506,
  364, 502, 366, 510
};

// 除湿-1.5のフレーム分割
uint16_t dehumid_frame1[] = {
  492, 366, 502, 318, 552, 366, 500, 312, 560, 366, 434
};

uint16_t dehumid_frame2[] = {
  1738, 432, 1248, 490, 434, 434, 436, 500, 366, 500, 
  1184, 488, 382, 554, 368, 502, 368, 500, 368, 434, 
  1258, 484, 430, 436, 1302, 500, 1182, 556, 320, 552, 
  1236, 502, 1184, 552, 1236, 502, 1190, 552, 1236, 436, 
  378, 552, 318, 488, 1302, 436, 432, 436, 390, 478, 
  388, 544, 318, 462, 408, 482, 436, 500, 322, 480, 
  436, 500, 316, 552, 312, 558, 366, 502, 1182, 556, 
  368, 434, 436, 500, 314, 464, 408, 486, 374, 492, 
  436, 500, 316, 486, 436, 434, 434, 434, 378, 494, 
  432, 432, 436, 436, 374, 560, 370, 500, 318
};

uint16_t dehumid_frame3[] = {
  316, 554, 310, 556, 320, 550, 366, 500, 320, 552, 
  314, 552, 1184, 556, 314, 556, 1188, 550, 318, 552, 
  366, 502, 318, 552, 310, 556, 370, 500, 366, 502, 
  370, 502, 312, 554, 316, 554, 310, 558, 316, 554, 
  366, 502, 314, 556, 314, 554, 318, 552, 310, 558, 
  368, 502, 308, 560, 314, 556, 312, 556, 320, 550, 
  312, 554, 314, 558, 312, 556, 316, 554, 310, 558, 
  318, 552, 314, 552, 368, 502, 308, 560, 318, 552, 
  316, 552, 322, 548, 366, 502, 316, 554, 1184, 554, 
  312, 558, 1188, 548, 1188, 550, 1182, 558, 322
};

uint16_t dehumid_frame4[] = {
  410, 1326, 412, 1272, 554, 1242, 500, 1238, 410, 456, 
  410, 460, 410, 1328, 410, 458, 408, 460, 410, 458, 
  410, 460, 410, 404, 462, 460, 410, 404, 464, 462, 
  408, 458, 410, 460, 410, 458, 410, 462, 408, 458, 
  410, 460, 410, 458, 408, 412, 460, 460, 406, 462, 
  410, 1328, 410, 460, 410, 458, 410, 1328, 410, 462, 
  408, 1330, 410, 4030, 318, 1338, 406, 1324, 408, 1328, 
  410, 462, 412, 1270, 464, 1334, 404, 458, 410, 462, 
  410, 458, 408, 404, 466, 460, 408, 462, 408, 486, 
  382, 1332, 408, 458, 410, 462, 408, 458, 408
};

uint16_t dehumid_frame5[] = {
  408, 460, 410, 460, 408, 462, 408, 460, 408, 464, 
  406, 458, 408, 410, 460, 400, 468, 464, 406, 402, 
  466, 408, 462, 486, 382, 402, 466, 1280, 460, 458, 
  410, 462, 408, 402, 466, 1330, 410, 1328, 410, 462, 408
};

void setup() {
  Serial.begin(115200);
  dht.begin();
  irrecv.enableIRIn();
  irsend.begin();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306の初期化に失敗しました");
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("DHT22");
  display.setCursor(10, 40);
  display.println("Sensor");
  display.display();
  delay(2000);
  
  Serial.println("赤外線受信待機中...");
}

// ダイキン冷房20度専用送信関数
void sendDaikinCooling20() {
  Serial.println("ダイキン 冷房20度 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  irrecv.disableIRIn();

  // フレーム1送信
  Serial.println("  フレーム1送信...");
  irsend.sendRaw(frame1, sizeof(frame1)/sizeof(frame1[0]), 38);
  delay(35); // フレーム間ギャップ

  // フレーム2送信
  Serial.println("  フレーム2送信...");
  irsend.sendRaw(frame2, sizeof(frame2)/sizeof(frame2[0]), 38);
  delay(35);

  // フレーム3送信
  Serial.println("  フレーム3送信...");
  irsend.sendRaw(frame3, sizeof(frame3)/sizeof(frame3[0]), 38);
  delay(35);

  // フレーム4送信
  Serial.println("  フレーム4送信...");
  irsend.sendRaw(frame4, sizeof(frame4)/sizeof(frame4[0]), 38);
  delay(35);

  // フレーム5送信
  Serial.println("  フレーム5送信...");
  irsend.sendRaw(frame5, sizeof(frame5)/sizeof(frame5[0]), 38);

  Serial.println("冷房20度 送信完了");

  // 受信を再度有効化
  delay(200);
  irrecv.enableIRIn();
}

// ダイキン自動+1度専用送信関数
void sendDaikinAutoPlus1() {
  Serial.println("ダイキン 自動+1度 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  irrecv.disableIRIn();

  // フレーム1送信
  Serial.println("  フレーム1送信...");
  irsend.sendRaw(auto_frame1, sizeof(auto_frame1)/sizeof(auto_frame1[0]), 38);
  delay(35);

  // フレーム2送信
  Serial.println("  フレーム2送信...");
  irsend.sendRaw(auto_frame2, sizeof(auto_frame2)/sizeof(auto_frame2[0]), 38);
  delay(35);

  // フレーム3送信
  Serial.println("  フレーム3送信...");
  irsend.sendRaw(auto_frame3, sizeof(auto_frame3)/sizeof(auto_frame3[0]), 38);
  delay(35);

  // フレーム4送信
  Serial.println("  フレーム4送信...");
  irsend.sendRaw(auto_frame4, sizeof(auto_frame4)/sizeof(auto_frame4[0]), 38);
  delay(35);

  // フレーム5送信
  Serial.println("  フレーム5送信...");
  irsend.sendRaw(auto_frame5, sizeof(auto_frame5)/sizeof(auto_frame5[0]), 38);

  Serial.println("自動+1度 送信完了");

  // 受信を再度有効化
  delay(200);
  irrecv.enableIRIn();
}

// ダイキン除湿-1.5専用送信関数
void sendDaikinDehumidMinus1_5() {
  Serial.println("ダイキン 除湿-1.5 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  irrecv.disableIRIn();

  // フレーム1送信
  Serial.println("  フレーム1送信...");
  irsend.sendRaw(dehumid_frame1, sizeof(dehumid_frame1)/sizeof(dehumid_frame1[0]), 38);
  delay(35);

  // フレーム2送信
  Serial.println("  フレーム2送信...");
  irsend.sendRaw(dehumid_frame2, sizeof(dehumid_frame2)/sizeof(dehumid_frame2[0]), 38);
  delay(35);

  // フレーム3送信
  Serial.println("  フレーム3送信...");
  irsend.sendRaw(dehumid_frame3, sizeof(dehumid_frame3)/sizeof(dehumid_frame3[0]), 38);
  delay(35);

  // フレーム4送信
  Serial.println("  フレーム4送信...");
  irsend.sendRaw(dehumid_frame4, sizeof(dehumid_frame4)/sizeof(dehumid_frame4[0]), 38);
  delay(35);

  // フレーム5送信
  Serial.println("  フレーム5送信...");
  irsend.sendRaw(dehumid_frame5, sizeof(dehumid_frame5)/sizeof(dehumid_frame5[0]), 38);

  Serial.println("除湿-1.5 送信完了");

  // 受信を再度有効化
  delay(200);
  irrecv.enableIRIn();
}

void controlAirConditioner(float temp, float hum) {
  unsigned long currentTime = millis();
  
  // 制御インターバルチェック（頻繁な制御を避ける）
  if (currentTime - lastControlTime < CONTROL_INTERVAL) {
    return;
  }
  
  ACMode newMode = MODE_NONE;
  
  // 温度優先の評価
  if (temp > TEMP_HIGH) {
    newMode = MODE_COOLING_20;
    Serial.println("制御: 温度が26度超 → 冷房20度");
  } 
  else if (temp < TEMP_LOW) {
    newMode = MODE_AUTO_PLUS1;
    Serial.println("制御: 温度が25.5度未満 → 自動+1度");
  }
  // 湿度の評価（温度条件に該当しない場合）
  else if (hum > HUM_HIGH) {
    newMode = MODE_DEHUMID_MINUS1_5;
    Serial.println("制御: 湿度が60%超 → 除湿-1.5");
  }
  else if (hum < HUM_LOW) {
    newMode = MODE_AUTO_PLUS1;
    Serial.println("制御: 湿度が56%未満 → 自動+1度");
  }
  // 通常（温度≦26度 かつ 湿度≦60%）
  else {
    newMode = MODE_AUTO_PLUS1;
    Serial.println("制御: 通常範囲 → 自動+1度");
  }
  
  // モードが変わった場合のみ送信
  if (newMode != currentMode) {
    switch (newMode) {
      case MODE_COOLING_20:
        sendDaikinCooling20(); // 専用関数を使用
        break;
      case MODE_AUTO_PLUS1:
        sendDaikinAutoPlus1(); // 専用関数を使用
        break;
      case MODE_DEHUMID_MINUS1_5:
        sendDaikinDehumidMinus1_5(); // 専用関数を使用
        break;
      default:
        break;
    }
    currentMode = newMode;
    lastControlTime = currentTime;
  }
}

void loop() {
  // 赤外線受信処理
  if (irrecv.decode(&results)) {
    Serial.println("====================================");
    Serial.print("受信したIRコード: ");
    serialPrintUint64(results.value, HEX);
    Serial.println();
    Serial.print("プロトコル: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("ビット数: ");
    Serial.println(results.bits);
    
    // Raw形式でも出力（配列定義用）
    Serial.print("uint16_t rawData[");
    Serial.print(results.rawlen - 1);
    Serial.println("] = {");
    Serial.print("  ");
    for (uint16_t i = 1; i < results.rawlen; i++) {
      Serial.print(results.rawbuf[i] * kRawTick);
      if (i < results.rawlen - 1) Serial.print(", ");
      if (i % 10 == 0) Serial.print("\n  ");
    }
    Serial.println("\n};");
    Serial.println("====================================");
    
    irrecv.resume();
  }
  
  // センサー読み取り（2秒ごと）
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead >= 2000) {
    lastSensorRead = millis();
    
    float humidity = dht.readHumidity() + HUM_OFFSET;
    float temperature = dht.readTemperature() + TEMP_OFFSET;
    
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("読み取りエラー");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(20, 25);
      display.println("ERROR!");
      display.display();
      return;
    }
    
    Serial.printf("温度: %.1f °C  湿度: %.1f %%\n", temperature, humidity);
    
    // ディスプレイ更新
    display.clearDisplay();
    
    // 温度表示
    display.setTextSize(1);
    display.setCursor(0, 5);
    display.println("Temperature");
    
    display.setTextSize(3);
    display.setCursor(10, 20);
    display.print(temperature, 1);
    display.setTextSize(2);
    display.setCursor(100, 25);
    display.println("C");
    
    // 区切り線
    display.drawLine(0, 45, 128, 45, SSD1306_WHITE);
    
    // 湿度表示
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println(" %");
    
    display.display();
    
    // エアコン制御ロジック実行
    controlAirConditioner(temperature, humidity);
  }
}
