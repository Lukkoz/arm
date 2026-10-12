from PyQt5 import QtWidgets, QtCore
from PyQt5.QtGui import QCursor, QPixmap, QIcon, QDoubleValidator, QSyntaxHighlighter, QTextCharFormat, QColor, QTextCursor, QFont, QDesktopServices
from PyQt5.QtCore import Qt, QRegularExpression, QUrl, pyqtSignal, QTimer
from PyQt5.QtWidgets import QButtonGroup, QRadioButton, QPushButton, QCheckBox, QVBoxLayout, QSizePolicy, QSpacerItem, QFileDialog, QFrame, QGridLayout, QTextEdit, QScrollBar, QLineEdit, QWidget, QMessageBox

from backend.file_managment.file_management import FileManagement

from gui.windows.simulation_objects_gui import SimulationObjectsGUI
from gui.windows.simulation_origin_gui import SimulationOriginGUI
from gui.windows.connect_device import ConnectDevice
from gui.windows.vision import VisionWindow

import webbrowser

from gui.style import *



from backend.core.event_manager import event_manager
import backend.core.variables as var

from backend.file_manager import save_file
from backend.file_manager import save_as_file
from backend.file_manager import open_file
from backend.file_manager import new_file

from backend.robot_management.communication import connect_robot, connect_robot_check
from backend.robot_management.communication import connect_io, connect_io_check
from backend.robot_management.communication import stop_robot, play_robot, pauze_robot, leave_stop_mode, robot_paused

from backend.simulation import enable_simulation

from backend.run_program import run_script
from backend.run_program import stop_script
from backend.run_program import run_single_line

from backend.vision import connect_cam, cam_connected

from backend.file_managment import get_image_path


