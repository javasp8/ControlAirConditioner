# エアコン自動制御システム

ESP32を使用したダイキンエアコンの自動制御システムです。温度・湿度センサーから不快指数（DI）を計算し、最適なモードでエアコンを自動制御します。

## 主な機能

- 📊 **不快指数（DI）ベースの自動制御**: 温度と湿度から快適度を判断
- 🌡️ **DHT22センサー**: 温度・湿度の高精度測定
- 📺 **OLEDディスプレイ**: リアルタイムでセンサー情報と天気予報を表示
- 🌐 **WiFi対応**: NTP時刻同期、天気予報API連携
- ☀️ **天気予報表示**: Open-Meteo APIから最高・最低気温と天気を取得・表示
- ⏰ **23時自動停止**: 7月〜9月以外は23時に自動停止（節電）
- 🎛️ **3つの運転モード**:
  - 冷房20度（DI 77以上）
  - 除湿-1.5度（DI 75〜77）
  - 自動+1度（DI 70〜75、68未満）

## ハードウェア構成

### 必要な部品

| 部品 | 型番・仕様 | 用途 |
|------|-----------|------|
| マイコン | ESP32-DevKitC | メイン制御 |
| 温湿度センサー | 温湿度センサー モジュール AM2302 | 環境測定 |
| 赤外線LED | 5mm赤外線LED 940nm OSI5LA5113A | エアコン制御 |
| 赤外線受信モジュール | 赤外線リモコン受信モジュールOSRB38C9AA | リモコン学習 |
| OLEDディスプレイ | 0.96インチ 128×64ドット有機ELディスプレイ(OLED) | 情報表示 |

### ピン接続

```
ESP32          デバイス
GPIO32    →    DHT22 (Data)
GPIO5     →    IR LED (送信)
GPIO18    →    IR受信モジュール
GPIO21    →    OLED SDA
GPIO22    →    OLED SCL
3.3V      →    センサー/ディスプレイ電源
GND       →    共通GND
```

## ソフトウェア構成

### プロジェクト構造

```
ControliAirConditioner/
├── include/
│   ├── AirConditionerController.h  # エアコン制御
│   ├── EnvironmentSensor.h         # 温湿度センサー
│   ├── DisplayController.h         # ディスプレイ制御
│   ├── WiFiManager.h               # WiFi接続管理
│   ├── TimeManager.h               # 時刻管理
│   ├── AutoStopController.h        # 自動停止制御
│   ├── WeatherForecast.h           # 天気予報取得
│   ├── secrets.h.example           # 認証情報テンプレート
│   └── secrets.h                   # WiFi認証情報（.gitignore）
├── src/
│   ├── main.cpp                    # メイン制御
│   ├── AirConditionerController.cpp
│   ├── EnvironmentSensor.cpp
│   ├── DisplayController.cpp
│   ├── WiFiManager.cpp
│   ├── TimeManager.cpp
│   ├── AutoStopController.cpp
│   └── WeatherForecast.cpp
└── platformio.ini                  # ビルド設定
```

### クラス設計

#### 🎛️ AirConditionerController
エアコンの赤外線制御を担当
- ダイキンエアコンのIR信号送信
- 不快指数（DI）計算
- 最適モード判定

#### 🌡️ EnvironmentSensor
温湿度センサーの読み取り
- DHT22センサー制御
- オフセット補正機能
- エラーハンドリング

#### 📺 DisplayController
OLEDディスプレイの制御
- センサーデータ表示
- 天気予報データ表示
- 起動画面表示
- リアルタイム更新

#### 🌐 WiFiManager
WiFi接続の管理
- 自動接続・再接続
- 接続状態監視
- タイムアウト処理

#### ⏰ TimeManager
時刻管理とNTP同期
- NTPサーバーからの時刻取得
- 日本時間（JST）への変換
- 夏季（7〜9月）判定

#### 🛑 AutoStopController
エアコン自動停止機能
- 23時の自動停止
- 夏季（7〜9月）のスキップ
- 1日1回のみ実行

#### ☀️ WeatherForecast
天気予報の取得と管理
- Open-Meteo API連携
- 起動時および1時間ごとに天気予報を自動取得
- 最高・最低気温、天気コードを取得
- 天気コードを読みやすい文字列に変換（Clear, Cloudy, Fog, Rain, Snow, Storm）

## セットアップ

### 1. 環境構築

```bash
# PlatformIOのインストール（VS Code拡張機能として推奨）
# または
pip install platformio

# プロジェクトのクローン
git clone <repository-url>
cd ControliAirConditioner
```

### 2. WiFi認証情報の設定

```bash
# テンプレートをコピー
cp include/secrets.h.example include/secrets.h

# secrets.h を編集して実際のWiFi情報を入力
# const char* SSID = "YOUR_WIFI_SSID";
# const char* PASSWORD = "YOUR_WIFI_PASSWORD";
```

⚠️ **重要**: `secrets.h` は `.gitignore` に含まれており、Gitにコミットされません。

### 3. ビルド＆アップロード

```bash
# ビルド
pio run

# ESP32にアップロード
pio run -t upload

# シリアルモニタで動作確認
pio device monitor
```

## 設定のカスタマイズ

`src/main.cpp` の各 namespace で設定を変更できます：

### ハードウェアピン設定
```cpp
namespace HardwareConfig {
  constexpr uint8_t DHT_PIN = 32;        // DHT22ピン
  constexpr uint8_t IR_RECV_PIN = 18;    // IR受信ピン
  constexpr uint8_t IR_SEND_PIN = 5;     // IR送信ピン
}
```

