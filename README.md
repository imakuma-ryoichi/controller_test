# Controller Bridge System 

コントローラーからの入力を **Wi-Fi (UDP)** および **Bluetooth (RFCOMM)** 経由で受信し、**ROS 2 (`sensor_msgs/msg/Joy`)** トピック `/ps5/joy` に自動優先・フォールバック制御してパブリッシュするシステムです。

---

## 構成概要

```
[ PS5 Controller ]
       │
       ▼
[ controller_sender (Python) ]
       ├── Wi-Fi (UDP : ポート 9999) ───┐
       └── Bluetooth (RFCOMM) ──────────┤
                                        ▼
                      [ ROS 2: ps5_controller_bridge ]
                                        │ (Wi-Fi優先 / 遅延・途絶時はBT自動切替)
                                        ▼
                               Topic: /ps5/joy (sensor_msgs/msg/Joy)
```

---

## リポジトリ構成

- **`controller_sender/`** : コントローラー接続側（送信機 PC / Raspberry Pi など）
  - `python/wifi_sender.py` : 入力を UDP で送信
  - `python/bt_sender.py` : の入力を Bluetooth (RFCOMM) で送信
  - `systemd/` : 常駐化用サービス設定
- **`ros2/`** : ROS 2 受信・ブリッジパッケージ
  - `src/ps5_controller_bridge/` : 受信 ROS 2 ノード（Wi-Fi UDP & Bluetooth 受信、優先度切り替え、Joy パブリッシュ）

---

## セットアップ手順

### 1. 必要パッケージのインストール

**Receiver (ROS 2) 側 / Sender 側共通:**
```bash
sudo apt update
sudo apt install -y python3-pip libbluetooth-dev bluetooth
pip3 install pybluez pygame
```

---

## 起動方法

### 1. Receiver (ROS 2) 側の起動

ROS 2 ワークスペースのビルドとノード起動を行います。

```bash
# ros2 ディレクトリ内でビルド (または colcon build)
cd /home/user/controller-rox/ros2
colcon build --packages-select ps5_controller_bridge
source install/setup.bash

# ブリッジノードの起動
ros2 launch ps5_controller_bridge bridge_launch.py
```

受信待機設定のパラメータ（IP, ポート, BT MACアドレス）は launch 引数または環境変数で変更可能です。

---

### 2. Sender 側の起動

コントローラーが接続された端末で実行します。

#### Wi-Fi (UDP) 送信:
```bash
cd /home/user/controller-rox/controller_sender/python

# 通常起動 (受信機のIPアドレスを指定)
python3 wifi_sender.py --host <RECEIVER_IP> --port 9999

# コントローラー実機なしのテスト (モック送信)
python3 wifi_sender.py --host 127.0.0.1 --port 9999 --mock
```

#### Bluetooth (RFCOMM) 送信:
```bash
cd /home/user/controller-rox/controller_sender/python

# 受信機の Bluetooth MAC アドレスを指定
python3 bt_sender.py --addr <RECEIVER_BT_MAC> --port 1
```

---

## トピックの確認

ROS 2 側で正しく受信・パブリッシュされているか確認します。

```bash
# トピック一覧
ros2 topic list

# 受信 Joy メッセージの表示
ros2 topic echo /ps5/joy
```
