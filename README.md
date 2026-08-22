# Controller Communication System

対応コントローラーの入力をLinux Joystick APIから取得し、生データのまま100 Hzで送信するシステム（例: PS5 DualSense Controller）。

SenderとReceiverを分離し、Senderでは入力取得と通信のみを担当する。

ReceiverではWi-FiおよびBluetoothからデータを受信し、最終的にROS 2へ変換する。

---

# システム構成

    対応コントローラー（例: PS5 DualSense Controller）
          │
          ↓
    /dev/input/jsX
          │
          ↓
    Linux Joystick API
          │
          ↓
    ControllerData
          │
       100 Hz
          │
      ┌───┴───┐
      ↓       ↓
    Wi-Fi   Bluetooth
      TCP     RFCOMM
      ↓       ↓
      └───┬───┘
          ↓
    controller-receiver
          │
          ↓
         ROS 2

---

# リポジトリ構成

SenderとReceiverは別リポジトリとして管理する。

## Sender

    controller-sender/
    ├── Makefile
    ├── README.md
    ├── .gitignore
    ├── config/
    │   └── controller_id.yaml
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

## Receiver

    controller-receiver/
    ├── Makefile
    ├── README.md
    ├── .gitignore
    ├── include/
    │   ├── controller_data.hpp
    │   ├── wifi_receiver.hpp
    │   └── bluetooth_receiver.hpp
    └── src/
        ├── main.cpp
        ├── wifi_receiver.cpp
        └── bluetooth_receiver.cpp

---

# Sender

## 役割

Senderは以下のみを担当する。

1. 対応コントローラー（例: PS5 DualSense Controller）から入力を取得
2. Linux Joystick APIから全入力イベントを取得
3. 現在の全入力状態を保持
4. ControllerDataにまとめる
5. 100 Hzで送信

Senderでは入力値の意味付けや加工を行わない。

以下はReceiver側で行う。

- 正規化
- デッドゾーン処理
- 操作量への変換
- ROS 2への変換
- ロボット制御

---

# 入力方式

LinuxのJoystick APIを使用する。

入力デバイス：

    /dev/input/jsX

例：

    /dev/input/js0
    /dev/input/js1

Senderでは基本入力を`/dev/input/jsX`から取得し、タッチパッドクリック（`EV_KEY` / `BTN_LEFT`、code 272）は`touchpad.event_device`で指定した`/dev/input/eventX`から取得する。指定先がタッチパッドデバイスでない場合は、`ABS_MT_POSITION_X`と指定ボタンを持つeventデバイスを自動検索する。軸イベント（`ABS_X/Y`、`ABS_MT_POSITION_X/Y`）は取得対象にしない。

---

# ControllerData

ControllerDataにはコントローラーの14入力の現在状態を保持する。

軸：

    axes[6] : 左スティックXY、右スティックXY、L2、R2

ボタン：

    buttons[14] : 13個のJoystickボタン + touchpad押下(0/1)

`axes[]` はLinux Joystick APIの生の`int32_t`値を保持し、L2/R2の入力加減も含む。
`buttons[]` は押下を1、離上を0として保持する。

値の正規化やデッドゾーン処理は行わない。

---

# 送信周期

送信周波数は100 Hz。

    1000 ms ÷ 100 Hz = 10 ms

したがって10 ms周期でControllerData全体を送信する。

入力イベントが発生するたびに送信する方式ではない。

入力イベントを読み続けて現在状態を更新し、10 msごとにその時点の全入力状態をまとめて送信する。

---

# 通信方式

## Wi-Fi

TCPを使用する。

    Sender
      ↓
    TCP
      ↓
    Receiver

TCPポート：

    5000

## Bluetooth

BluetoothのRFCOMMを使用する。

    Sender
      ↓
    RFCOMM
      ↓
    Receiver

Wi-FiとBluetoothは、それぞれ単独で動作確認した後、両方を同時に使用する。

---

# 必要環境

## Sender

