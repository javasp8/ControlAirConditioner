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
