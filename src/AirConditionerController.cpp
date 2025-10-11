/**
 * AirConditionerController.cpp
 *
 * ダイキンエアコンを赤外線で制御するクラスの実装ファイル
 *
 * 主な機能:
 * - 温度・湿度から不快指数（DI）を計算
 * - DI値に基づいて最適なエアコンモードを自動選択
 * - 赤外線信号の送受信（ダイキンエアコン用）
 *
 * 対応モード:
 * - COOLING_20: 冷房20度（DI 77以上の暑い時）
 * - AUTO_PLUS_1: 自動+1度（快適範囲または肌寒い時）
 * - DEHUMID_MINUS_1_5: 除湿-1.5度（やや暑い時）
 */

#include "AirConditionerController.h"
#include <IRutils.h>  // 赤外線ユーティリティ関数

/**
 * 不快指数（DI）の閾値設定
 * ユーザー希望: DI値 70～75 を保つ（寒がり向け設定）
 *
 * namespaceは名前の衝突を避けるための仕組み
 * DIThreshold::TARGET_MINのように使う
 */
namespace DIThreshold {
  constexpr float TARGET_MIN = 70.0f;         // 目標DI最小値（これより下がったら暖める）
  constexpr float TARGET_MAX = 75.0f;         // 目標DI最大値（これより上がったら対策）
  constexpr float COOLING_THRESHOLD = 77.0f;  // 冷房開始閾値（強力な冷房が必要）
  constexpr float HEATING_THRESHOLD = 68.0f;  // 暖房開始閾値（肌寒い）
  // constexprは「コンパイル時定数」を意味するキーワード
  // この値は実行中に変わらないので、メモリ効率が良い
}

/**
 * コンストラクタ（オブジェクトを作成する時に呼ばれる特別な関数）
 * @param sendPin  赤外線送信用のピン番号
 * @param recvPin  赤外線受信用のピン番号
 *
 * : daikinAC_(sendPin) の部分は「初期化リスト」と呼ばれ、
 * メンバ変数を効率的に初期化するC++の記法です
 */
AirConditionerController::AirConditionerController(uint8_t sendPin, uint8_t recvPin)
  : daikinAC_(sendPin), irRecv_(recvPin), currentMode_(ACMode::NONE) {
  // コンストラクタの本体（今回は初期化リストで全て完了しているので空）
}

/**
 * 初期化処理（setup関数から呼ばれる）
 * エアコンの赤外線送信機能と受信機能を起動する
 */
void AirConditionerController::begin() {
  daikinAC_.begin();      // ダイキンエアコン送信ライブラリの初期化
  irRecv_.enableIRIn();   // 赤外線受信機能を有効化
  Serial.println("[AC] エアコンコントローラー初期化完了");
}

/**
 * エアコンの動作モードを設定する
 * @param mode 設定したいモード（COOLING_20、AUTO_PLUS_1、DEHUMID_MINUS_1_5のいずれか）
 *
 * 現在のモードと同じ場合は、無駄な信号送信を避けるため何もしない
 */
void AirConditionerController::setMode(ACMode mode) {
  // 既に同じモードの場合は処理をスキップ
  if (mode == currentMode_) {
    Serial.println("[AC] モード変更なし");
    return;  // ここで関数を終了
  }

  // switch文：modeの値に応じて異なる処理を実行
  switch (mode) {
    case ACMode::OFF:                // エアコン停止の場合
      sendOff();
      break;  // switch文を抜ける
    case ACMode::COOLING_20:         // 冷房20度モードの場合
      sendCooling20();
      break;  // switch文を抜ける
    case ACMode::AUTO_PLUS_1:        // 自動+1度モードの場合
      sendAutoPlus1();
      break;
    case ACMode::DEHUMID_MINUS_1_5:  // 除湿-1.5度モードの場合
      sendDehumidMinus1_5();
      break;
    default:  // 上記以外（想定外のモード）の場合
      Serial.println("[AC] 無効なモード");
      return;
  }

  // モード変更が成功したら、現在のモードを更新
  currentMode_ = mode;
}

