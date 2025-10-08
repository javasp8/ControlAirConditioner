#include "AirConditionerController.h"
#include <IRutils.h>

// 温度・湿度の閾値
namespace Threshold {
  constexpr float TEMP_HIGH = 26.0f;
  constexpr float TEMP_LOW = 25.5f;
  constexpr float HUM_HIGH = 60.0f;
  constexpr float HUM_LOW = 56.0f;
}

AirConditionerController::AirConditionerController(uint8_t sendPin, uint8_t recvPin)
  : daikinAC_(sendPin), irRecv_(recvPin), currentMode_(ACMode::NONE) {
}

void AirConditionerController::begin() {
  daikinAC_.begin();
  irRecv_.enableIRIn();
  Serial.println("[AC] エアコンコントローラー初期化完了");
}

void AirConditionerController::setMode(ACMode mode) {
  if (mode == currentMode_) {
    Serial.println("[AC] モード変更なし");
    return;
  }

  switch (mode) {
    case ACMode::COOLING_20:
      sendCooling20();
      break;
    case ACMode::AUTO_PLUS_1:
      sendAutoPlus1();
      break;
    case ACMode::DEHUMID_MINUS_1_5:
      sendDehumidMinus1_5();
      break;
    default:
      Serial.println("[AC] 無効なモード");
      return;
  }

  currentMode_ = mode;
}

ACMode AirConditionerController::determineOptimalMode(float temperature, float humidity) {
  // 温度優先の評価
  if (temperature > Threshold::TEMP_HIGH) {
    Serial.printf("[AC] 温度が%.1f度超 → 冷房20度\n", Threshold::TEMP_HIGH);
    return ACMode::COOLING_20;
  }
  else if (temperature < Threshold::TEMP_LOW) {
    Serial.printf("[AC] 温度が%.1f度未満 → 自動+1度\n", Threshold::TEMP_LOW);
    return ACMode::AUTO_PLUS_1;
  }
  // 湿度の評価（温度条件に該当しない場合）
  else if (humidity > Threshold::HUM_HIGH) {
    Serial.printf("[AC] 湿度が%.1f%%超 → 除湿-1.5\n", Threshold::HUM_HIGH);
    return ACMode::DEHUMID_MINUS_1_5;
  }
  else if (humidity < Threshold::HUM_LOW) {
    Serial.printf("[AC] 湿度が%.1f%%未満 → 自動+1度\n", Threshold::HUM_LOW);
    return ACMode::AUTO_PLUS_1;
  }
  // 通常範囲
  else {
    Serial.println("[AC] 通常範囲 → 自動+1度");
    return ACMode::AUTO_PLUS_1;
  }
}

void AirConditionerController::handleIRReceive() {
  decode_results results;
  if (irRecv_.decode(&results)) {
    Serial.println("====================================");
    Serial.print("[IR] 受信コード: ");
    serialPrintUint64(results.value, HEX);
    Serial.println();
    Serial.print("[IR] プロトコル: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("[IR] ビット数: ");
    Serial.println(results.bits);

    // Raw形式でも出力
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

    irRecv_.resume();
  }
}

void AirConditionerController::sendCooling20() {
  Serial.println("[AC] 冷房20度 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinCool);        // 冷房モード
  daikinAC_.setTemp(20);                 // 温度20度
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.setSwingVertical(false);     // スイング無効
  daikinAC_.setSwingHorizontal(false);   // 水平スイング無効

  // IR信号送信
  daikinAC_.send();

  Serial.println("[AC] 冷房20度 送信完了");

  // 受信を再度有効化
  delay(200);
  irRecv_.enableIRIn();
}

void AirConditionerController::sendAutoPlus1() {
  Serial.println("[AC] 自動+1度 送信開始");

  // 受信を無効化
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinAuto);        // 自動モード
  daikinAC_.setTemp(27);                 // 温度27度（基準26度+1度）
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.setSwingVertical(false);     // スイング無効
  daikinAC_.setSwingHorizontal(false);   // 水平スイング無効

  // IR信号送信
  daikinAC_.send();

  Serial.println("[AC] 自動+1度 送信完了");

  // 受信を再度有効化
  delay(200);
  irRecv_.enableIRIn();
}

void AirConditionerController::sendDehumidMinus1_5() {
  Serial.println("[AC] 除湿-1.5 送信開始");

  // 受信を無効化
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinDry);         // 除湿モード
  daikinAC_.setTemp(24.5);               // 温度24.5度（基準26度-1.5度）
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.setSwingVertical(false);     // スイング無効
  daikinAC_.setSwingHorizontal(false);   // 水平スイング無効

  // IR信号送信
  daikinAC_.send();

  Serial.println("[AC] 除湿-1.5 送信完了");

  // 受信を再度有効化
  delay(200);
  irRecv_.enableIRIn();
}
