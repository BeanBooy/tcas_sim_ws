import os
os.environ["LIBGL_ALWAYS_SOFTWARE"] = "1"  # removes big warning log at startup (thank you mr gemini)
import sys
import signal
import threading
import rclpy
from PyQt5.QtWidgets import QApplication

from .monitor import FlightMonitorNode, MainWindow

def run_ros_spin(node):
    try:
        rclpy.spin(node)
    except rclpy.executors.ExternalShutdownException:
        pass

def main():
    rclpy.init()
    # second thread for ros spin
    node = FlightMonitorNode()
    ros_thread = threading.Thread(target=run_ros_spin, args=(node,), daemon=True)
    ros_thread.start()
    # start qt app in current thread
    app = QApplication(sys.argv)
    # causes instant shutdown on ctrl+c
    signal.signal(signal.SIGINT, lambda sig, frame: app.quit())
    window = MainWindow()
    window.show()
    app.exec_()
    # cleanup
    rclpy.shutdown()
    ros_thread.join(timeout=1.0)
    node.destroy_node()

if __name__ == '__main__':
    main()
