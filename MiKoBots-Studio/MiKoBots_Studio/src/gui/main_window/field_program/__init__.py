from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWebEngineWidgets import QWebEngineView, QWebEngineSettings
from PyQt5.QtGui import QCursor, QPixmap, QSyntaxHighlighter, QTextCharFormat,  QDesktopServices, QTextCursor, QFont, QIcon
from PyQt5.QtCore import Qt, QRegularExpression, QUrl, pyqtSignal, QStringListModel, pyqtSlot, QTimer
from PyQt5.QtWidgets import QCompleter, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QCheckBox, QComboBox, QSizePolicy, QApplication, QFileDialog, QFrame, QGridLayout, QTextEdit, QScrollBar, QLineEdit, QWidget

from backend.core.event_manager import event_manager
from gui.style import *
import os

from backend.run_program import run_blockly_code, run_script, stop_script, check_program_run
from backend.simulation import check_simulation_on
from backend.robot_management.communication import stop_robot, leave_stop_mode
from backend.file_managment import get_image_path

from backend.file_manager import blockly_save

import backend.core.variables as var

import json

from .highlight_text import HighlightText
from .text_edit import CustomTextEdit

# Shown in the program field at start: a servo test for the ARM, one servo at a time.
EXAMPLE_PROGRAM = '''# ARM servo test: moves every servo on its own and back.
# Works on the robot and in the simulation. Keep the arm clear while it runs.
#   J1 base, J2 shoulder, J3 elbow   (channels 0, 1, 2)
#   wrist roll                       (channel 3)  -90..90 deg, 0 = straight
#   wrist pitch                      (channel 5)  -90..90 deg to the horizontal, 0 = level, + = up
from robot_library import Move
import time

robot = Move()

v, a = 30, 30                  # speed and acceleration in %
SWING = [30, 20, 20]           # test angle of J1, J2, J3 in deg

robot.MoveJointPos([0, 0, 0], v, a)    # start position
robot.WristRoll(0, v, a)
robot.WristPitch(0, v, a)
time.sleep(1)

# J1, J2, J3 one after another: +swing, -swing, back to 0
# (the wrist keeps the gripper at its pitch while J2 and J3 move)
for joint in range(3):
    print(f"J{joint + 1}")
    for angle in (SWING[joint], -SWING[joint], 0):
        pos = [0, 0, 0]
        pos[joint] = angle
        robot.MoveJointPos(pos, v, a)
        time.sleep(0.5)

# wrist roll
print("wrist roll")
for angle in (45, -45, 0):
    robot.WristRoll(angle, v, a)
    time.sleep(0.5)

# wrist pitch: gripper up, down, level
print("wrist pitch")
for angle in (30, -30, 0):
    robot.WristPitch(angle, v, a)
    time.sleep(0.5)

print("servo test done")
'''

# Shown in the editor of the text mode: Kawasaki AS and G-code, see PROGRAMMING.md.
EXAMPLE_TEXT_PROGRAM = '''; Text program: Kawasaki AS and G-code (mm, %, s). Home the robot first.
; Kawasaki AS
HOME
SPEED 30
POINT pick = TRANS(200, 0, 250)
JMOVE pick
DRAW 0, 0, -40
TWAIT 1
DRAW 0, 0, 40
; G-code
G90
G1 X150 Y100 Z250 F3000
G4 P500
G28
'''

PROGRAM_MODES = [
    ("Python", "python"),
    ("Blockly", "blockly"),
    ("Text (Kawasaki AS / G-code)", "text"),
]

