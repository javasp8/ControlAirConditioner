/**
 * main.cpp
 *
 * エアコン自動制御システムのメイン制御ファイル
 * 各機能クラスを統合し、全体の動作を制御します。
 */

#include <Arduino.h>
#include <Wire.h>
#include "AirConditionerController.h"
#include "EnvironmentSensor.h"
#include "DisplayController.h"
#include "WiFiManager.h"
#include "TimeManager.h"
#include "AutoStopController.h"
#include "WeatherForecast.h"
#include "secrets.h"  // WiFi認証情報（Gitにコミットされない）

// ========================================
// 設定
// ========================================

// ハードウェアピン設定
namespace HardwareConfig {
  constexpr uint8_t DHT_PIN = 32;
  constexpr uint8_t IR_RECV_PIN = 18;
  constexpr uint8_t IR_SEND_PIN = 5;
}

// センサーオフセット
namespace SensorConfig {
  constexpr float TEMP_OFFSET = -2.0f;
  constexpr float HUM_OFFSET = 0.0f;
}

// WiFi設定
namespace WiFiConfig {
  constexpr unsigned long CONNECT_TIMEOUT_MS = 10000;  // 接続タイムアウト（10秒）
}

// 時刻設定
namespace TimeConfig {
  const char* NTP_SERVER = "ntp.nict.jp";  // 日本の公式NTPサーバー（NICT）
  const long GMT_OFFSET_SEC = 9 * 3600;     // 日本時間（JST）はUTC+9時間
  const int DAYLIGHT_OFFSET_SEC = 0;        // 日本にはサマータイム（夏時間）なし
  constexpr int AUTO_STOP_HOUR = 23;        // 自動停止する時刻（23時）
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
  constexpr unsigned long SENSOR_READ_INTERVAL_MS = 2000;   // センサー読み取り間隔
  constexpr unsigned long CONTROL_INTERVAL_MS = 60000;      // エアコン制御間隔
  constexpr unsigned long AUTO_STOP_CHECK_INTERVAL_MS = 60000;  // 自動停止チェック間隔
  constexpr unsigned long WEATHER_UPDATE_INTERVAL_MS = 3600000; // 天気予報更新間隔（1時間）
  constexpr unsigned long STARTUP_DELAY_MS = 2000;          // 起動時の待機時間
}

// 天気予報設定（東京の座標）
namespace WeatherConfig {
  constexpr float LATITUDE = 35.653204f;
  constexpr float LONGITUDE = 139.688272f;
}

// ========================================
// グローバルオブジェクト
// ========================================

// デバイス制御
AirConditionerController airConditioner(HardwareConfig::IR_SEND_PIN, HardwareConfig::IR_RECV_PIN);
EnvironmentSensor sensor(HardwareConfig::DHT_PIN, DHT22, SensorConfig::TEMP_OFFSET, SensorConfig::HUM_OFFSET);
DisplayController displayCtrl(DisplayConfig::SCREEN_WIDTH, DisplayConfig::SCREEN_HEIGHT,
                               &Wire, DisplayConfig::OLED_RESET, DisplayConfig::SCREEN_ADDRESS);

// 機能管理クラス
WiFiManager wifiMgr(WiFiSecrets::SSID, WiFiSecrets::PASSWORD, WiFiConfig::CONNECT_TIMEOUT_MS);
TimeManager timeMgr(TimeConfig::NTP_SERVER, TimeConfig::GMT_OFFSET_SEC, TimeConfig::DAYLIGHT_OFFSET_SEC);
AutoStopController autoStop(airConditioner, timeMgr, TimeConfig::AUTO_STOP_HOUR);
WeatherForecast weatherForecast(WeatherConfig::LATITUDE, WeatherConfig::LONGITUDE);

// タイミング管理
unsigned long lastSensorReadTime = 0;
unsigned long lastControlTime = 0;
unsigned long lastAutoStopCheckTime = 0;

// ========================================
// セットアップ
// ========================================

void setup() {
  // シリアル通信開始
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("エアコン自動制御システム起動");
  Serial.println("========================================");

  // WiFi接続
  if (wifiMgr.connect()) {
    Serial.println("[System] WiFi接続完了");
    // WiFi接続成功後、時刻を同期
    timeMgr.syncTime();
    // 天気予報を取得
    weatherForecast.begin();
  } else {
    Serial.println("[System] WiFi接続失敗 - WiFiなしで継続");
  }

  // センサー初期化
  sensor.begin();

  // ディスプレイ初期化
  if (!displayCtrl.begin()) {
    Serial.println("[System] ディスプレイ初期化失敗 - 継続");
  }
  displayCtrl.showStartupScreen();
  delay(TimingConfig::STARTUP_DELAY_MS);

  // 起動画面表示後、ディスプレイをクリア
  Serial.println("[System] スタートアップ完了、ディスプレイをクリア");

  // エアコンコントローラー初期化
  airConditioner.begin();

  Serial.println("[System] システム起動完了");
  Serial.println("========================================\n");
}

// ========================================
// メインループ
// ========================================

void loop() {
  // WiFi接続状態の監視（切断時は再接続を試みる）
  wifiMgr.checkConnection();

  // 赤外線受信処理（常時監視）
  airConditioner.handleIRReceive();

  // 天気予報の定期更新（1時間ごと）
  weatherForecast.update();

  // 現在時刻を取得
  unsigned long currentTime = millis();

  // 23時自動停止チェック（1分ごと）
  if (currentTime - lastAutoStopCheckTime >= TimingConfig::AUTO_STOP_CHECK_INTERVAL_MS) {
    lastAutoStopCheckTime = currentTime;
    autoStop.check();  // 7月〜9月以外の23時にエアコンを自動停止
  }

  // センサー読み取りと制御処理
  if (currentTime - lastSensorReadTime >= TimingConfig::SENSOR_READ_INTERVAL_MS) {
    lastSensorReadTime = currentTime;

    // センサーデータ読み取り
    SensorData sensorData = sensor.read();

    // 不快指数（DI）を計算
    if (sensorData.isValid) {
      sensorData.discomfortIndex = airConditioner.calculateDiscomfortIndex(
        sensorData.temperature,
        sensorData.humidity
      );
    }

    // ディスプレイ更新（天気予報付き）
    String formattedTime = timeMgr.getFormattedTime("%Y-%m-%d %H:%M");
    WeatherData weatherData = weatherForecast.getData();
    displayCtrl.showSensorDataWithWeather(sensorData, formattedTime, weatherData);

    // センサーエラー時は制御スキップ
    if (!sensorData.isValid) {
      return;
    }

    // エアコン制御判定（制御間隔チェック）
    if (currentTime - lastControlTime >= TimingConfig::CONTROL_INTERVAL_MS) {
      lastControlTime = currentTime;

      // 最適なモードを決定（DI値ベース）
      ACMode optimalMode = airConditioner.determineOptimalMode(
        sensorData.temperature,
        sensorData.humidity
      );

      // モード設定（変更がある場合のみ送信）
      // airConditioner.setMode(optimalMode);  // ← 必要に応じてコメント解除
    }
  }
}