### センサー補正
```cpp
namespace SensorConfig {
  constexpr float TEMP_OFFSET = -2.0f;   // 温度補正（℃）
  constexpr float HUM_OFFSET = 0.0f;     // 湿度補正（%）
}
```

### 時刻設定
```cpp
namespace TimeConfig {
  const char* NTP_SERVER = "ntp.nict.jp";
  constexpr int AUTO_STOP_HOUR = 23;     // 自動停止時刻
}
```

### タイミング設定
```cpp
namespace TimingConfig {
  constexpr unsigned long SENSOR_READ_INTERVAL_MS = 2000;   // センサー読取間隔
  constexpr unsigned long CONTROL_INTERVAL_MS = 60000;      // エアコン制御間隔
  constexpr unsigned long AUTO_STOP_CHECK_INTERVAL_MS = 60000;  // 停止チェック間隔
  constexpr unsigned long WEATHER_UPDATE_INTERVAL_MS = 3600000; // 天気予報更新間隔（1時間）
}
```

### 天気予報設定
```cpp
namespace WeatherConfig {
  constexpr float LATITUDE = 35.653204f;   // 緯度（デフォルト：東京）
  constexpr float LONGITUDE = 139.688272f; // 経度（デフォルト：東京）
}
```

## 不快指数（DI）について

温度（T）と湿度（H）から計算される快適度の指標：

```
DI = 0.81T + 0.01H(0.99T - 14.3) + 46.3
```

### DI値の目安
| DI値 | 体感 | システムの動作 |
|------|------|---------------|
| 〜55 | 寒い | - |
| 55〜60 | 肌寒い | - |
| 60〜65 | 何も感じない | - |
| 65〜70 | 快い | - |
| 70〜75 | 暑くない | 自動+1度 |
| 75〜77 | やや暑い | 除湿-1.5度 |
| 77〜 | 暑くて不快 | 冷房20度 |

## 動作ログ例

```
========================================
エアコン自動制御システム起動
========================================

[WiFi] WiFi接続を開始します...
[WiFi] SSID: YourWiFi
.....
[WiFi] WiFi接続成功！
[WiFi] IPアドレス: 192.168.1.100
[WiFi] 電波強度 (RSSI): -45 dBm
[System] WiFi接続完了

[Time] NTP時刻同期を開始...
[Time] 時刻同期成功
[Time] 現在時刻: 2025/10/11 22:30:15

[Weather] WeatherForecast初期化完了
[Weather] API URL: http://api.open-meteo.com/v1/forecast?...
[Weather] 初回天気予報データ取得開始
[Weather] APIリクエスト送信: http://api.open-meteo.com/v1/forecast?...
[Weather] APIレスポンス受信成功
[Weather] 天気予報データ更新完了
[Weather]   - 最高気温: 18.5 °C
[Weather]   - 最低気温: 15.4 °C
[Weather]   - 天気コード: 55
[Weather]   - 天気: Rain

[Sensor] 環境センサー初期化完了
[Display] ディスプレイ初期化成功
[AC] エアコンコントローラー初期化完了
[System] システム起動完了
========================================

[Sensor] 温度: 26.5°C, 湿度: 65.0%
[AC] 温度:26.5℃, 湿度:65.0%, DI:74.2
[AC] DI 74.2 (快適範囲) → 自動+1度

[AutoStop] 現在時刻: 23時, 月: 10月
[AutoStop] ========================================
[AutoStop] 23時になりました。エアコンを自動停止します（10月は対象期間）
[AutoStop] ========================================
[AC] エアコン停止 送信開始
[AC] エアコン停止 送信完了
```

## 使用ライブラリ

| ライブラリ | バージョン | 用途 |
|-----------|-----------|------|
| IRremoteESP8266 | ^2.8.6 | 赤外線送受信 |
| DHT sensor library | ^1.4.4 | DHT22制御 |
| Adafruit SSD1306 | ^2.5.7 | OLEDディスプレイ |
| Adafruit GFX Library | ^1.11.3 | グラフィック描画 |
| Adafruit Unified Sensor | ^1.1.14 | センサー統合 |
| ArduinoJson | ^7.2.1 | JSON解析（天気予報API用） |

## トラブルシューティング

### WiFiに接続できない
- `secrets.h` のSSID/パスワードを確認
- WiFiルーターの2.4GHz帯を使用しているか確認
- シリアルモニタで接続ログを確認

### センサーが読み取れない
- DHT22のピン接続を確認（VCC, GND, Data）
- データピンのプルアップ抵抗（10kΩ）を確認
- センサーの電源電圧（3.3V）を確認

### エアコンが反応しない
- 赤外線LEDの向きを確認
- IR LEDの電流制限抵抗を確認
- リモコンコードがダイキン製に対応しているか確認

### 時刻が同期されない
- WiFi接続が成功しているか確認
- NTPサーバー（ntp.nict.jp）にアクセスできるか確認
- ファイアウォール設定を確認

### 天気予報が取得できない
- WiFi接続が成功しているか確認
- Open-Meteo API（api.open-meteo.com）にアクセスできるか確認
- シリアルモニタで天気予報のログを確認
- 緯度・経度の設定を確認（`WeatherConfig`）

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。

## 参考資料

- [IRremoteESP8266 Documentation](https://github.com/crankyoldgit/IRremoteESP8266)
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [Open-Meteo Weather API](https://open-meteo.com/)
- [PlatformIO Documentation](https://docs.platformio.org/)