class MenuField(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet(style_frame)
        self.layout = QGridLayout(self)
        self.setStyleSheet(style_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(0)
        
        self.simulation_origin_gui = SimulationOriginGUI()
        self.simulation_objects_gui = SimulationObjectsGUI()
        self.connect_robot_window = ConnectDevice("ROBOT")
        self.connect_io_window = ConnectDevice("IO")
        self.connect_cam_window = ConnectDevice("CAMERA")
        self.vision_window = VisionWindow()

        frame = QFrame()
        layout = QVBoxLayout()
        layout.setSpacing(5)
        layout.setContentsMargins(10, 10, 10, 10)
        frame.setLayout(layout)
        self.layout.addWidget(frame)
        self.FrameButtons(layout)  

        space_widget = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)
        self.layout.addItem(space_widget)      

        frame = QFrame()
        layout = QVBoxLayout()
        layout.setSpacing(5)
        layout.setContentsMargins(10, 10, 10, 10)
        frame.setLayout(layout)
        self.layout.addWidget(frame) 
        self.SimButtons(layout)

        space_widget = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)
        self.layout.addItem(space_widget)   

        frame = QFrame()
        layout = QGridLayout()
        layout.setSpacing(5)
        layout.setContentsMargins(10, 10, 10, 10)
        frame.setLayout(layout)
        self.layout.addWidget(frame) 
        self.RobotButtons(layout)
        
        self.subscribeToEvents()

        self.setLayout(self.layout)
    
    def subscribeToEvents(self):
        event_manager.subscribe("request_robot_connect_button_color", self.ButtonConnectRobotColor)
        event_manager.subscribe("request_io_connect_button_color", self.ButtonConnectIOColor)
        event_manager.subscribe("request_robot_home_button_color", self.ButtonHomeRobotColor)
        event_manager.subscribe("request_cam_connect_button_color", self.ButtonConnectCamColor)
        event_manager.subscribe("request_stop_sim", self.ButtonStopSim)
        event_manager.subscribe("request_disable_robot_button", self.ButtonConnectRobotDisable)
        event_manager.subscribe("request_disable_io_button", self.ButtonConnectIODisable)
        event_manager.subscribe("request_disable_cam_button", self.ButtonConnectCamDisable)
        
    def FrameButtons(self, layout):
        image_path = get_image_path('studio.png')
        
        button = QPushButton()
        button.setFixedSize(130,130)
        button.setIcon(QIcon(image_path))
        button.clicked.connect(lambda: QDesktopServices.openUrl(QUrl("https://www.mikobots.com")))
        button.setStyleSheet("border-radius: 15px;")
        button.setIconSize(button.size())
        layout.addWidget(button)
        
        button = QPushButton("New file")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.pressed.connect(lambda: new_file())
        layout.addWidget(button)

        button = QPushButton("Save")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.pressed.connect(lambda: save_file())
        layout.addWidget(button)

        button = QPushButton("Save as")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.pressed.connect(lambda: save_as_file())
        layout.addWidget(button)

        button = QPushButton("Open")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.pressed.connect(lambda: open_file())
        layout.addWidget(button)
  
   
   # simulation buttons
    def SimButtons(self, layout):
        title = QtWidgets.QLabel("Mode")
        title.setStyleSheet(style_label_title)
        title.setAlignment(QtCore.Qt.AlignCenter | QtCore.Qt.AlignVCenter)
        title.setFixedHeight(30)
        layout.addWidget(title)
        
        # one toggle for the target of programs and jogging: robot or simulation
        self.ModeToggle = QPushButton()
        self.ModeToggle.setCheckable(True)
        self.ModeToggle.setFixedSize(130, 50)
        self.ModeToggle.setStyleSheet(style_toggle_mode)
        self.ModeToggle.toggled.connect(self.EnableSimulation)
        self.ModeToggle.setToolTip("Click to switch between the robot and the simulation")
        self.UpdateModeText(False)
        layout.addWidget(self.ModeToggle)
        self.mode_switch_by_app = False
        # no robot connected at start: simulation (once the window is up, so the labels exist)
        QTimer.singleShot(0, lambda: self.SetMode(simulation=True))
        
        button = QPushButton("Add origin")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.clicked.connect(self.simulation_origin_gui.show)
        layout.addWidget(button) 
        
        button = QPushButton("Show / hide")
        button.setFixedSize(130, 25)
        button.setStyleSheet(style_button_menu)
        button.clicked.connect(lambda: self.ShowHide())
        layout.addWidget(button)  
   
    def ButtonStopSim(self):
        stop_script()   
   
    def EnableSimulation(self, simulation):
        if not simulation and not self.mode_switch_by_app and not connect_robot_check():
            if not self.AskSwitchToRobot():
                self.SetMode(simulation=True)        # stay in simulation
                return
        enable_simulation(2 if simulation else 0)    # 2 = Qt checked, as the old checkbox sent
        self.UpdateModeText(simulation)

    def AskSwitchToRobot(self):
        """Warning before ROBOT mode without a connected robot; True = switch anyway."""
        box = QMessageBox(self)
        box.setIcon(QMessageBox.Warning)
        box.setWindowTitle("No robot connected")
        box.setText("<b>No robot is connected.</b>")
        box.setInformativeText("In ROBOT mode programs, jogging and Home do nothing until a robot "
                               "is connected (Connect robot).\n\nSwitch to ROBOT anyway?")
        to_robot = box.addButton("Switch to ROBOT", QMessageBox.YesRole)
        stay = box.addButton("Stay in SIMULATION", QMessageBox.NoRole)
        for button, color in ((to_robot, "#2e7d32"), (stay, "#1565c0")):
            button.setMinimumSize(180, 40)
            button.setStyleSheet(style_button_dialog % color)
        box.setDefaultButton(stay)
        box.setEscapeButton(stay)
        box.exec_()
        return box.clickedButton() == to_robot

    # switch without asking: at start (simulation), on robot connect (robot) / disconnect (simulation)
    def SetMode(self, simulation):
        self.mode_switch_by_app = True
        if self.ModeToggle.isChecked() == simulation:
            self.EnableSimulation(simulation)
        else:
            self.ModeToggle.setChecked(simulation)   # calls EnableSimulation through toggled
        self.mode_switch_by_app = False

    def UpdateModeText(self, simulation):
        self.ModeToggle.setText("SIMULATION" if simulation else "ROBOT")
        
    def ShowHide(self):
        self.simulation_objects_gui.openevent()
        self.simulation_objects_gui.show()   
        self.simulation_objects_gui.raise_()
        
    # Buttons that connect the robot and other
   
    def RobotButtons(self, layout):
        title = QtWidgets.QLabel("Robot")
        title.setStyleSheet(style_label_title)
        title.setAlignment(QtCore.Qt.AlignCenter | QtCore.Qt.AlignVCenter)
        title.setFixedHeight(30)
        layout.addWidget(title, 0, 0, 1, 2)
        
        self.button_connect_robot = QPushButton("Connect robot")
        self.button_connect_robot.setFixedSize(130, 25)
        self.button_connect_robot.setStyleSheet(style_button_menu)
        self.button_connect_robot.pressed.connect(lambda: self.connect_robot())
        layout.addWidget(self.button_connect_robot, 1, 0, 1, 2)

        self.button_home_robot = QPushButton("Home")
        self.button_home_robot.setFixedSize(130, 25)
        self.button_home_robot.setStyleSheet(style_button_menu)
        self.button_home_robot.pressed.connect(lambda: run_single_line("robot.Home()"))
        layout.addWidget(self.button_home_robot, 2, 0, 1, 2)

        # one button: Pause holds the robot in the middle of a move, Resume continues it
        self.button_pause = QPushButton()
        self.button_pause.setFixedSize(130, 25)
        self.button_pause.setStyleSheet(style_button_menu)
        self.button_pause.pressed.connect(self.PauseOrResume)
        layout.addWidget(self.button_pause, 3, 0, 1, 2)
        self.paused_shown = None
        self.UpdatePauseButton()
        self.pause_timer = QTimer(self)              # follows the robot (also after STOP or Resume elsewhere)
        self.pause_timer.timeout.connect(self.UpdatePauseButton)
        self.pause_timer.start(200)

        image_path = get_image_path('stop.png')
        button_stop = QPushButton()
        button_stop.setFixedSize(130, 40)
        button_stop.setIcon(QIcon(image_path))
        button_stop.setStyleSheet(style_button_red)
        button_stop.setToolTip("Stop the program and the robot at once")
        button_stop.pressed.connect(lambda: self.stop_robot())
        layout.addWidget(button_stop, 4, 0, 1, 2)

    def stop_robot(self):
        stop_script()
        stop_robot()
        leave_stop_mode()       # jog, Home and Run work again right away

    def PauseOrResume(self):
        if robot_paused():
            play_robot()
        else:
            pauze_robot()
        self.UpdatePauseButton()

    def UpdatePauseButton(self):
        paused = bool(robot_paused())
        if paused == self.paused_shown:
            return
        self.paused_shown = paused
        self.button_pause.setText("  Resume" if paused else "  Pause")
        self.button_pause.setIcon(QIcon(get_image_path("play.png" if paused else "pauze.png")))
        

    def connect_robot(self):
        if connect_robot_check():
            connect_robot()
        else:
            self.connect_robot_window.empty_list()
            self.connect_robot_window.show()
            self.connect_robot_window.raise_()

# disable buttons
    def ButtonConnectRobotDisable(self, state):
        if state:
            self.button_connect_robot.setDisabled(True)
        else:
            self.button_connect_robot.setEnabled(True)

    # the IO robot and camera have no buttons in this version (ARM only)
    def ButtonConnectIODisable(self, state):
        pass
            
    def ButtonConnectCamDisable(self, state):
        pass
            
# change color buttons
    def ButtonConnectRobotColor(self, connect):
        if connect:
            self.button_connect_robot.setStyleSheet(style_button_pressed)
            self.connect_robot_window.close()
        else:
            self.button_connect_robot.setStyleSheet(style_button_menu)
        self.SetMode(simulation=not connect)         # robot connected: ROBOT, else SIMULATION

    def ButtonConnectIOColor(self, connect):
        if connect:
            self.connect_io_window.close()
            
    def ButtonHomeRobotColor(self, home):
        if home:
            self.button_home_robot.setStyleSheet(style_button_pressed)
        else:
            self.button_home_robot.setStyleSheet(style_button_menu)
            
    def ButtonConnectCamColor(self, connect):
        if connect:
            self.connect_cam_window.close()
      