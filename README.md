# Controller Communication System (Python版)

## 概要
- コントローラ入力取得 → Wi‑Fi (UDP) & Bluetooth (RFCOMM) へ同時送信
- **C++ 実装は全て削除**し、Python スクリプト `controller_sender.py` のみで動作
- systemd による常駐サービスとして利用想定

## 必要環境
- Ubuntu/Debian 系 Linux (Raspberry Pi 等)
- Python 3.9 以上
- 必要パッケージ（1回だけ実行）
```bash
sudo apt update
sudo apt install python3-pip python3-evdev bluetooth libbluetooth-dev
pip3 install pyyaml pybluez
```

## 設定ファイル (`config.yaml`)
プロジェクト直下に配置する例:
```yaml
controller_device: "/dev/input/event0"   # 取得したいコントローラの evdev パス
touchpad_device: "/dev/input/event1"   # 使用しない場合は false にする
touchpad_enabled: false
wifi:
  enabled: true
  address: "192.168.1.100"
  port: 5000
bluetooth:
  enabled: true
  address: "01:23:45:67:89:AB"
  channel: 1
send_rate_hz: 60
```
必要に応じてデバイスパスやアドレスを書き換えてください。

## 実行方法
### 手動実行
```bash
cd /home/user/controller-rox
python3 controller_sender.py --config config.yaml
```
Ctrl+C で終了します。

### systemd で常駐
`/etc/systemd/system/controller-sender.service` として配置例:
```ini
[Unit]
Description=Controller sender (Python)
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/user/controller-rox/controller_sender.py --config /home/user/controller-rox/config.yaml
Restart=always
User=your_user   # 適宜置換

[Install]
WantedBy=multi-user.target
```
```bash
# 配置 & 有効化
sudo cp systemd/controller-sender.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable controller-sender
sudo systemctl start controller-sender
# 状態確認
systemctl status controller-sender
# ログ確認
journalctl -u controller-sender -f
```

## テスト手順
1. コントローラが `/dev/input/event0` 等に認識されているか確認 (`ls /dev/input/event*`)。
2. `jstest` 等で入力が取得できることを確認。
3. `controller_sender.py` 起動後、別端末で `nc -u <IP> 5000` で UDP パケット受信を確認。
4. Bluetooth が有効なら `bluetoothctl` でペアリングし、RFCOMM が開通しているか確認。
5. systemd サービスとして自動起動し、`journalctl -u controller-sender -f` でエラーログが無いことを確認。

## これまでの C++ 実装は削除済み
- `controller_sender/src/*.cpp` と `controller_receiver/*.cpp` は全て削除しました。
- 参考になる Makefile 等は削除して問題ありません。

---

### License
MIT License


対応コントローラーの入力をLinux Joystick APIから取得し、生データのまま設定周期で送信するシステム（例: PS5 DualSense Controller）。

SenderとReceiverを分離し、Senderでは入力取得と通信のみを担当する。

ReceiverではWi-FiおよびBluetoothからデータを受信し、最終的にROS 2へ変換する。

---

# 最初に見るところ

## 全体の流れ

    Controller
        ↓
    Sender
        ↓ Wi-Fi UDP / Bluetooth RFCOMM
    Receiver / ROS 2
        ↓
    /controller/joy

## Receiver / ROS 2側の起動

Receiver側PCで実行する。

```bash
cd ~/controller_test
git pull
source /opt/ros/humble/setup.bash
make receiver-ros2
make launch
```

`make launch`で`ros2/build/joy_publisher`が起動し、`/controller/joy`へ`sensor_msgs/msg/Joy`をPublishする。

トピック確認：

```bash
ros2 topic list
ros2 topic echo /controller/joy
```

ビルドが怪しいときは一度cleanする。

```bash
make -C ros2 clean
make receiver-ros2
```

## Sender側の起動

Sender側PCで実行する。

```bash
cd ~/controller_test/controller_sender
make
./build/controller_sender
```

Makefile経由で起動する場合：

```bash
cd ~/controller_test/controller_sender
make run
```

