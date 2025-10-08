#ifndef AIR_CONDITIONER_CONTROLLER_H
#define AIR_CONDITIONER_CONTROLLER_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <ir_Daikin.h>

// エアコンの動作モード
enum class ACMode {
  NONE,
  COOLING_20,        // 冷房20度
  AUTO_PLUS_1,       // 自動+1度
  DEHUMID_MINUS_1_5  // 除湿-1.5
};

// エアコン制御クラス
class AirConditionerController {
public:
  AirConditionerController(uint8_t sendPin, uint8_t recvPin);

  // 初期化
  void begin();

  // 指定されたモードでエアコンを制御
  void setMode(ACMode mode);

  // 現在のモードを取得
  ACMode getCurrentMode() const { return currentMode_; }

  // 温度と湿度に基づいて最適なモードを決定
  ACMode determineOptimalMode(float temperature, float humidity);

  // 赤外線信号の受信処理
  void handleIRReceive();

private:
  IRDaikinESP daikinAC_;
  IRrecv irRecv_;
  ACMode currentMode_;

  // 各モードの送信関数
  void sendCooling20();
  void sendAutoPlus1();
  void sendDehumidMinus1_5();
};

#endif // AIR_CONDITIONER_CONTROLLER_H
