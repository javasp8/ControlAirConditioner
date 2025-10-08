#ifndef ENVIRONMENT_SENSOR_H
#define ENVIRONMENT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

// センサーデータ構造体
struct SensorData {
  float temperature;
  float humidity;
  bool isValid;

  SensorData() : temperature(0.0f), humidity(0.0f), isValid(false) {}
  SensorData(float temp, float hum, bool valid)
    : temperature(temp), humidity(hum), isValid(valid) {}
};

// 環境センサークラス
class EnvironmentSensor {
public:
  EnvironmentSensor(uint8_t pin, uint8_t type, float tempOffset = 0.0f, float humOffset = 0.0f);

  // 初期化
  void begin();

  // センサーデータを読み取る
  SensorData read();

  // オフセットを設定
  void setTemperatureOffset(float offset) { temperatureOffset_ = offset; }
  void setHumidityOffset(float offset) { humidityOffset_ = offset; }

private:
  DHT dht_;
  float temperatureOffset_;
  float humidityOffset_;
};

#endif // ENVIRONMENT_SENSOR_H
