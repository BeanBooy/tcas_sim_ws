import sys
import threading
import rclpy
import signal
from rclpy.node import Node
from interfaces.msg import Pose

from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout
from PyQt5.QtCore import QTimer
import pyqtgraph as pg

# global dict for flight paths
flight_tracks = {}
tracks_lock = threading.Lock()

class FlightMonitorNode(Node):
    def __init__(self):
        super().__init__('pyqt_flight_monitor')
        self.subscription = self.create_subscription(
            Pose,
            '/airspace_broadcast',
            self.listener_callback,
            10
        )

    def listener_callback(self, msg):
        icao = msg.icao_hex
        lat = msg.latitude
        lon = msg.longitude

        with tracks_lock:
            if icao not in flight_tracks:
                flight_tracks[icao] = {"lats": [], "lons": []}
            
            flight_tracks[icao]["lats"].append(lat)
            flight_tracks[icao]["lons"].append(lon)
            
            # limit size to prevent unlimited growing
            if len(flight_tracks[icao]["lats"]) > 200:
                flight_tracks[icao]["lats"].pop(0)
                flight_tracks[icao]["lons"].pop(0)


class MainWindow(QMainWindow):
    def __init__(self, node):
        super().__init__()
        self.node = node
        self.setWindowTitle("Airspace Monitor")
        self.resize(900, 700)

        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        layout = QVBoxLayout(self.central_widget)

        # pyqtgraph
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setLabel('left', 'Latitude (Grad)')
        self.plot_widget.setLabel('bottom', 'Longitude (Grad)')
        self.plot_widget.showGrid(x=True, y=True)
        # mouse controlls
        view_box = self.plot_widget.getPlotItem().getViewBox()
        view_box.setMouseMode(view_box.PanMode)
                
        layout.addWidget(self.plot_widget)

        # dics for each flight graph
        self.curves = {}

        self.color_palette = [
            'r', 'g', 'b', 'c', 'm', 'y', 
            'orange', 'purple', 'deeppink', 'lime', 'dodgerblue'
        ]

        # timer for gui update
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_loop)
        self.timer.start(100)

        # ingolstadt for reverence
        self.plot_widget.plot(
            [11.4333],
            [48.7667],
            pen=None,
            symbol='o',
            symbolSize=12,
            symbolBrush='b'
        )

    def update_loop(self):
        try:
            # non-blocking ros tick (fetches new messages)
            rclpy.spin_once(self.node, timeout_sec=0)
        except Exception:
            pass
        
        with tracks_lock:
            for icao, track in flight_tracks.items():
                lons = track["lons"]
                lats = track["lats"]

                if icao not in self.curves:

                    color_index = len(self.curves) % len(self.color_palette)
                    chosen_color = self.color_palette[color_index]

                    # new graph for new plane
                    curve = self.plot_widget.plot(
                        lons, lats,
                        pen=pg.mkPen(color=chosen_color, width=2),
                        symbol='o',
                        symbolSize=2,
                        symbolBrush=chosen_color,
                        name=icao
                    )
                    self.curves[icao] = curve
                else:
                    # update graph
                    self.curves[icao].setData(lons, lats)

def main():
    rclpy.init()
    node = FlightMonitorNode()

    app = QApplication(sys.argv)
    
    # Sorgt dafür, dass Ctrl+C die Qt-App sofort beendet statt abzustürzen
    signal.signal(signal.SIGINT, lambda sig, frame: app.quit())

    window = MainWindow(node)
    window.show()

    app.exec_()

    # Sauberes Aufräumen nach dem Schließen
    node.destroy_node()
    if rclpy.ok():
        rclpy.shutdown()


if __name__ == '__main__':
    main()
