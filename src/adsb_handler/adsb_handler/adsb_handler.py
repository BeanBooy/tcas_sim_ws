import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node

import socket
import time
import threading
import time

from interfaces.msg import Adsb

class AdsbHandler(Node):

    def __init__(self):
        super().__init__('adsb_handler')
        self.declare_parameter("host", "127.0.0.1")
        self.HOST = self.get_parameter("host").get_parameter_value().string_value
        self.declare_parameter("port", "30003")
        self.PORT = int(self.get_parameter("port").get_parameter_value().string_value)
        # topic for adsb command messages
        self.publisher_ = self.create_publisher(Adsb, "/adsb_cmd", 10)
        # timer for scanning available nodes and cleanup current flights
        self.plane_discovery_timer = self.create_timer(1, self.plane_discovery_timer_callback)
        # master registry dictionary (key: node_name, value: {state, icao})
        self.plane_pool = {}
        # last known active plane nodes (same as the plane_pool) but as a set for easy diffing
        self.last_active_plane_nodes = set()
        # start listening to tcp port for data
        self.stream_thread = threading.Thread(target=self.stream_flight_data, daemon=True)
        self.stream_thread.start()

    def stream_flight_data(self):
        self.get_logger().info(f"Connecting to {self.HOST}:{self.PORT}...")
        while True:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.connect((self.HOST, self.PORT))
                    self.get_logger().info("Connected! Listening for flight data...")
                    buffer = ""
                    while True:
                        chunk = s.recv(1024)
                        if not chunk:
                            break
                        buffer += chunk.decode("utf-8", errors="ignore")
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
        fields = data.split(',')
        # only care about transmission messages (MSG) 2, 3 and 4
        if fields[0] == "MSG" and fields[1] in ['2', '3', '4']:
            icao = fields[4].upper()  # 24 bit icao hex code
            # should never be null, but reconform since whole logic builds on the icao hex
            if not icao:
                return
            # find or initialize the plane node which handles this flight
            plane_node = self.get_plane_node_from_icao(icao)
            if plane_node is None:
                plane_node = self.assign_flight_to_node(icao)
                if plane_node is None:
                    return  # no free plane node found... displays error
            # send message
            self.send_adsb_command(plane_node, icao, 
                int(fields[11]) if fields[11] != "" else None,  # altitude
                int(fields[12]) if fields[12] != "" else None,  # ground speed
                int(fields[13]) if fields[13] != "" else None,  # track
                float(fields[14]) if fields[14] != "" else None,  # latitude
                float(fields[15]) if fields[15] != "" else None,  # longitude
                int(fields[16]) if fields[16] != "" else None)  # vertical speed

    def send_adsb_command(self, plane_node, icao, alt, gs, track, lat, lon, vs):
        adsb_msg = Adsb()
        adsb_msg.node_name = plane_node
        adsb_msg.icao = icao
        if alt:
            adsb_msg.altitude = alt
            adsb_msg.altitude_valid = True
        if gs:
            adsb_msg.ground_speed = gs
            adsb_msg.ground_speed_valid = True
        if track:
            adsb_msg.track = track
            adsb_msg.track_valid = True
        if lat and lon:
            adsb_msg.latitude = lat
            adsb_msg.longitude = lon
            adsb_msg.coords_valid = True
        if vs:
            adsb_msg.vertical_speed = vs
            adsb_msg.vertical_speed_valid = True
        # send new data to plane node
        self.publisher_.publish(adsb_msg)
        # setting update time to keep track of flight validy
        self.plane_pool[plane_node]["last_update"] = time.time()

    def plane_discovery_timer_callback(self):
        # unasign plane nodes which haven't received updates in a while (probably because out of range)
        current_time = time.time()
        for node_name, info in self.plane_pool.items():
            if info["state"] == "active" and current_time - info["last_update"] > 10.0:  # seconds
                info["state"] = "free"
                self.get_logger().warn(f"Node {node_name} (flight {info["icao"]}) was released! (hasn't received updates in reasonable abount of time)")
        # get current plane nodes from ros graph
        current_plane_nodes = set(
            name for name in self.get_node_names() 
            if name.lstrip('/').startswith("plane_")
        )
        joined_nodes = current_plane_nodes - self.last_active_plane_nodes
        left_nodes = self.last_active_plane_nodes - current_plane_nodes
        # handle newly discovered plane nodes
        for node in joined_nodes:
            self.plane_pool[node] = { "state": "free" }
            self.get_logger().info(f"Node joined pool: {node}")
        # handle crashed/dead plane nodes (need reassigment if holding icao adress)
        for node in left_nodes:
            self.get_logger().warn(f"Node crashed/left: {node}")
            # check if this dead node was tracking a real-world plane
            if node in self.plane_pool and self.plane_pool[node]["state"] == "active":
                abandoned_icao = self.plane_pool[node]["icao"]
                self.get_logger().warn(f"Node {node} connection lost! Reassigning active flight {abandoned_icao}...")
                # find an free node to rescue this real-world plane
                new_node = self.assign_flight_to_node(abandoned_icao)
                if new_node:
                    self.get_logger().info(f"Successfully rescued flight {abandoned_icao} on new node {new_node}")
            # remove from master pool dict
            del self.plane_pool[node]
        # update history tracker
        self.last_active_plane_nodes = current_plane_nodes

    def assign_flight_to_node(self, icao_code):
        # find the first free node
        for node_name, info in self.plane_pool.items():
            if info["state"] == "free":
                info["state"] = "active"
                info["icao"] = icao_code
                self.get_logger().info(f"Assigned flight {icao_code} to node {node_name}")
                return node_name
        # if no free node is availabe
        self.get_logger().error(f"Can't assign icao {icao_code}, too few nodes! ({len(self.plane_pool)})")
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