import socket
import json
from typing import Any

def send_data(host: str, port: int, data: Any) -> None:
    """Send JSON‑encoded `data` to ``host:port`` over TCP.
    The message is prefixed with a 4‑byte length header.
    """
    payload = json.dumps(data).encode('utf-8')
    length = len(payload).to_bytes(4, byteorder='big')
    with socket.create_connection((host, port)) as sock:
        sock.sendall(length + payload)

if __name__ == "__main__":
    # Example payload – replace with actual controller data
    sample = {"axis_x": 0.5, "axis_y": -0.2, "button_a": True}
    send_data("192.168.1.100", 9999, sample)