- Linux
- C++
- GNU Compiler Collection
- GNU Make
- BlueZ
- Linux Joystick API
- 対応機種の一例: PS5 DualSense Controller

## Receiver

- Linux
- C++
- GNU Compiler Collection
- GNU Make
- BlueZ
- Wi-Fi
- Bluetooth

---

# 必要パッケージ

## Sender

以下をインストールする。

    sudo apt update
    sudo apt install build-essential bluez libbluetooth-dev joystick

## Receiver

以下をインストールする。

    sudo apt update
    sudo apt install build-essential bluez libbluetooth-dev

---

# パッケージの用途

| パッケージ | 用途 |
|---|---|
| build-essential | GNU Compiler Collection、GNU Makeなど |
| bluez | LinuxのBluetooth機能 |
| libbluetooth-dev | C/C++からBluetoothを使用するための開発用ファイル |
| joystick | jstestによるコントローラー確認 |

ROS 2はSenderには必要ない。

ROS 2への変換はReceiver側で行う。

---

# Senderのセットアップ

## 1. DualSenseを接続

PS5 DualSenseをRaspberry PiへBluetoothまたはUSBで接続する。

## 2. Joystickデバイスを確認

    ls -l /dev/input/js*

例：

    /dev/input/js0
    /dev/input/js1

## 3. 入力デバイスを確認

    jstest /dev/input/js0

または、

    jstest /dev/input/js1

DualSenseのスティック、トリガー、ボタンなどを操作し、入力値が変化するデバイスを確認する。

---

# Senderの設定

`config/controller_id.yaml`でJoystickイベント番号と`ControllerData`の対応を設定する。
このファイルはビルド後も変更でき、起動時に読み込まれる。別の設定ファイルを使う場合：

    ./build/controller_sender --config /path/to/controller_id.yaml

標準設定ファイル：

    controller_sender/config/controller_id.yaml

コントローラーのデバイス、ReceiverのIPアドレス、ポートなどの通信設定は`src/main.cpp`で設定する。

---

# Senderのネットワーク設定

ReceiverのIPアドレスをSenderに設定する。

例：

    192.168.1.100

TCPポートは5000を使用する。

---

# Senderのビルド

Senderのディレクトリで実行する。

    make clean
    make

Makefileがbuildディレクトリを自動的に作成する。

生成される実行ファイル：

    build/controller_sender

インストール時は設定ファイルを`/etc/controller_sender/controller_id.yaml`へ配置する：

    sudo make install

---

# Senderの実行

    make run

または、

    ./build/controller_sender

---

# Receiverのビルド

Receiverのディレクトリで実行する。

    make clean
    make

生成される実行ファイル：

    build/controller_receiver

---

# Receiverの実行

    make run

または、

    ./build/controller_receiver

---

# 動作確認

動作確認は以下の順番で行う。

    1. DualSense認識確認
    2. Joystick API入力確認
    3. Sender単体確認
    4. Receiver単体確認
    5. Wi-Fi通信確認
    6. Bluetooth通信確認
    7. Wi-Fi + Bluetooth同時通信確認
    8. 100 Hz確認
    9. systemd確認

---

# 1. DualSense認識確認

    ls -l /dev/input/js*

DualSenseに対応するjsXが存在することを確認する。

---

# 2. Joystick API入力確認

    jstest /dev/input/js0

DualSenseを操作して、軸やボタンの値が変化することを確認する。

確認対象：

- 左スティック
- 右スティック
- L2
- R2
- 十字キー
- ×
- ○
- □
- △
- L1
- R1
- L3
- R3
- Create
- Options
- PS
- タッチパッド

実際の軸番号・ボタン番号はLinux側のデバイスマッピングに依存するため、jstestで確認する。

---

# 3. Sender単体確認

まず通信部分を確認する前に、入力取得が正常か確認する。

    make clean
    make
    make run

デバッグ時にはControllerDataのaxes[]とbuttons[]を表示して確認する。

確認する内容：

