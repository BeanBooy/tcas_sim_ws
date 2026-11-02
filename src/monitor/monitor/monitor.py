import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node

from interfaces.msg import Pose

class Monitor(Node):

    def __init__(self):
        super().__init__('monitor')
        
        self.subscription = self.create_subscription(
            Pose,
            '/airspace_broadcast',
            self.listener_callback,
            10)

    def listener_callback(self, msg):
        #self.get_logger().info(f"I heard something from {msg.icao_hex}")
        return


def main(args=None):
    try:
        with rclpy.init(args=args):
            node = Monitor()
            rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass


if __name__ == '__main__':
    main()