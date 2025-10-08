#include <Arduino.h>
#include <Wire.h>
#include "AirConditionerController.h"
#include "EnvironmentSensor.h"
#include "DisplayController.h"

// ハードウェアピン設定
namespace HardwareConfig {
  constexpr uint8_t DHT_PIN = 32;
  constexpr uint8_t IR_RECV_PIN = 18;
  constexpr uint8_t IR_SEND_PIN = 5;
}

// センサーオフセット
namespace SensorConfig {
  constexpr float TEMP_OFFSET = -1.5f;
  constexpr float HUM_OFFSET = 0.0f;
}

// ディスプレイ設定
namespace DisplayConfig {
  constexpr uint8_t SCREEN_WIDTH = 128;
  constexpr uint8_t SCREEN_HEIGHT = 64;
  constexpr int8_t OLED_RESET = -1;
  constexpr uint8_t SCREEN_ADDRESS = 0x3C;
}

// タイミング設定
namespace TimingConfig {
  constexpr unsigned long SENSOR_READ_INTERVAL_MS = 2000;  // センサー読み取り間隔
  constexpr unsigned long CONTROL_INTERVAL_MS = 60000;     // エアコン制御間隔
  constexpr unsigned long STARTUP_DELAY_MS = 2000;         // 起動時の待機時間
}

// グローバルオブジェクト
AirConditionerController airConditioner(HardwareConfig::IR_SEND_PIN, HardwareConfig::IR_RECV_PIN);
EnvironmentSensor sensor(HardwareConfig::DHT_PIN, DHT22, SensorConfig::TEMP_OFFSET, SensorConfig::HUM_OFFSET);
DisplayController displayCtrl(DisplayConfig::SCREEN_WIDTH, DisplayConfig::SCREEN_HEIGHT,
                               &Wire, DisplayConfig::OLED_RESET, DisplayConfig::SCREEN_ADDRESS);

// タイミング管理
unsigned long lastSensorReadTime = 0;
unsigned long lastControlTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("エアコン自動制御システム起動");
  Serial.println("========================================");

  // センサー初期化
  sensor.begin();

  // ディスプレイ初期化
  if (!displayCtrl.begin()) {
    Serial.println("[System] ディスプレイ初期化失敗 - 継続");
  }
  displayCtrl.showStartupScreen();
  delay(TimingConfig::STARTUP_DELAY_MS);

  // エアコンコントローラー初期化
  airConditioner.begin();

  Serial.println("[System] システム起動完了");
  Serial.println("========================================\n");
}

void loop() {
  // 赤外線受信処理（常時監視）
  airConditioner.handleIRReceive();

  // センサー読み取りと制御処理
  unsigned long currentTime = millis();

  if (currentTime - lastSensorReadTime >= TimingConfig::SENSOR_READ_INTERVAL_MS) {
    lastSensorReadTime = currentTime;

    // センサーデータ読み取り
    SensorData sensorData = sensor.read();

    // ディスプレイ更新
    displayCtrl.showSensorData(sensorData);

    // センサーエラー時は制御スキップ
    if (!sensorData.isValid) {
      return;
    }

    // エアコン制御判定（制御間隔チェック）
    if (currentTime - lastControlTime >= TimingConfig::CONTROL_INTERVAL_MS) {
      lastControlTime = currentTime;

      // 最適なモードを決定
      ACMode optimalMode = airConditioner.determineOptimalMode(
        sensorData.temperature,
        sensorData.humidity
      );

      // モード設定（変更がある場合のみ送信）
      airConditioner.setMode(optimalMode);
    }
  }
}
