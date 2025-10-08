#include "EnvironmentSensor.h"

EnvironmentSensor::EnvironmentSensor(uint8_t pin, uint8_t type, float tempOffset, float humOffset)
  : dht_(pin, type), temperatureOffset_(tempOffset), humidityOffset_(humOffset) {
}

void EnvironmentSensor::begin() {
  dht_.begin();
  Serial.println("[Sensor] 環境センサー初期化完了");
}

SensorData EnvironmentSensor::read() {
  float humidity = dht_.readHumidity();
  float temperature = dht_.readTemperature();

  // 読み取りエラーチェック
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("[Sensor] 読み取りエラー");
    return SensorData(0.0f, 0.0f, false);
  }

  // オフセット適用
  temperature += temperatureOffset_;
  humidity += humidityOffset_;

  Serial.printf("[Sensor] 温度: %.1f°C, 湿度: %.1f%%\n", temperature, humidity);

  return SensorData(temperature, humidity, true);
}
