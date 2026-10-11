
JOG_SPEED = "50"
JOG_ACCEL = "50"


######### robot config
LANGUAGE = 'en'
LANGUAGE_DATA =None 

NAME_JOINTS = ["J1","J2","J3","J4","J5","J6", "TOOL"]
UNIT_JOINT = ["deg","deg","deg","deg","deg","deg"]

NAME_AXIS = ["X","Y","Z","y","p","r"]
UNIT_AXIS = ["mm","mm","mm","deg","deg","deg"]

ROBOT_NAME = None
SELECTED_ROBOT = None
ROBOT_SETTINGS = None

# Position of the Simulation
POS_JOINT_SIM = None
POS_AXIS_SIM = None

# Position of the robot
POS_AXIS = None
POS_JOINT = None

DH_PARAM = None
NUMBER_OF_JOINTS = None
EXTRA_JOINT = None

# Wrist roll of the ARM: a servo outside the kinematics that turns the tool about its own axis
WRIST_ROLL_LIMITS = (-90, 90)   # deg, same as the firmware
WRIST_ROLL = 0.0                # deg, last roll sent to the robot
WRIST_ROLL_SIM = 0.0            # deg, shown in the simulation
WRIST_ROLL_AXIS = "x"           # axis of the TOOL frame the tool turns about

# Wrist pitch of the ARM: gripper angle to the horizontal (0 = level, + = up), part of the kinematics
WRIST_PITCH_LIMITS = (-90, 90)  # deg, same as the firmware
WRIST_PITCH = 0.0               # deg, last pitch sent to the robot
WRIST_PITCH_SIM = 0.0           # deg, used by the simulation and its kinematics

MAX_JOINT_SPEED = None
MAX_JOINT_MOVE = None

TOOL = None
TOOL_NAME = None
TOOL_POS = None
TOOL_SETTINGS = None
TOOL_FRAME = None
TOOL_CAM_OFFSET = None
TOOL_CAM_SETTINGS = None
TOOL_TYPE = None
TOOL_SERVO_SETTINGS = None
TOOL_PIN = None