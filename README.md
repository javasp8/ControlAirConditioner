📋 リファクタリング内容

  🗂️ 新しいファイル構造

  include/ (ヘッダーファイル)
  - DaikinIRData.h - IRデータ定義（名前空間で整理）
  - AirConditionerController.h - エアコン制御クラス
  - EnvironmentSensor.h - 環境センサークラス
  - DisplayController.h - ディスプレイ制御クラス

  src/ (実装ファイル)
  - AirConditionerController.cpp - エアコン制御実装
  - EnvironmentSensor.cpp - センサー制御実装
  - DisplayController.cpp - ディスプレイ制御実装
  - main.cpp - メインループ（104行、元の430行から大幅削減）

  🎯 主な改善点

  1. クラス化による責務の分離
    - エアコン制御、センサー、ディスプレイを独立したクラスに分離
    - 各クラスが単一責任を持つ設計
  2. 名前空間による整理
    - DaikinIR::Cooling20、DaikinIR::AutoPlus1などでIRデータを整理
    - 設定値をHardwareConfig、SensorConfig、TimingConfigで管理
  3. 命名の改善
    - ACMode::COOLING_20 (冷房20度)
    - ACMode::AUTO_PLUS_1 (自動+1度)
    - ACMode::DEHUMID_MINUS_1_5 (除湿-1.5)
    - わかりやすい変数名と定数名
  4. 保守性の向上
    - ハードコードされた値をconstexpr定数に置き換え
    - ログメッセージに[AC]、[Sensor]、[Display]のプレフィックスを追加
    - エラーハンドリングの統一
  5. 再利用性
    - 各クラスは独立して他のプロジェクトでも使用可能
    - IRデータは簡単に追加・変更可能


✅ 修正完了

  ブログとIRremoteESP8266ライブラリの調査に基づいて、以下の重要な修正を実施しました：

  🔄 主な変更点

  1. sendRaw()からIRDaikinESPクラスへ移行
    - ❌ 旧: 手動でRawデータを5フレームに分けて送信
    - ✅ 新: IRDaikinESPクラスを使用し、設定値を指定するだけで自動送信
  2. コードの大幅な簡素化
    - DaikinIRData.h（280行のRawデータ定義）を削除
    - AirConditionerController.cppを188行→172行に削減
    - フレーム分割やタイミング管理が不要に
  3. 動作の信頼性向上
    - ライブラリが正しいダイキンプロトコルで自動送信
    - フレーム間ギャップ、チェックサム計算などを自動処理
    - エラーが起きにくい設計

  📝 新しい設定方法

  // 冷房20度の例
  daikinAC_.on();                        // 電源ON
  daikinAC_.setMode(kDaikinCool);        // 冷房モード
  daikinAC_.setTemp(20);                 // 温度20度
  daikinAC_.setFan(kDaikinFanAuto);      // 風量自動
  daikinAC_.send();                      // 送信
