#include "DisplayController.h"

DisplayController::DisplayController(uint8_t width, uint8_t height, TwoWire* wire, int8_t resetPin, uint8_t address)
  : display_(width, height, wire, resetPin), width_(width), height_(height) {
}

bool DisplayController::begin() {
  if (!display_.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[Display] 初期化失敗");
    return false;
  }
  Serial.println("[Display] ディスプレイ初期化完了");
  return true;
}

void DisplayController::showStartupScreen() {
  display_.clearDisplay();
  display_.setTextSize(2);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(10, 20);
  display_.println("DHT22");
  display_.setCursor(10, 40);
  display_.println("Sensor");
  display_.display();
}

void DisplayController::showSensorData(const SensorData& data) {
  if (!data.isValid) {
    showError("Sensor Error");
    return;
  }

  display_.clearDisplay();

  // 温度表示
  display_.setTextSize(1);
  display_.setCursor(0, 5);
  display_.println("Temperature");

  display_.setTextSize(3);
  display_.setCursor(10, 20);
  display_.print(data.temperature, 1);
  display_.setTextSize(2);
  display_.setCursor(100, 25);
  display_.println("C");

  // 区切り線
  display_.drawLine(0, 45, width_, 45, SSD1306_WHITE);

  // 湿度表示
  display_.setTextSize(1);
  display_.setCursor(0, 50);
  display_.print("Humidity: ");
  display_.print(data.humidity, 1);
  display_.println(" %");

  display_.display();
}

void DisplayController::showError(const char* message) {
  display_.clearDisplay();
  display_.setTextSize(2);
  display_.setCursor(20, 25);
  display_.println(message);
  display_.display();
}