class ProgramField(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.layout = QGridLayout(self)
        self.setStyleSheet(style_widget)
        self.layout.setSpacing(5)

        self.font_size = 11
        self.FrameProgramming()
        
        self.subscribeToEvents()
        
    def subscribeToEvents(self):
        event_manager.subscribe("request_set_program_title", self.SetProgramName)
        event_manager.subscribe("request_program_field_clear", self.ProgramFieldClear)
        event_manager.subscribe("request_program_field_insert", self.ProgramFieldInsert)
        event_manager.subscribe("request_program_field_get", self.ProgramFieldGet)
        event_manager.subscribe("request_get_program_type", self.GetProgramTpe)
        event_manager.subscribe("request_set_program_type", self.SetProgramMode)
        event_manager.subscribe("request_text_program_get", self.TextProgramGet)
        event_manager.subscribe("request_text_program_set", self.TextProgramSet)
        event_manager.subscribe("request_get_blockly_code", self.GetBlocklyCode)
        event_manager.subscribe("request_save_blockly_file", self.save_workspace)
        event_manager.subscribe("request_load_blockly_file", self.load_workspace)
        event_manager.subscribe("request_clear_blockly_file", self.clear_workspace)



    def FrameProgramming(self): 
        self.PROGRAM_NAME = QLabel("Program: New file.miko")
        self.PROGRAM_NAME.setStyleSheet(style_label_title)
        self.PROGRAM_NAME.setAlignment(QtCore.Qt.AlignCenter | QtCore.Qt.AlignVCenter)
        self.PROGRAM_NAME.setFixedHeight(24)
        self.layout.addWidget(self.PROGRAM_NAME, 0, 0)
        
        # program type: Python and text programs share the editor, each keeps its own text
        self.mode = "python"
        self.texts = {"python": EXAMPLE_PROGRAM, "text": EXAMPLE_TEXT_PROGRAM}
        self.COMBO_MODE = QComboBox()
        self.COMBO_MODE.setStyleSheet(style_combo)
        self.COMBO_MODE.setFixedWidth(230)
        for label, mode in PROGRAM_MODES:
            self.COMBO_MODE.addItem(label, mode)
        self.COMBO_MODE.currentIndexChanged.connect(lambda index: self.SwitchProgramStle(self.COMBO_MODE.itemData(index)))

        # one Run button: runs on the robot or the simulation (the Mode toggle), Stop while running
        self.RUN_BUTTON = QPushButton()
        self.RUN_BUTTON.setFixedHeight(30)
        self.RUN_BUTTON.setMinimumWidth(190)
        self.RUN_BUTTON.setStyleSheet(style_button_run)
        self.RUN_BUTTON.clicked.connect(self.RunOrStop)

        mode_row = QHBoxLayout()
        mode_row.setContentsMargins(0, 0, 0, 0)
        mode_row.addWidget(self.COMBO_MODE)
        mode_row.addStretch(1)
        mode_row.addWidget(self.RUN_BUTTON)
        self.layout.addLayout(mode_row, 1, 0)

        self.run_state = None
        self.UpdateRunButton()
        self.run_timer = QTimer(self)                 # follows the program and the Mode toggle
        self.run_timer.timeout.connect(self.UpdateRunButton)
        self.run_timer.start(200)

        self.h_layout = QHBoxLayout()
        self.h_layout.setContentsMargins(0, 0, 1, 2)  # Set margins to zero
        self.h_layout.setSpacing(0)  # Set spacing to zer 

        self.frame = QWidget()
        self.frame.setStyleSheet(style_frame)
        self.frame.setLayout(self.h_layout)

        # Create line numbers text area (non-editable)
        self.line_numbers = QTextEdit()
        self.line_numbers.setReadOnly(True)
        self.line_numbers.setFrameStyle(QFrame.NoFrame)  # No frame for better aesthetics
        self.line_numbers.setFixedWidth(40)  # Fixed width for line numbers column
        self.line_numbers.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)  # Hide vertical scrollbar
        self.line_numbers.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)  # Hide horizontal scrollbar       
        self.line_numbers.setFont(QFont("Consolas", 11))
        self.line_numbers.setStyleSheet("""
            QTextEdit {
                background-color: white;
                padding: 5;
                border-top-right-radius: 0px;  
                border-bottom-right-radius: 0px; 
                border-bottom-left-radius: 5px;   
                border-top-left-radius: 5px;     
                color: black;
            }
            
        """)
        
        self.PROGRAM_TEXT_WIDGET = CustomTextEdit(self) 
        self.PROGRAM_TEXT_WIDGET.setTabStopWidth(20)
        self.PROGRAM_TEXT_WIDGET.setLineWrapMode(QTextEdit.NoWrap)
        self.PROGRAM_TEXT_WIDGET.verticalScrollBar().valueChanged.connect(self.sync_scrollbars)
        self.PROGRAM_TEXT_WIDGET.setFont(QFont("Consolas", 11))
        self.PROGRAM_TEXT_WIDGET.setStyleSheet(style_textedit_program)
        
        
        self.h_layout.addWidget(self.line_numbers)
        self.h_layout.addWidget(self.PROGRAM_TEXT_WIDGET)


        self.button_plus = QPushButton("+")
        self.button_plus.setFixedSize(30,30)
        self.button_plus.setStyleSheet(style_button_zoom)
        self.button_plus.clicked.connect(lambda: self.change_font_size(1))

        self.button_min = QPushButton("-")
        self.button_min.setFixedSize(30,30)
        self.button_min.setStyleSheet(style_button_zoom)
        self.button_min.clicked.connect(lambda: self.change_font_size(-1))

        button_layout = QVBoxLayout()
        button_layout.setContentsMargins(20, 20, 20, 20)  # Set padding around the button
        button_layout.addWidget(self.button_plus)
        button_layout.addWidget(self.button_min)

        self.web_view = QWebEngineView()
        self.web_view.settings().setAttribute(QWebEngineSettings.JavascriptEnabled, True)
        self.web_view.settings().setAttribute(QWebEngineSettings.PluginsEnabled, False)
        self.web_view.settings().setAttribute(QWebEngineSettings.WebGLEnabled, False)
        self.web_view.settings().setAttribute(QWebEngineSettings.FullScreenSupportEnabled, False)



        # Get the current directory of the file
        current_directory = os.path.dirname(__file__)
        blockly_path = os.path.abspath(os.path.join(current_directory, '../../..', 'blockly', 'blockly.html'))
        self.web_view.setUrl(QUrl.fromLocalFile(blockly_path))  # Make sure to set the correct path to your HTML file
        self.layout.addWidget(self.web_view, 2, 0)


        #self.blockly_frame.hide()
        self.layout.setRowStretch(1, 1)  # Set the stretch factor for row 0
        self.layout.setColumnStretch(0, 1)  # Set the stretch factor for column 0


        self.PROGRAM_TEXT_WIDGET.setPlainText(EXAMPLE_PROGRAM)
        
        self.layout.addWidget(self.frame, 2, 0)
        self.layout.addLayout(button_layout, 2, 0, alignment=Qt.AlignBottom | Qt.AlignRight)        
        
        self.PROGRAM_TEXT_WIDGET.textChanged.connect(self.update_line_numbers)
       
        self.highlighter = HighlightText(self.PROGRAM_TEXT_WIDGET.document())
        

        layout_question = QHBoxLayout()
        layout_question.setAlignment(Qt.AlignRight)
        self.layout.addLayout(layout_question,0,0, alignment=Qt.AlignTop | Qt.AlignRight)
        
        button = QPushButton("?")
        button.setStyleSheet(style_button_help)
        button.setFixedSize(20,20)
        button.clicked.connect(lambda: QDesktopServices.openUrl(QUrl("https://mikobots.com/mikobots-studio/help/program-field/")))
        layout_question.addWidget(button)

    def change_font_size(self, size):
        new_size = self.highlighter.font_size + size  # Increase font size by 2
        self.highlighter.update_font_size(new_size)

        
        self.line_numbers.setFontPointSize(new_size)

        self.PROGRAM_TEXT_WIDGET.selectAll()
        cursor = self.PROGRAM_TEXT_WIDGET.textCursor()
        char_format = cursor.charFormat()

        font = QFont(char_format.fontFamily(), new_size)
        char_format.setFont(font)

        cursor.mergeCharFormat(char_format)

        # Deselect the text
        cursor.movePosition(QTextCursor.Start)
        self.PROGRAM_TEXT_WIDGET.setTextCursor(cursor)

 
    def SwitchProgramStle(self, mode):
        if mode == self.mode:
            return
        if self.mode in self.texts:                       # keep the text of the editor mode we leave
            self.texts[self.mode] = self.PROGRAM_TEXT_WIDGET.toPlainText()
        self.mode = mode

        if mode == "blockly":
            self.frame.hide()
            self.button_min.hide()
            self.button_plus.hide()
        else:
            self.PROGRAM_TEXT_WIDGET.setPlainText(self.texts[mode])
            self.frame.show()
            self.button_min.show()
            self.button_plus.show()

    def UpdateRunButton(self):
        state = (check_program_run(), check_simulation_on())
        if state == self.run_state:
            return
        self.run_state = state
        running, simulation = state
        target = "simulation" if simulation else "robot"
        if running:
            self.RUN_BUTTON.setText("  Stop")
            self.RUN_BUTTON.setIcon(QIcon(get_image_path("stop.png")))
            self.RUN_BUTTON.setToolTip("Stop the program")
        else:
            self.RUN_BUTTON.setText(f"  Run on {target}")
            self.RUN_BUTTON.setIcon(QIcon(get_image_path("play.png")))
            self.RUN_BUTTON.setToolTip(f"Run the program on the {target} (switch with the Mode toggle)")
        self.RUN_BUTTON.setProperty("running", running)
        self.RUN_BUTTON.style().unpolish(self.RUN_BUTTON)
        self.RUN_BUTTON.style().polish(self.RUN_BUTTON)

    def RunOrStop(self):
        if check_program_run():
            stop_script()
            if not check_simulation_on():
                stop_robot()
                leave_stop_mode()   # like the red STOP: the next Run, jog or Home works right away
        else:
            run_script(check_simulation_on())
        self.UpdateRunButton()

    def SetProgramMode(self, mode):
        index = self.COMBO_MODE.findData(mode)
        self.COMBO_MODE.setCurrentIndex(index if index >= 0 else 0)

    # the text of one editor mode, wherever it is now (in the editor or kept aside)
    def GetModeText(self, mode):
        return self.PROGRAM_TEXT_WIDGET.toPlainText() if self.mode == mode else self.texts[mode]

    def SetModeText(self, mode, text):
        if self.mode == mode:
            self.PROGRAM_TEXT_WIDGET.setPlainText(text)
        else:
            self.texts[mode] = text

    def TextProgramGet(self):
        return self.GetModeText("text")

    def TextProgramSet(self, text):
        self.SetModeText("text", text)

    def update_line_numbers(self):
        # Get the number of lines in the text editor
        
        text = self.PROGRAM_TEXT_WIDGET.toPlainText()
        lines = text.split('\n')
        line_count = len(lines)

        # Generate line numbers
        line_number_text = "\n".join(str(i + 1) for i in range(line_count))

        # Set the line numbers text
        self.line_numbers.setText(line_number_text)

        # Sync vertical scrolling of the line numbers with the text editor
        self.line_numbers.verticalScrollBar().setValue(self.PROGRAM_TEXT_WIDGET.verticalScrollBar().value())

    def sync_scrollbars(self):
        # Synchronize the scrollbar values
        self.line_numbers.verticalScrollBar().setValue(self.PROGRAM_TEXT_WIDGET.verticalScrollBar().value())
    
    # the Python program, also while another mode is shown
    def ProgramFieldClear(self):
        self.SetModeText("python", "")

    def ProgramFieldInsert(self, text):
        if self.mode == "python":
            self.PROGRAM_TEXT_WIDGET.insertPlainText(text)
        else:
            self.texts["python"] += text

    def ProgramFieldGet(self):
        return self.GetModeText("python")
    
    def SetProgramName(self, name):
        self.PROGRAM_NAME.setText(f"Program: {name}")

    def GetProgramTpe(self):
        return self.mode            # "python", "blockly" or "text"
        
    def GetBlocklyCode(self):
        self.web_view.page().runJavaScript("python.pythonGenerator.workspaceToCode(workspace);", run_blockly_code)
        
    @pyqtSlot()
    def save_workspace(self):
        # Call the saveWorkspace function in JavaScript and handle the result
        try:
            self.web_view.page().runJavaScript("saveWorkspace();", self.BlocklyConverting)
        except Exception as e:
            print(f"Error running JavaScript: {e}")

    def BlocklyConverting(self, xmlString):
        blockly_save(xmlString)
        
        


    @pyqtSlot()
    def load_workspace(self, xmlString):
        try:
            # Escape xmlString and pass to JavaScript
            escaped_xmlString = json.dumps(xmlString)
            self.web_view.page().runJavaScript(f'loadWorkspace({escaped_xmlString});')
        except FileNotFoundError:
            print(var.LANGUAGE_DATA.get("message_no_blockly_workspace"))
            
            
    @pyqtSlot()
    def clear_workspace(self):
        # Run JavaScript to clear the Blockly workspace
        self.web_view.page().runJavaScript("workspace.clear();")  
        