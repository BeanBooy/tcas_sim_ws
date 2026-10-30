import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node

from interfaces.msg import Adsb


class AdsbHandler(Node):

    def __init__(self):
        super().__init__('adsb_handler')
        self.publisher_ = self.create_publisher(Adsb, 'adsb_cmd', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

        # timer for scanning available nodes
        self.plane_discovery_timer = self.create_timer(1, self.plane_discovery_timer_callback)

        # instead if using timer connect to adsb socket and push real data on update



    def timer_callback(self):
        msg = Adsb()
        msg.node_name = 'plane_%d' % self.i
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing to plane node "%s"' % msg.node_name)
        self.i += 1

    def plane_discovery_timer_callback(self):
        pass


def main(args=None):
    try:
        with rclpy.init(args=args):
            node = AdsbHandler()
            rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass


if __name__ == '__main__':
    main()