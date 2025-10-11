#include "WeatherForecast.h"

WeatherForecast::WeatherForecast(float latitude, float longitude)
  : lastUpdateTime_(0) {
  // API URLを構築
  apiUrl_ = "http://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 6) +
            "&longitude=" + String(longitude, 6) +
            "&daily=weather_code,temperature_2m_max,temperature_2m_min" +
            "&timezone=Asia/Tokyo&forecast_days=1";

  // 天気データを初期化
  weatherData_.isValid = false;
  weatherData_.tempMax = 0.0f;
  weatherData_.tempMin = 0.0f;
  weatherData_.weatherCode = 0;
  weatherData_.weatherString = "N/A";
  weatherData_.lastUpdate = 0;

  Serial.println("[Weather] WeatherForecast初期化完了");
  Serial.print("[Weather] API URL: ");
  Serial.println(apiUrl_);
}

bool WeatherForecast::begin() {
  Serial.println("[Weather] 初回天気予報データ取得開始");
  return fetchWeatherData();
}

void WeatherForecast::update() {
  unsigned long currentTime = millis();

  // 1時間経過チェック（またはオーバーフロー対策）
  if (currentTime - lastUpdateTime_ >= UPDATE_INTERVAL_MS ||
      currentTime < lastUpdateTime_) {
    Serial.println("[Weather] 定期更新: 天気予報データ取得開始");
    fetchWeatherData();
  }
}

WeatherData WeatherForecast::getData() const {
  return weatherData_;
}

bool WeatherForecast::fetchWeatherData() {
  HTTPClient http;

  Serial.print("[Weather] APIリクエスト送信: ");
  Serial.println(apiUrl_);

  http.begin(apiUrl_);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("[Weather] APIレスポンス受信成功");

    // JSONパース
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("[Weather] JSONパースエラー: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }

    // データ抽出
    JsonArray timeArray = doc["daily"]["time"];
    JsonArray weatherCodeArray = doc["daily"]["weather_code"];
    JsonArray tempMaxArray = doc["daily"]["temperature_2m_max"];
    JsonArray tempMinArray = doc["daily"]["temperature_2m_min"];

    if (timeArray.size() > 0 && weatherCodeArray.size() > 0 &&
        tempMaxArray.size() > 0 && tempMinArray.size() > 0) {

      weatherData_.weatherCode = weatherCodeArray[0];
      weatherData_.tempMax = tempMaxArray[0];
      weatherData_.tempMin = tempMinArray[0];
      weatherData_.weatherString = weatherCodeToString(weatherData_.weatherCode);
      weatherData_.isValid = true;
      weatherData_.lastUpdate = millis();
      lastUpdateTime_ = millis();

      Serial.println("[Weather] 天気予報データ更新完了");
      Serial.print("  - 最高気温: ");
      Serial.print(weatherData_.tempMax, 1);
      Serial.println(" °C");
      Serial.print("  - 最低気温: ");
      Serial.print(weatherData_.tempMin, 1);
      Serial.println(" °C");
      Serial.print("  - 天気コード: ");
      Serial.println(weatherData_.weatherCode);
      Serial.print("  - 天気: ");
      Serial.println(weatherData_.weatherString);

      http.end();
      return true;
    } else {
      Serial.println("[Weather] JSONデータが不完全です");
      http.end();
      return false;
    }
  } else {
    Serial.print("[Weather] HTTPエラー: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

String WeatherForecast::weatherCodeToString(int code) const {
  // 天気コードを文字列に変換（表示領域に合わせて省略形を使用）
  if (code == 0) {
    return "Clear";        // 快晴
  } else if (code >= 1 && code <= 3) {
    return "Cloudy";       // 晴れ〜曇り
  } else if (code == 45 || code == 48) {
    return "Fog";          // 霧
  } else if (code >= 51 && code <= 67) {
    return "Rain";         // 雨
  } else if (code >= 71 && code <= 77) {
    return "Snow";         // 雪
  } else if (code >= 80 && code <= 99) {
    return "Storm";        // 雷雨・シャワー
  } else {
    return "Unknown";      // 不明
  }
}
