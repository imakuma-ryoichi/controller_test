#!/usr/bin/env python3
"""
Wi-Fi (UDP) Controller Sender for PS5 DualSense.
Reads controller inputs via evdev / Linux joystick and sends raw axes/buttons payload via UDP.
"""
import socket
import json
import time
import argparse
from typing import Dict, Any, List

def send_data_udp(sock: socket.socket, host: str, port: int, data: Dict[str, Any]) -> None:
    """Send JSON-encoded data to host:port over UDP."""
    payload = json.dumps(data).encode('utf-8')
    sock.sendto(payload, (host, port))

def main():
    parser = argparse.ArgumentParser(description="PS5 Controller UDP Sender")
    parser.add_argument("--host", type=str, default="127.0.0.1", help="Receiver IP address (Wi-Fi)")
    parser.add_argument("--port", type=int, default=9999, help="Receiver UDP port (default: 9999)")
    parser.add_argument("--rate", type=float, default=50.0, help="Send rate in Hz (default: 50)")
    parser.add_argument("--mock", action="store_true", help="Send mock data for testing without controller")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    interval = 1.0 / args.rate

    print(f"[UDP Sender] Target: {args.host}:{args.port} | Rate: {args.rate} Hz")

    if args.mock:
        print("[UDP Sender] Running in mock test mode...")
        t0 = time.time()
        try:
            while True:
                elapsed = time.time() - t0
                # Mock sample joy data (PS5 axes and buttons)
                sample = {
                    "axes": [0.0, 0.0, 0.0, 0.0, -1.0, -1.0, 0.0, 0.0],
                    "buttons": [0] * 14,
                    "timestamp": time.time()
                }
                send_data_udp(sock, args.host, args.port, sample)
                time.sleep(interval)
        except KeyboardInterrupt:
            print("\n[UDP Sender] Stopped.")
        return

    # Real controller read using pygame or evdev if installed
    try:
        import pygame
        pygame.init()
        pygame.joystick.init()
        if pygame.joystick.get_count() == 0:
            print("[UDP Sender] No joystick detected! Please connect PS5 controller or use --mock.")
            return

        js = pygame.joystick.Joystick(0)
        js.init()
        print(f"[UDP Sender] Connected to: {js.get_name()}")

        while True:
            pygame.event.pump()
            num_axes = js.get_numaxes()
            num_buttons = js.get_numbuttons()

            axes = [float(js.get_axis(i)) for i in range(num_axes)]
            buttons = [int(js.get_button(i)) for i in range(num_buttons)]

            payload = {
                "axes": axes,
                "buttons": buttons,
                "timestamp": time.time()
            }
            send_data_udp(sock, args.host, args.port, payload)
            time.sleep(interval)

    except ImportError:
        print("[UDP Sender] pygame not found. Falling back to test mock mode. Install pygame for hardware read (`pip install pygame`).")
        # Run mock mode
        while True:
            sample = {
                "axes": [0.0] * 8,
                "buttons": [0] * 14,
                "timestamp": time.time()
            }
            send_data_udp(sock, args.host, args.port, sample)
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\n[UDP Sender] Stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
