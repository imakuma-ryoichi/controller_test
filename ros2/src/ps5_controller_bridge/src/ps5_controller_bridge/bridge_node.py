import threading
import queue
import time
import socket
import json
import logging
from typing import Dict, Any

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy

# Simple receiver for Wi‑Fi (TCP)
class WifiReceiver(threading.Thread):
    def __init__(self, host: str, port: int, out_queue: queue.Queue):
        super().__init__(daemon=True)
        self.host = host
        self.port = port
        self.out_queue = out_queue
        self._stop_event = threading.Event()
        self.logger = logging.getLogger('WifiReceiver')

    def run(self) -> None:
        while not self._stop_event.is_set():
            try:
                with socket.create_connection((self.host, self.port), timeout=5) as sock:
                    # Expect 4‑byte length prefix
                    length_bytes = sock.recv(4)
                    if len(length_bytes) < 4:
                        continue
                    length = int.from_bytes(length_bytes, byteorder='big')
                    payload = sock.recv(length)
                    while len(payload) < length:
                        payload += sock.recv(length - len(payload))
                    data = json.loads(payload.decode('utf-8'))
                    self.out_queue.put(('wifi', data))
            except Exception as e:
                self.logger.debug(f'Wi‑Fi receive error: {e}')
                time.sleep(1)
        self.logger.info('Wi‑Fi receiver stopped')

    def stop(self):
        self._stop_event.set()

# Simple receiver for Bluetooth (RFCOMM) using pybluez
class BtReceiver(threading.Thread):
    def __init__(self, bt_addr: str, port: int, out_queue: queue.Queue):
        super().__init__(daemon=True)
        self.bt_addr = bt_addr
        self.port = port
        self.out_queue = out_queue
        self._stop_event = threading.Event()
        self.logger = logging.getLogger('BtReceiver')

    def run(self) -> None:
        try:
            import bluetooth  # pybluez
        except ImportError:
            self.logger.error('pybluez not installed')
            return
        while not self._stop_event.is_set():
            try:
                sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
                sock.settimeout(5)
                sock.connect((self.bt_addr, self.port))
                while not self._stop_event.is_set():
                    data = sock.recv(1024)
                    if not data:
                        break
                    try:
                        msg = json.loads(data.decode('utf-8'))
                        self.out_queue.put(('bt', msg))
                    except json.JSONDecodeError:
                        continue
                sock.close()
            except Exception as e:
                self.logger.debug(f'Bluetooth receive error: {e}')
                time.sleep(1)
        self.logger.info('Bluetooth receiver stopped')

    def stop(self):
        self._stop_event.set()


class BridgeNode(Node):
    def __init__(self):
        super().__init__('ps5_controller_bridge')
        self.declare_parameter('wifi_host', '192.168.1.100')
        self.declare_parameter('wifi_port', 9999)
        self.declare_parameter('bt_addr', '00:11:22:33:44:55')
        self.declare_parameter('bt_port', 1)
        self.declare_parameter('fallback_latency_threshold_ms', 200)

        self.publisher_ = self.create_publisher(Joy, 'ps5/joy', 10)
        self.queue = queue.Queue()
        self.active_source = 'wifi'  # start with wifi

        # Start receivers
        self.wifi_receiver = WifiReceiver(
            host=self.get_parameter('wifi_host').get_parameter_value().string_value,
            port=self.get_parameter('wifi_port').get_parameter_value().integer_value,
            out_queue=self.queue,
        )
        self.bt_receiver = BtReceiver(
            bt_addr=self.get_parameter('bt_addr').get_parameter_value().string_value,
            port=self.get_parameter('bt_port').get_parameter_value().integer_value,
            out_queue=self.queue,
        )
        self.wifi_receiver.start()
        self.bt_receiver.start()

        self.timer = self.create_timer(0.02, self.process)  # 50 Hz
        self.get_logger().info('Bridge node started, using Wi‑Fi as primary channel')

    def process(self):
        # Drain queue, keep most recent message per source
        latest: Dict[str, Any] = {}
        while not self.queue.empty():
            source, data = self.queue.get_nowait()
            latest[source] = data
        # Choose source
        if 'wifi' in latest:
            chosen = 'wifi'
            payload = latest['wifi']
        elif 'bt' in latest:
            chosen = 'bt'
            payload = latest['bt']
        else:
            return
        self.active_source = chosen
        joy_msg = Joy()
        joy_msg.header.stamp = self.get_clock().now().to_msg()
        joy_msg.axes = payload.get('axes', [])
        joy_msg.buttons = payload.get('buttons', [])
        self.publisher_.publish(joy_msg)
        self.get_logger().debug(f'Published Joy from {chosen}')

    def destroy_node(self):
        self.wifi_receiver.stop()
        self.bt_receiver.stop()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = BridgeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
