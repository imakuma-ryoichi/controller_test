#!/usr/bin/env python3
"""
Bluetooth (RFCOMM) Controller Sender for PS5 DualSense.
Sends controller inputs via Bluetooth RFCOMM socket.
"""
import time
import json
import argparse
from typing import Dict, Any

def main():
    parser = argparse.ArgumentParser(description="PS5 Controller Bluetooth Sender")
    parser.add_argument("--addr", type=str, default="00:11:22:33:44:55", help="Receiver Bluetooth MAC Address")
    parser.add_argument("--port", type=int, default=1, help="Receiver RFCOMM channel (default: 1)")
    parser.add_argument("--rate", type=float, default=50.0, help="Send rate in Hz (default: 50)")
    parser.add_argument("--mock", action="store_true", help="Send mock data for testing")
    args = parser.parse_args()

    try:
        import bluetooth
    except ImportError:
        print("[BT Sender] Error: pybluez is required for Bluetooth transmission. Install via `pip install pybluez`.")
        return

    print(f"[BT Sender] Connecting to {args.addr} on channel {args.port}...")
    sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    try:
        sock.connect((args.addr, args.port))
        print("[BT Sender] Connected.")
    except Exception as e:
        print(f"[BT Sender] Connection failed: {e}")
        return

    interval = 1.0 / args.rate
    try:
        while True:
            sample = {
                "axes": [0.0] * 8,
                "buttons": [0] * 14,
                "timestamp": time.time()
            }
            payload = json.dumps(sample).encode('utf-8')
            sock.send(payload)
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n[BT Sender] Stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
