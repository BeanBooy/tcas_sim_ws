import sys
import signal
import rclpy
from PyQt5.QtWidgets import QApplication

from .monitor import FlightMonitorNode, MainWindow

def main():
    rclpy.init()
    node = FlightMonitorNode()

    app = QApplication(sys.argv)
    
    # causes instant shutdown on ctrl+c
    signal.signal(signal.SIGINT, lambda sig, frame: app.quit())

    window = MainWindow(node)
    window.show()

    app.exec_()

    # cleanup
    node.destroy_node()
    if rclpy.ok():
        rclpy.shutdown()


if __name__ == '__main__':
    main()