/**
 * 不快指数（Discomfort Index: DI）を計算
 * @param temperature 温度（摂氏）
 * @param humidity    湿度（%）
 * @return 不快指数（DI値）
 *
 * 計算式: DI = 0.81T + 0.01H(0.99T - 14.3) + 46.3
 * DI値の目安:
 *   〜55: 寒い
 *   55〜60: 肌寒い
 *   60〜65: 何も感じない
 *   65〜70: 快い
 *   70〜75: 暑くない
 *   75〜80: やや暑い
 *   80〜85: 暑くて汗が出る
 *   85〜  : 暑くてたまらない
 */
float AirConditionerController::calculateDiscomfortIndex(float temperature, float humidity) {
  // fは「float型の数値」を表す接尾辞（これがないとdouble型になる）
  float di = 0.81f * temperature + 0.01f * humidity * (0.99f * temperature - 14.3f) + 46.3f;
  return di;  // 計算結果を呼び出し元に返す
}

/**
 * 温度と湿度から最適なエアコンモードを決定する
 * @param temperature 現在の温度（摂氏）
 * @param humidity    現在の湿度（%）
 * @return 最適なエアコンモード
 *
 * 不快指数（DI）を計算し、その値に応じて最適なモードを選択します。
 * 目標: DI 70〜75を維持（寒がり向けの設定）
 */
ACMode AirConditionerController::determineOptimalMode(float temperature, float humidity) {
  // 不快指数（DI）を計算
  float di = calculateDiscomfortIndex(temperature, humidity);

  // 現在の状態をシリアルモニタに出力（%.1fは小数点以下1桁まで表示）
  Serial.printf("[AC] 温度:%.1f℃, 湿度:%.1f%%, DI:%.1f\n", temperature, humidity, di);

  // DI値に基づいてモードを決定
  // 目標: DI 70～75を維持（寒がり向け設定）

  // if-else if-else構文：上から順に条件を評価し、最初に真になった処理を実行
  if (di >= DIThreshold::COOLING_THRESHOLD) {
    // DI 77以上: 暑くて不快 → 冷房20度で強力に冷却
    Serial.printf("[AC] DI %.1f (暑い) → 冷房20度\n", di);
    return ACMode::COOLING_20;  // ここで関数終了、値を返す
  }
  else if (di > DIThreshold::TARGET_MAX) {
    // DI 75～77: やや暑い → 除湿で快適化
    Serial.printf("[AC] DI %.1f (やや暑い) → 除湿-1.5\n", di);
    return ACMode::DEHUMID_MINUS_1_5;
  }
  else if (di >= DIThreshold::TARGET_MIN && di <= DIThreshold::TARGET_MAX) {
    // DI 70～75: 目標範囲内 → 現状維持（自動モード）
    // &&は「かつ」を意味する論理演算子（両方の条件が真の時に真）
    Serial.printf("[AC] DI %.1f (快適範囲) → 自動+1度\n", di);
    return ACMode::AUTO_PLUS_1;
  }
  else if (di < DIThreshold::HEATING_THRESHOLD) {
    // DI 68未満: 肌寒い → 自動モードで暖房も可能に
    Serial.printf("[AC] DI %.1f (肌寒い) → 自動+1度\n", di);
    return ACMode::AUTO_PLUS_1;
  }
  else {
    // DI 68～70: わずかに低い → 自動モード
    // どの条件にも当てはまらなかった場合（デフォルト）
    Serial.printf("[AC] DI %.1f (やや涼しい) → 自動+1度\n", di);
    return ACMode::AUTO_PLUS_1;
  }
}

/**
 * 赤外線リモコン信号を受信して内容を表示する（デバッグ用）
 *
 * リモコンのボタンを押した時の信号を受信し、詳細情報を
 * シリアルモニタに表示します。自分でIR信号を作る時に便利。
 */
void AirConditionerController::handleIRReceive() {
  decode_results results;  // 受信結果を格納する構造体

  // irRecv_.decode()は信号を受信した時にtrueを返す
  if (irRecv_.decode(&results)) {
    Serial.println("====================================");
    Serial.print("[IR] 受信コード: ");
    // 64ビット整数を16進数で表示（HEXは16進数を意味する）
    serialPrintUint64(results.value, HEX);
    Serial.println();
    Serial.print("[IR] プロトコル: ");
    Serial.println(typeToString(results.decode_type));  // 例：DAIKIN、NEC等
    Serial.print("[IR] ビット数: ");
    Serial.println(results.bits);  // 信号のビット長

    // Raw形式でも出力（生の信号タイミングデータ）
    // これをコピペすれば同じ信号を再送信できる
    Serial.print("uint16_t rawData[");
    Serial.print(results.rawlen - 1);
    Serial.println("] = {");
    Serial.print("  ");
    // forループ：i=1から始めて、results.rawlenより小さい間、繰り返す
    for (uint16_t i = 1; i < results.rawlen; i++) {
      Serial.print(results.rawbuf[i] * kRawTick);  // タイミングをマイクロ秒に変換
      if (i < results.rawlen - 1) Serial.print(", ");  // 最後以外はカンマをつける
      if (i % 10 == 0) Serial.print("\n  ");  // 10個ごとに改行（見やすくするため）
    }
    Serial.println("\n};");
    Serial.println("====================================");

    // 次の信号を受信できるようにする
    irRecv_.resume();
  }
}

