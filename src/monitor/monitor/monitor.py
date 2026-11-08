import threading
import rclpy
import time
import numpy as np
from rclpy.node import Node
from interfaces.msg import Pose
from PyQt5.QtWidgets import QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSplitter
from PyQt5.QtCore import QTimer, Qt
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore

from .plane_list_item import PlaneListItem
from .plane import Plane


# global dict for plane objects
flight_tracks = {}  # icao -> plane instance
tracks_lock = threading.Lock()

class FlightMonitorNode(Node):
    def __init__(self):
        super().__init__('monitor')
        self.subscription = self.create_subscription(
            Pose,
            '/airspace_broadcast',
            self.ab_callback,
            10
        )

    def ab_callback(self, msg):
        with tracks_lock:
            if msg.icao_hex not in flight_tracks:
                flight_tracks[msg.icao_hex] = Plane(msg.icao_hex)
            # add position
            flight_tracks[msg.icao_hex].add_position(msg.latitude, msg.longitude, msg.altitude, time.time())


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # load ui
        self.init_ui()
        # capture global start time for altitude graph
        self.global_start_time = time.time()
        # set colors for the flights
        self.color_palette = ['#ff4d4d', '#2ecc71', '#3498db', 
                              '#f1c40f', '#9b59b6', '#e67e22', 
                              '#1abc9c', '#e84393', '#00cec9']
        self.color_index = 0
        # memory for ui elements (widgets and curves)
        self.plane_items = {}  # icao -> PlaneListItem
        self.latlon_curves = {}  # icao -> pyqtgraph plot curve
        self.alt_curves = {}  # icao -> pyqtgraph plot curve
        # timer for ui updates
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_loop)
        self.timer.start(100)

    def update_loop(self):
        with tracks_lock:
            for icao, plane in flight_tracks.items():
                # if plane is new assign color and create list element
                if plane.color is None:
                    # assign color
                    plane.color = self.color_palette[self.color_index % len(self.color_palette)]
                    self.color_index += 1
                    # create sidebar list element
                    item = PlaneListItem(icao, color_hex=plane.color)
                    item.toggled.connect(self.on_plane_toggled)
                    item.deleted.connect(self.on_plane_deleted)
                    self.sidebar_layout.insertWidget(self.sidebar_layout.count() - 1, item)
                    self.plane_items[icao] = item

                # set update time for sidebar list element
                if icao in self.plane_items and plane.times:
                    self.plane_items[icao].last_update_time = plane.times[-1]

                # update lat/lon graph (always)
                point_sizes = [0] * (len(plane.lons) - 1) + [10]
                if icao not in self.latlon_curves and plane.lons:
                    self.latlon_curves[icao] = self.plot_latlon.plot(
                        plane.lons, plane.lats,
                        pen=pg.mkPen(color=plane.color, width=2),
                        symbol='o', symbolSize=point_sizes, symbolBrush=None, symbolPen=pg.mkPen(color=plane.color, width=2)
                    )
                elif icao in self.latlon_curves:
                    self.latlon_curves[icao].setData(plane.lons, plane.lats, symbolSize=point_sizes)

                # update alt curves (only if selected)
                if plane.selected:
                    rel_times = [t - self.global_start_time for t in plane.times] if plane.times else []
                    if icao not in self.alt_curves and plane.alts:
                        self.alt_curves[icao] = self.plot_alt.plot(
                            rel_times, plane.alts,
                            pen=pg.mkPen(color=plane.color, width=2)
                        )
                    elif icao in self.alt_curves:
                        self.alt_curves[icao].setData(rel_times, plane.alts)

    def on_plane_toggled(self, icao, checked):
        with tracks_lock:
            if icao in flight_tracks:
                flight_tracks[icao].selected = checked
                if not checked and icao in self.alt_curves:
                    self.plot_alt.removeItem(self.alt_curves[icao])
                    del self.alt_curves[icao]

    def on_plane_deleted(self, icao):
        with tracks_lock:
            # delete from global tracks
            if icao in flight_tracks:
                del flight_tracks[icao]
            # delete lat/lon curve
            if icao in self.latlon_curves:
                self.plot_latlon.removeItem(self.latlon_curves[icao])
                del self.latlon_curves[icao]
            # delete alt curve
            if icao in self.alt_curves:
                self.plot_alt.removeItem(self.alt_curves[icao])
                del self.alt_curves[icao]
            # delete list item from sidebar
            if icao in self.plane_items:
                item = self.plane_items[icao]
                self.sidebar_layout.removeWidget(item)
                item.deleteLater()
                del self.plane_items[icao]

    def init_ui(self):
        pg.setConfigOptions(antialias=True, useOpenGL=False) 
        self.setWindowTitle("Airspace Monitor")
        self.resize(900, 700)

        self.setStyleSheet("""
            QMainWindow {
                background-color: #000;
            }
            QSplitter::handle {
                background-color: white;
            }
            QSplitter::handle:horizontal {
                width: 4px;  /* Dicke der vertikalen Trennlinie */
                height: 4px;
            }
            QSplitter::handle:vertical {
                height: 4px; /* Dicke der horizontalen Trennlinie */
            }
        """)

        # central layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)

        main_splitter = QSplitter(Qt.Horizontal)  # sidebar and graphs

        # left sidebar
        self.sidebar_widget = QWidget()
        self.sidebar_layout = QVBoxLayout(self.sidebar_widget)
        self.sidebar_layout.setContentsMargins(6, 6, 6, 6)
        self.sidebar_widget.setStyleSheet("background-color: #000; border: None; color: white;")

        self.sidebar_label = QLabel("All Flights")
        self.sidebar_label.setStyleSheet("border: None; font-size: 18px; font-weight: bold; padding: 5px;")
        self.sidebar_layout.addWidget(self.sidebar_label)
        self.sidebar_layout.addStretch()

        vertical_splitter = QSplitter(Qt.Vertical)

        # upper graph (lat/lon)
        top_container = QWidget()
        top_layout = QVBoxLayout(top_container)
        top_layout.setContentsMargins(10, 10, 10, 10)
        top_layout.setSpacing(0)

        self.plot_latlon = pg.PlotWidget()
        self.plot_latlon.setLabel('left', 'Latitude (°)')
        self.plot_latlon.setLabel('bottom', 'Longitude (°)')
        self.plot_latlon.showGrid(x=True, y=True)
        self.plot_latlon.getPlotItem().getViewBox().setMouseMode(pg.ViewBox.PanMode)
        self.plot_latlon.getViewBox().setAspectLocked(True, ratio=1.0)
        top_layout.addWidget(self.plot_latlon)

        # bottom graph (alt)
        bottom_container = QWidget()
        bottom_layout = QVBoxLayout(bottom_container)
        bottom_layout.setContentsMargins(10, 10, 10, 10)
        bottom_layout.setSpacing(0)

        self.plot_alt = pg.PlotWidget()
        self.plot_alt.setLabel('left', 'Altitude (m)')
        self.plot_alt.setLabel('bottom', 'Time')
        self.plot_alt.showGrid(x=True, y=True)
        self.plot_alt.getPlotItem().getViewBox().setMouseMode(pg.ViewBox.PanMode)
        bottom_layout.addWidget(self.plot_alt)

        # vertical splitter
        vertical_splitter.addWidget(top_container)
        vertical_splitter.addWidget(bottom_container)
        vertical_splitter.setSizes([525, 175])

        # horizontal splitter
        main_splitter.addWidget(self.sidebar_widget)
        main_splitter.addWidget(vertical_splitter)
        main_splitter.setSizes([200, 700])

        # link layout
        main_layout.addWidget(main_splitter)

        # draw reference point (Ingolstadt)
        lat_center = 48.7667
        lon_center = 11.4333
        self.plot_latlon.plot([lon_center], [lat_center], pen=None, symbol='o', symbolSize=10, symbolBrush="#AAA")
        self.draw_circle(lat_center, lon_center, 10)
        self.draw_circle(lat_center, lon_center, 20)
        self.draw_circle(lat_center, lon_center, 30)


    def draw_circle(self, lat_center, lon_center, radius_km):
        # convert meters to degrees (approximate)
        d_lat = radius_km * 1000 / 111111.0
        d_lon = radius_km * 1000 / (111111.0 * np.cos(np.radians(lat_center)))
        # generate parametric circle points
        theta = np.linspace(0, 2 * np.pi, 200)
        circle_lats = lat_center + d_lat * np.sin(theta)
        circle_lons = lon_center + d_lon * np.cos(theta)
        self.plot_latlon.plot(circle_lons, circle_lats, pen=pg.mkPen(color="#AAA", width=1, style=QtCore.Qt.DotLine))
        # text
        text_item = pg.TextItem(text=f"{radius_km} km", color="#AAA")
        text_item.setAnchor((0.5, 0))
        text_item.setPos(lon_center, lat_center - d_lat)
        self.plot_latlon.addItem(text_item)

