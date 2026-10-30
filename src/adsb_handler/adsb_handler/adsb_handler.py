import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node

import socket
import time
import threading

from interfaces.msg import Adsb

HOST = '127.0.0.1'
PORT = 30003

class AdsbHandler(Node):

    def __init__(self):
        super().__init__('adsb_handler')

        self.publisher_ = self.create_publisher(Adsb, 'adsb_cmd', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

        # timer for scanning available nodes
        self.plane_discovery_timer = self.create_timer(1, self.plane_discovery_timer_callback)


        # master registry dictionary (key: node_name, value: {state, icao})
        self.plane_pool = {}
        # last known active plane nodes (same as the plane_pool) but as a set for easy diffing
        self.last_active_plane_nodes = set()
        # start listening to tcp port for data
        self.stream_thread = threading.Thread(target=self.stream_dump1090, daemon=True)
        self.stream_thread.start()


    def stream_dump1090(self):
        while True:
            self.get_logger().info(f"Connecting to {HOST}:{PORT}...")
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.connect((HOST, PORT))
                    self.get_logger().info("Connected! Listening for flight data...")
                    while True:
                        data = s.recv(1024)
                        if not data:
                            break
                        # precessing
                        self.get_logger().info(f"received: {data}")
                        
            except ConnectionRefusedError:
                self.get_logger().error("Connection refused. Make sure dump1090 is running with the --net flag.")
            # wait some time, then try reconnect.
            time.sleep(2.0)


    def timer_callback(self):
        msg = Adsb()
        msg.node_name = 'plane_%d' % self.i
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing to plane node "%s"' % msg.node_name)
        self.i += 1

    def plane_discovery_timer_callback(self):
        # get current plane nodes
        current_plane_nodes = set(
            name for name in self.get_node_names() 
            if name.lstrip('/').startswith("plane_")
        )

        joined_nodes = current_plane_nodes - self.last_active_plane_nodes
        left_nodes = self.last_active_plane_nodes - current_plane_nodes

        # handle newly discovered plane nodes (add to pool as 'sleep')
        for node in joined_nodes:
            self.plane_pool[node] = {
                "state": "sleeping", 
                "icao": None
            }
            self.get_logger().info(f"Node joined pool: {node}")

        # handle crashed/dead plane nodes (need reassigment if holding icao adress)
        for node in left_nodes:
            self.get_logger().warn(f"Node crashed/left: {node}")
            
            # check if this dead node was tracking a real-world plane
            if node in self.plane_pool and self.plane_pool[node]["state"] == "active":
                abandoned_icao = self.plane_pool[node]["icao"]
                self.get_logger().warn(f"Plane {abandoned_icao} lost! Reassigning...")
                
                # find an available sleeping node to rescue this real-world plane
                new_node = self.assign_flight_to_node(abandoned_icao)
                if new_node:
                    self.get_logger().info(f"Successfully rescued flight {abandoned_icao} on new node {new_node}")

            # remove from master pool dict
            del self.plane_pool[node]

        # update history tracker
        self.last_active_plane_nodes = current_plane_nodes

    def assign_flight_to_node(self, icao_code):
        # find the first available sleeping node
        for node_name, info in self.plane_pool.items():
            if info["state"] == "sleeping":
                info["state"] = "active"
                info["icao"] = icao_code
                self.get_logger().info(f"Assigned live flight {icao_code} to plane node {node_name}")
                return node_name
        
        self.get_logger().error("No available sleeping plane nodes left to handle incoming ADS-B data!")
        self.get_logger().error(f"Currently active plane nodes: {len(self.plane_pool)}")
        return None


def main(args=None):
    try:
        with rclpy.init(args=args):
            node = AdsbHandler()
            rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass


if __name__ == '__main__':
    main()