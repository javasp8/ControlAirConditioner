#include "AirConditionerController.h"
#include "DaikinIRData.h"
#include <IRutils.h>

// 温度・湿度の閾値
namespace Threshold {
  constexpr float TEMP_HIGH = 26.0f;
  constexpr float TEMP_LOW = 25.5f;
  constexpr float HUM_HIGH = 60.0f;
  constexpr float HUM_LOW = 56.0f;
}

// IR送信設定
namespace IRConfig {
  constexpr uint16_t CARRIER_FREQUENCY_KHZ = 38;
  constexpr uint16_t FRAME_GAP_MS = 35;
  constexpr uint16_t POST_SEND_DELAY_MS = 200;
}

AirConditionerController::AirConditionerController(uint8_t sendPin, uint8_t recvPin)
  : irSend_(sendPin), irRecv_(recvPin), currentMode_(ACMode::NONE) {
}

void AirConditionerController::begin() {
  irRecv_.enableIRIn();
  irSend_.begin();
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

void AirConditionerController::sendFrames(
    const uint16_t* frame1, uint16_t len1,
    const uint16_t* frame2, uint16_t len2,
    const uint16_t* frame3, uint16_t len3,
    const uint16_t* frame4, uint16_t len4,
    const uint16_t* frame5, uint16_t len5,
    const char* modeName) {

  Serial.printf("[AC] %s 送信開始\n", modeName);

  // 受信を無効化（送信中の干渉を防ぐ）
  irRecv_.disableIRIn();

  // フレーム1送信
  Serial.println("  [1/5] フレーム1送信");
  irSend_.sendRaw(frame1, len1, IRConfig::CARRIER_FREQUENCY_KHZ);
  delay(IRConfig::FRAME_GAP_MS);

  // フレーム2送信
  Serial.println("  [2/5] フレーム2送信");
  irSend_.sendRaw(frame2, len2, IRConfig::CARRIER_FREQUENCY_KHZ);
  delay(IRConfig::FRAME_GAP_MS);

  // フレーム3送信
  Serial.println("  [3/5] フレーム3送信");
  irSend_.sendRaw(frame3, len3, IRConfig::CARRIER_FREQUENCY_KHZ);
  delay(IRConfig::FRAME_GAP_MS);

  // フレーム4送信
  Serial.println("  [4/5] フレーム4送信");
  irSend_.sendRaw(frame4, len4, IRConfig::CARRIER_FREQUENCY_KHZ);
  delay(IRConfig::FRAME_GAP_MS);

  // フレーム5送信
  Serial.println("  [5/5] フレーム5送信");
  irSend_.sendRaw(frame5, len5, IRConfig::CARRIER_FREQUENCY_KHZ);

  Serial.printf("[AC] %s 送信完了\n", modeName);

  // 受信を再度有効化
  delay(IRConfig::POST_SEND_DELAY_MS);
  irRecv_.enableIRIn();
}

void AirConditionerController::sendCooling20() {
  using namespace DaikinIR::Cooling20;
  sendFrames(frame1, frame1_length,
             frame2, frame2_length,
             frame3, frame3_length,
             frame4, frame4_length,
             frame5, frame5_length,
             "冷房20度");
}

void AirConditionerController::sendAutoPlus1() {
  using namespace DaikinIR::AutoPlus1;
  sendFrames(frame1, frame1_length,
             frame2, frame2_length,
             frame3, frame3_length,
             frame4, frame4_length,
             frame5, frame5_length,
             "自動+1度");
}

void AirConditionerController::sendDehumidMinus1_5() {
  using namespace DaikinIR::DehumidMinus1_5;
  sendFrames(frame1, frame1_length,
             frame2, frame2_length,
             frame3, frame3_length,
             frame4, frame4_length,
             frame5, frame5_length,
             "除湿-1.5");
}
