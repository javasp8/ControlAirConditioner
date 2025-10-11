#ifndef WEATHER_FORECAST_H
#define WEATHER_FORECAST_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 天気予報データ構造体
struct WeatherData {
  bool isValid;              // データの有効性
  float tempMax;             // 最高気温 (°C)
  float tempMin;             // 最低気温 (°C)
  int weatherCode;           // 天気コード
  String weatherString;      // 天気の文字列表現
  unsigned long lastUpdate;  // 最終更新時刻 (millis)
};

// 天気予報管理クラス
class WeatherForecast {
public:
  // コンストラクタ
  WeatherForecast(float latitude, float longitude);

  // 初期化（起動時の天気予報取得）
  bool begin();

  // 定期更新チェック（1時間ごとに更新）
  void update();

  // 最新の天気予報データを取得
  WeatherData getData() const;

private:
  // API設定
  String apiUrl_;

  // 更新管理
  static constexpr unsigned long UPDATE_INTERVAL_MS = 3600000;  // 1時間 = 3600秒 = 3600000ミリ秒
  unsigned long lastUpdateTime_;

  // 天気データ
  WeatherData weatherData_;

  // 内部処理関数
  bool fetchWeatherData();
  String weatherCodeToString(int code) const;
};

#endif // WEATHER_FORECAST_H
