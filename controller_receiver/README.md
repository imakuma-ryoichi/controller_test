# Controller Communication System

Linuxに接続したコントローラーの生データを取得し、Wi-FiおよびBluetoothを利用して送受信するシステム。

最終的には受信側でROS 2へ変換する。

## システム構成

```text
Controller
    ↓
Linux evdev
    ↓
controller-sender
    ├── Wi-Fi → TCP
    └── Bluetooth → RFCOMM
              ↓
      controller-receiver
              ↓
        ControllerData
              ↓
            ROS 2
```

## リポジトリ構成

送信側と受信側は独立したリポジトリとして管理する。

```text
controller-sender/
controller-receiver/
```

## 通信仕様

| 項目 | 内容 |
|---|---|
| 入力インターフェース | Linux evdev |
| 入力デバイス | `/dev/input/eventX` |
| データ形式 | `ControllerData` |
| 送信周期 | 100 Hz |
| 送信間隔 | 10 ms |
| Wi-Fi | Transmission Control Protocol |
| TCPポート | 5000 |
| Bluetooth | Radio Frequency Communication |
| Bluetoothチャンネル | 1 |
| データ処理 | 生データのまま送信 |
| 受信後 | ROS 2へ変換予定 |

## ControllerData

senderとreceiverで共通のデータ構造を使用する。対応コントローラーの14入力を保持する（例: PS5 DualSense Controller）。

| データ | Linuxイベント |
|---|---|
| 軸配列 | 左スティックXY、右スティックXY、L2、R2 |
| ボタン配列 | 13個のJoystickボタン + touchpad押下 |

軸配列は`int32_t`の生データ、ボタン配列は押下時1・離上時0で送信する。

## controller-sender

Raspberry Piに接続したコントローラーからLinux evdevを利用して入力を取得し、100 HzでWi-FiおよびBluetoothへ送信する。

### ディレクトリ構成

```text
controller-sender/
├── .gitignore
├── Makefile
├── include/
│   ├── controller_data.hpp
│   ├── controller_input.hpp
│   ├── wifi_sender.hpp
│   └── bluetooth_sender.hpp
├── src/
│   ├── main.cpp
│   ├── controller_input.cpp
│   ├── wifi_sender.cpp
│   └── bluetooth_sender.cpp
└── systemd/
    └── controller-sender.service
```

### 使用技術

- C++
- Linux evdev
- Transmission Control Protocol
- Bluetooth Radio Frequency Communication
- BlueZ
- systemd

## controller-receiver

Wi-FiおよびBluetoothから`ControllerData`を受信する。

受信したデータは共通の`ControllerData`として扱い、最終的にROS 2へ変換する。

### ディレクトリ構成

```text
controller-receiver/
├── .gitignore
├── Makefile
├── include/
│   ├── controller_data.hpp
│   ├── wifi_receiver.hpp
│   └── bluetooth_receiver.hpp
└── src/
    ├── main.cpp
    ├── wifi_receiver.cpp
    └── bluetooth_receiver.cpp
```

## 必要パッケージ

Bluetooth通信にはBlueZの開発用ライブラリを使用する。

```bash
sudo apt update
sudo apt install libbluetooth-dev
```

コントローラーの入力イベントを確認する場合は`evtest`を使用する。

```bash
sudo apt install evtest
```

## コントローラーの確認

コントローラーをRaspberry Piへ接続し、入力デバイスを確認する。

```bash
ls -l /dev/input/
```

より詳細に確認する場合：

```bash
sudo evtest
```

`ABS_X`、`ABS_Y`、`ABS_RX`などのイベントが発生するデバイスをコントローラーとして使用する。

## ビルド

### sender

```bash
make
```

実行ファイル：

```text
build/controller_sender
```

実行：

```bash
make run
```

ビルド成果物を削除：

```bash
make clean
```

### receiver

```bash
make
```

実行ファイル：

```text
build/controller_receiver
```

実行：

```bash
make run
```

ビルド成果物を削除：

```bash
make clean
```

## senderの設定

`src/main.cpp`の以下の値を実環境に合わせて変更する。

- コントローラーの`/dev/input/eventX`
- receiverのIPアドレス
- Wi-FiのTCPポート
- receiverのBluetoothアドレス
- BluetoothのRFCOMMチャンネル

## Wi-Fi動作確認

1. receiver側でプログラムを起動する。
2. sender側でプログラムを起動する。
3. コントローラーを操作する。
4. receiver側にX軸、Y軸、回転の値が表示されることを確認する。

receiver側では以下のように表示される。

```text
[Wi-Fi] x=123 y=-456 rotation=789
```

## Bluetooth動作確認

1. senderとreceiverのBluetoothを有効化する。
2. 必要に応じて`bluetoothctl`でペアリングする。
3. receiverを起動する。
4. senderを起動する。
5. Bluetooth接続が確立することを確認する。
6. コントローラーを操作する。
7. receiver側でデータを受信できることを確認する。

Bluetooth接続後は以下のように表示される。

```text
Bluetooth接続
[Bluetooth] x=123 y=-456 rotation=789
```

## 推奨する動作確認順序

```text
コントローラー認識
    ↓
evdevイベント確認
    ↓
senderの入力取得確認
    ↓
receiver起動確認
    ↓
Wi-Fi通信確認
    ↓
100 Hz送信確認
    ↓
Bluetooth通信確認
    ↓
Wi-Fi + Bluetooth同時通信確認
    ↓
systemd確認
    ↓
ROS 2への変換
```

## systemd

senderは最終的にsystemdによって常駐・自動起動する。

サービスファイル：

```text
systemd/controller-sender.service
```

実行ファイルを配置：

```bash
sudo install -m 755 build/controller_sender /usr/local/bin/controller_sender
```

サービスファイルを配置：

```bash
sudo cp systemd/controller-sender.service /etc/systemd/system/
```

systemdの設定を再読み込み：

```bash
sudo systemctl daemon-reload
```

自動起動を有効化：

```bash
sudo systemctl enable controller-sender
```

起動：

```bash
sudo systemctl start controller-sender
```

状態確認：

```bash
systemctl status controller-sender
```

ログ確認：

```bash
journalctl -u controller-sender -f
```

停止：

```bash
sudo systemctl stop controller-sender
```

自動起動を無効化：

```bash
sudo systemctl disable controller-sender
```

## 現在のデータフロー

```text
Controller
    ↓
/dev/input/eventX
    ↓
controller-sender
    ↓
ControllerData
    ↓
100 Hz
    ├──────────────┐
    ↓              ↓
  Wi-Fi        Bluetooth
   TCP           RFCOMM
    ↓              ↓
    └───────┬──────┘
            ↓
   controller-receiver
            ↓
     ControllerData
            ↓
          ROS 2
```

## 今後の実装

1. Wi-Fi通信の安定性確認
2. Bluetooth通信の安定性確認
3. Wi-Fi / Bluetoothの通信状態監視
4. Wi-Fi / Bluetoothの優先制御
5. 通信断時の処理
6. 受信データの共通処理
7. ROS 2メッセージへの変換
8. senderのsystemdによる完全自動起動
9. 必要に応じた通信方式・送信周期の最適化