- スティック操作で対応する軸値が変化する
- トリガー操作で対応する軸値が変化する
- ボタンを押すと対応するボタン状態が変化する
- ボタンを離すと状態が戻る
- 全入力状態がControllerDataに保持される

---

# 4. Receiver単体確認

Receiverを先に起動する。

    make run

ReceiverがWi-FiおよびBluetoothの待受状態になっていることを確認する。

---

# 5. Wi-Fi通信確認

Receiverを先に起動する。

その後、Senderを起動する。

    make run

DualSenseを操作し、Receiver側でControllerDataを受信できることを確認する。

通信経路：

    DualSense
        ↓
    /dev/input/jsX
        ↓
    Sender
        ↓
    TCP
        ↓
    Receiver

---

# 6. Bluetooth通信確認

Bluetoothを有効化する。

    bluetoothctl

必要に応じてBluetoothのペアリングを行う。

Receiverを起動し、その後Senderを起動する。

Bluetooth通信が確立し、ControllerDataを受信できることを確認する。

通信経路：

    DualSense
        ↓
    /dev/input/jsX
        ↓
    Sender
        ↓
    Bluetooth RFCOMM
        ↓
    Receiver

---

# 7. Wi-Fi + Bluetooth同時通信確認

Wi-FiとBluetoothをそれぞれ単独で確認した後、両方を同時に動作させる。

    DualSense
        ↓
    Sender
        ├── Wi-Fi TCP ────────→ Receiver
        └── Bluetooth RFCOMM ─→ Receiver

両方からControllerDataを受信できることを確認する。

---

# 8. 100 Hz確認

Senderは10 ms周期でControllerDataを送信する。

    1000 ms ÷ 10 ms = 100 Hz

Receiver側で受信数を計測し、約100 packets/sとなることを確認する。

実際の受信レートは、オペレーティングシステムのスケジューリング、ネットワーク遅延、通信状態などによって多少変動する。

---

# 9. systemd

Senderは最終的にsystemdサービスとして動作させる。

サービスファイル：

    systemd/controller-sender.service

サービスファイルを配置：

    sudo cp systemd/controller-sender.service /etc/systemd/system/

systemdの設定を再読み込み：

    sudo systemctl daemon-reload

自動起動を有効化：

    sudo systemctl enable controller-sender

起動：

    sudo systemctl start controller-sender

状態確認：

    systemctl status controller-sender

ログ確認：

    journalctl -u controller-sender -f

停止：

    sudo systemctl stop controller-sender

自動起動を無効化：

    sudo systemctl disable controller-sender

---

# Makefile

Makefileでは以下を自動的に行う。

- C++ソースのコンパイル
- buildディレクトリの作成
- オブジェクトファイルの生成
- 実行ファイルの生成
- buildディレクトリの削除
- 実行

## ビルド

    make

## 実行

    make run

## クリーン

    make clean

---

# Git

ビルド成果物はGitへ登録しない。

.gitignoreには少なくとも以下を設定する。

    build/
    *.o

---

# Senderの責任範囲

Sender：

    DualSense
        ↓
    Linux Joystick API
        ↓
    全入力取得
        ↓
    ControllerData
        ↓
    100 Hz
        ↓
    Wi-Fi / Bluetooth

Senderでは入力データを加工しない。

---

# Receiverの責任範囲

Receiver：

    Wi-Fi / Bluetooth
        ↓
    ControllerData
        ↓
    入力データの解釈
        ↓
    必要な変換
        ↓
    ROS 2

Receiver側で必要に応じて以下を行う。

- 正規化
- デッドゾーン処理
- 軸の反転
- 操作量への変換
- Wi-Fi / Bluetoothの優先制御
- 通信状態の監視
- ROS 2メッセージへの変換

---

# 最終的なデータフロー

    PS5 DualSense
          ↓
    /dev/input/jsX
          ↓
    Linux Joystick API
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

Senderは「取得して送る」ことに専念し、Receiver側で入力データを必要な形式へ変換する。