設定ファイルを明示する場合：

```bash
./build/controller_sender \
  --config config/controller_id.yaml \
  --connection-config config/controller_connection.yaml
```

## よく触る設定

Senderの送信先IP、ポート、入力デバイス：

```text
controller_sender/config/controller_connection.yaml
```

使わない通信はOFFにする。Raspberry Pi 5からWi-Fiだけで送るなら、BluetoothはOFF推奨。

```yaml
wifi:
  enabled: true

bluetooth:
  enabled: false
```

Senderのボタン/軸割り当て、タッチパッド、送信周期：

```text
controller_sender/config/controller_id.yaml
```

送信周期を遅くしてデバッグする例：

```yaml
send_rate_hz: 10
```

タッチパッドを使わないなら、evdev探索と読み取りを止める。

```yaml
touchpad:
  enabled: false
```

Receiverの待受UDPポート、Bluetooth RFCOMMチャンネル：

```text
controller_receiver/config/receiver_connection.yaml
```

ROS 2側でWi-Fi/Bluetoothの優先順と無通信タイムアウトを変える設定：

```text
ros2/config/comm_config.yaml
```

RDK側を軽くしたい場合は、ROS 2のpoll周期を下げ、新規パケットが来たときだけPublishする。

```yaml
poll_rate_hz: 50
publish_on_new_data_only: true
```

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
       設定周期
          │
      ┌───┴───┐
      ↓       ↓
    Wi-Fi   Bluetooth
      UDP     RFCOMM
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
5. 設定周期で送信

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

送信周波数は`controller_sender/config/controller_id.yaml`の`send_rate_hz`で設定する。
既定値は100 Hz。

    1000 ms ÷ 100 Hz = 10 ms

既定値では10 ms周期でControllerData全体を送信する。

入力イベントが発生するたびに送信する方式ではない。

入力イベントを読み続けて現在状態を更新し、設定周期ごとにその時点の全入力状態をまとめて送信する。

---

# 通信方式

## Wi-Fi

UDPを使用する。

    Sender
      ↓
    UDP
      ↓
    Receiver

UDPポート：

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

コントローラーのデバイス、ReceiverのIPアドレス、ポートなどの通信設定は`config/controller_connection.yaml`で設定する。

---

# Senderのネットワーク設定

ReceiverのIPアドレスをSenderの`config/controller_connection.yaml`に設定する。

例：

    192.168.1.100

Senderの`wifi.port`とReceiverの`wifi.port`を同じ値（既定値は5000）に設定する。

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
    UDP
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
        ├── Wi-Fi UDP ────────→ Receiver
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
    設定周期
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
         設定周期
          ├──────────────┐
          ↓              ↓
        Wi-Fi        Bluetooth
         UDP           RFCOMM
          ↓              ↓
          └───────┬──────┘
                  ↓
         controller-receiver
                  ↓
             ControllerData
                  ↓
                ROS 2 (sensor_msgs/msg/Joy)

Senderは「取得して送る」ことに専念し、Receiver側で入力データを必要な形式へ変換する。

---

# ビルド・実行コマンド一覧（Root Makefile）

リポジトリ直下の `Makefile` から各種ビルド・起動を実行できます。

### 1. ビルド
- **Senderのみビルド:**
  ```bash
  make sender
  ```
- **Receiver & ROS 2をビルド:**
  ```bash
  make receiver-ros2
  ```

### 2. 実行
- **単体 Receiver（受信のみ）の起動:**
  ```bash
  make run-receiver
  ```
- **ROS 2 ノード（Joy変換・Publish）の起動:**
  ```bash
  make launch
  ```

ROS 2ノードは`/controller/joy`へ`sensor_msgs/msg/Joy`をPublishします。
Wi-Fi/Bluetoothの優先順位と無通信タイムアウトは`ros2/config/comm_config.yaml`で設定します。
受信ポートとRFCOMMチャンネルは`controller_receiver/config/receiver_connection.yaml`を使います。

### 3. クリーン
- **全ビルド成果物の削除:**
  ```bash
  make clean
  ```
