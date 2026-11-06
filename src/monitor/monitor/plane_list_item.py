from ament_index_python.packages import get_package_share_directory
from PyQt5.QtWidgets import QHBoxLayout, QCheckBox, QPushButton, QFrame, QLabel
from PyQt5.QtCore import Qt, pyqtSignal, QTimer
from PyQt5.QtGui import QIcon
from pathlib import Path
import time

assets_dir = Path(get_package_share_directory('monitor')) / "assets"

class PlaneListItem(QFrame):

    toggled = pyqtSignal(str, bool)
    deleted = pyqtSignal(str)

    def __init__(self, icao_hex, color_hex="#FFFFFF", *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.icao_hex = icao_hex
        self.color_hex = color_hex
        self.is_selected = False
        self.last_update_time = time.time()

        self.setAttribute(Qt.WA_StyledBackground, True)
        self.setCursor(Qt.PointingHandCursor)
        self.setProperty("selected", "false")

        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(10)

        # colored recktangle
        self.color_rect = QFrame(self)
        self.color_rect.setFixedSize(10, 20)
        self.color_rect.setStyleSheet(f"background-color: {self.color_hex}; border: none;")
        layout.addWidget(self.color_rect)

        # icao label
        self.icao_label = QLabel(icao_hex, self)
        self.icao_label.setStyleSheet("border: none; background: transparent; color: white; font-weight: bold;")
        layout.addWidget(self.icao_label)

        layout.addStretch()

        # time lable since last update
        self.time_label = QLabel("0s", self)
        self.time_label.setStyleSheet("border: none; background: transparent; color: white;")
        layout.addWidget(self.time_label)

        # delete button
        self.del_button = QPushButton(self)
        self.del_button.setIcon(QIcon(f"{assets_dir}/delete.svg"))
        self.del_button.setFixedSize(20, 20)
        self.del_button.setCursor(Qt.PointingHandCursor)
        self.del_button.setStyleSheet("""
            QPushButton {
                border: none;
                background-color: transparent;
            }
            QPushButton::hover {
                background-color: red;
            }
        """)
        layout.addWidget(self.del_button)
        self.del_button.setVisible(False)

        self.setLayout(layout)

        self.setStyleSheet("""
            PlaneListItem {
                background-color: transparent;
                border: none;
            }
            PlaneListItem:hover {
                background-color: #222;
            }
            PlaneListItem[selected="true"] {
                background-color: #383838;
            }
            PlaneListItem[selected="true"]:hover {
                background-color: #444;
            }
        """)

        # timer for last update calculation
        self.time_timer = QTimer(self)
        self.time_timer.timeout.connect(self._check_time)
        self.time_timer.start(1000)

        self.del_button.clicked.connect(self._on_delete_clicked)

    def _check_time(self):
        elapsed = time.time() - self.last_update_time
        self.time_label.setText(f"{elapsed:.2f}s")
        # change color
        if elapsed > 15:
            self.time_label.setStyleSheet("border: none; background: transparent; color: red;")
        elif elapsed > 8:
            self.time_label.setStyleSheet("border: none; background: transparent; color: yellow;")
        else:
            self.time_label.setStyleSheet("border: none; background: transparent; color: white;")

    def _on_delete_clicked(self):
        self.deleted.emit(self.icao_hex)

    def enterEvent(self, event):
        self.del_button.setVisible(True)
        super().enterEvent(event)

    def leaveEvent(self, event):
        self.del_button.setVisible(False)
        super().leaveEvent(event)

    def mousePressEvent(self, event):
        clicked_widget = self.childAt(event.pos())
        if clicked_widget != self.del_button:
            # toggle selected state
            self.is_selected = not self.is_selected
            # update css property and redraw widget
            self.setProperty("selected", "true" if self.is_selected else "false")
            self.style().unpolish(self)
            self.style().polish(self)
            # send update
            self.toggled.emit(self.icao_hex, self.is_selected)
        super().mousePressEvent(event)
