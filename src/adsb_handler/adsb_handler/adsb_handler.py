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

        self.publisher_ = self.create_publisher(Adsb, '/adsb_cmd', 10)
        # timer_period = 0.5  # seconds
        # self.timer = self.create_timer(timer_period, self.timer_callback)
        # self.i = 0

        # timer for scanning available nodes
        self.plane_discovery_timer = self.create_timer(1, self.plane_discovery_timer_callback)


        # master registry dictionary (key: node_name, value: {state, icao})
        self.plane_pool = {}
        # last known active plane nodes (same as the plane_pool) but as a set for easy diffing
        self.last_active_plane_nodes = set()
        # start listening to tcp port for data
        self.stream_thread = threading.Thread(target=self.stream_flight_data, daemon=True)
        self.stream_thread.start()


    def stream_flight_data(self):
        self.get_logger().info(f"Connecting to {HOST}:{PORT}...")
        while True:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.connect((HOST, PORT))
                    self.get_logger().info("Connected! Listening for flight data...")
                    buffer = ""
                    while True:
                        chunk = s.recv(1024)
                        if not chunk:
                            break
                        buffer += chunk.decode('utf-8', errors='ignore')
                        lines = buffer.split('\n')
                        # keep the last (possible incomplete) line in buffer
                        buffer = lines.pop()
                        # clean up the data
                        for line in lines:
                            line = line.strip()
                            if line:
                                self.process_flight_data(line)
                        
            except ConnectionRefusedError:
                self.get_logger().error("Connection refused! Check ADS-B receiver config.")
            # wait some time, then try reconnect
            time.sleep(2.0)

    def process_flight_data(self, data):
        #self.get_logger().info(f"received: {data}")

        fields = data.split(',')

        # example messages from console:
        #           icao         alt  GS Track  lat      lon   var squawk
        # 'MSG,8,,,407F38,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
        # 'MSG,5,,,440669,,,,,,,36000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
        # 'MSG,8,,,440669,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
        # 'MSG,4,,,440669,,,,,,,     ,474,288,        ,        ,320,    ,0,0,0,0\n'
        # 'MSG,5,,,407F38,,,,,,,38975,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
        # 'MSG,4,,,407F38,,,,,,,     ,454,136,        ,        ,64 ,    ,0,0,0,0\n'
        # 'MSG,5,,,407F38,,,,,,,38975,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
        # 'MSG,3,,,407F38,,,,,,,38975,   ,   ,48.59436,11.01562,   ,    ,0,0,0,0\n'
        # 'MSG,3,,,440669,,,,,,,36000,   ,   ,48.98427,11.76137,   ,    ,0,0,0,0\n'
        # 'MSG,8,,,39E688,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
        # 'MSG,3,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
        # 'MSG,4,,,782158,,,,,,,     ,535,186,        ,        ,-64,    ,0,0,0,0\n'
        # 'MSG,6,,,406E10,,,,,,,     ,   ,   ,        ,        ,   ,3106,0,0,0,0\n'
        # 'MSG,5,,,440669,,,,,,,36000,   ,   ,        ,        ,   ,    , , , , \n'
        # 'MSG,3,,,39E688,,,,,,,20800,   ,   ,48.85210,11.19500,   ,    ,0,0,0,0\n'
        # 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    , , , , \n'
        # 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
        # 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'

        # only care about transmission messages (MSG) 2, 3 and 4
        if fields[0] == 'MSG' and fields[1] in ['2', '3', '4']:
            self.get_logger().info(f"received XXXX")
            icao = fields[4].upper()  # 24 bit icao hex code
            
            if not icao:
                return

            plane_node = self.get_plane_node_from_icao(icao)
            if plane_node is None:
                plane_node = self.assign_flight_to_node(icao)
                if plane_node is None:
                    return  # no free plane node found...

            adsb_msg = Adsb()
            adsb_msg.node_name = plane_node
            adsb_msg.icao = icao
            if fields[11] != "":
                adsb_msg.altitude = int(fields[11])
                adsb_msg.altitude_valid = True
            if fields[12] != "":
                adsb_msg.ground_speed = int(fields[12])
                adsb_msg.ground_speed_valid = True
            if fields[13] != "":
                adsb_msg.track = int(fields[13])
                adsb_msg.track_valid = True
            if fields[14] != "" and fields[15] != "":
                adsb_msg.latitude = float(fields[14])
                adsb_msg.longitude = float(fields[15])
                adsb_msg.coords_valid = True
            if fields[16] != "":
                adsb_msg.vertical_speed = int(fields[16])
                adsb_msg.vertical_speed_valid = True

            self.publisher_.publish(adsb_msg)
            self.get_logger().info(f"published to: {plane_node}")

    # def timer_callback(self):
    #     msg = Adsb()
    #     msg.node_name = 'plane_%d' % self.i
    #     self.publisher_.publish(msg)
    #     self.get_logger().info('Publishing to plane node "%s"' % msg.node_name)
    #     self.i += 1

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
        
        self.get_logger().error(f"Can't assign icao {icao_code}, too few nodes!")
        self.get_logger().error(f"Currently active plane nodes: {len(self.plane_pool)}")
        return None

    def get_plane_node_from_icao(self, icao_code):
        for node_name, info in self.plane_pool.items():
            if info["state"] == "active" and info["icao"] == icao_code:
                return node_name
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