/**
 * エアコンを停止（電源オフ）
 * 23時の自動停止機能などで使用
 */
void AirConditionerController::sendOff() {
  Serial.println("[AC] エアコン停止 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  irRecv_.disableIRIn();

  // ダイキンエアコンの電源をオフ
  daikinAC_.off();  // 電源OFF

  // IR信号を実際に送信（ここで赤外線LEDが光る）
  daikinAC_.send();

  Serial.println("[AC] エアコン停止 送信完了");

  // 受信を再度有効化
  delay(200);  // 送信完了を待つ
  irRecv_.enableIRIn();
}

/**
 * 冷房20度の信号を送信
 * 暑い時（DI 77以上）に使用する強力な冷房モード
 */
void AirConditionerController::sendCooling20() {
  Serial.println("[AC] 冷房20度 送信開始");

  // 受信を無効化（送信中の干渉を防ぐ）
  // 送信と受信を同時に行うと誤動作するため、送信中は受信を止める
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定（ドット「.」でメソッドを呼び出す）
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinCool);        // 冷房モード（kDaikinCoolは定数）
  daikinAC_.setTemp(20);                 // 温度20度に設定
  daikinAC_.setFan(kDaikinFanAuto);      // 風量は自動調整
  daikinAC_.setSwingVertical(false);     // 上下スイング無効
  daikinAC_.setSwingHorizontal(false);   // 左右スイング無効

  // IR信号を実際に送信（ここで赤外線LEDが光る）
  daikinAC_.send();

  Serial.println("[AC] 冷房20度 送信完了");

  // 受信を再度有効化（送信完了後、少し待ってから受信を再開）
  delay(200);  // 200ミリ秒待つ
  irRecv_.enableIRIn();
}

/**
 * 自動+1度の信号を送信
 * 快適範囲内、または肌寒い時（DI 70〜75、または68未満）に使用
 * 自動モードなので、冷房/暖房を自動で切り替えてくれる
 */
void AirConditionerController::sendAutoPlus1() {
  Serial.println("[AC] 自動+1度 送信開始");

  // 受信を無効化（送信中の干渉防止）
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinAuto);        // 自動モード（冷暖房を自動判断）
  daikinAC_.setTemp(27);                 // 温度27度（基準26度+1度、寒がり向け）
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.setSwingVertical(false);     // スイング無効
  daikinAC_.setSwingHorizontal(false);   // 水平スイング無効

  // IR信号送信
  daikinAC_.send();

  Serial.println("[AC] 自動+1度 送信完了");

  // 受信を再度有効化
  delay(200);  // 送信完了を待つ
  irRecv_.enableIRIn();
}

/**
 * 除湿-1.5度の信号を送信
 * やや暑い時（DI 75〜77）に使用
 * 湿度を下げることで体感温度を下げ、快適性を向上させる
 */
void AirConditionerController::sendDehumidMinus1_5() {
  Serial.println("[AC] 除湿-1.5 送信開始");

  // 受信を無効化（送信中の干渉防止）
  irRecv_.disableIRIn();

  // ダイキンエアコンの設定
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinDry);         // 除湿モード（湿度を下げる）
  daikinAC_.setTemp(24.5);               // 温度24.5度（基準26度-1.5度）
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.setSwingVertical(false);     // スイング無効
  daikinAC_.setSwingHorizontal(false);   // 水平スイング無効

  // IR信号送信
  daikinAC_.send();

  Serial.println("[AC] 除湿-1.5 送信完了");

  // 受信を再度有効化
  delay(200);  // 送信完了を待つ
  irRecv_.enableIRIn();
}
