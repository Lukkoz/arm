# Programming the ARM in MiKoBots Studio

The program field has three program types, chosen in the dropdown above it: **Python**,
**Blockly** and **Text (Kawasaki AS / G-code)**. Python and Text keep their own text when you
switch. The green **Run** button next to the dropdown runs the program on the robot or on the
3D model, depending on the **ROBOT / SIMULATION** toggle under *Mode*; while the program runs
the button turns red and stops it.

## The ARM

| Part | PCA9685 channel | Range | Command |
|---|---|---|---|
| J1 base | 0 | -90 … 90° | joint and position moves |
| J2 shoulder | 1 | -60 … 95° | joint and position moves |
| J3 elbow | 2 | -90 … 60° | joint and position moves |
| Wrist roll | 3 | -90 … 90°, 0 = gripper straight | `WristRoll` |
| Gripper (tool) | 4 | 0 … 100 % | `tool.MoveTo` (no tool fitted yet) |
| Wrist pitch | 5 | -90 … 90° to the horizontal, 0 = level, + = up | `WristPitch` |

- Positions are in **mm**: X forward, Y left, Z up, measured from the base.
- Speed `v` and acceleration `a` are in **%** of the robot's maximum (1–100).
- The ARM has 3 axes, so every position or joint list has **3 values**.
- The gripper keeps its pitch while the arm moves. `MoveJ` / `MoveL` take the pitch into
  account, so X Y Z is always the tip of the gripper. The roll does not change the position.

## Python

A Python program starts with the imports it needs:

```python
from robot_library import Move, Tool, IO
import time

robot = Move()
tool = Tool()
IO = IO()
```

### Moving the robot: `robot = Move()`

| Command | What it does |
|---|---|
| `robot.Home()` | Homes the robot: joints to their home position, roll 0, pitch 0. **Needed once after connecting before `MoveJ`, `MoveL`, `OffsetJ`, `OffsetL`.** |
| `robot.MoveJ([x, y, z], v, a)` | Moves the gripper tip to X Y Z; every joint takes its own path. Always give `v` and `a`. |
| `robot.MoveL([x, y, z], v=50, a=50)` | Moves the gripper tip to X Y Z in a straight line. |
| `robot.OffsetJ([dx, dy, dz], v=50, a=50)` | Moves the tip by dx dy dz from where it is (joint path). |
| `robot.OffsetL([dx, dy, dz], v=50, a=50)` | Moves the tip by dx dy dz in a straight line. |
| `robot.MoveJointPos([j1, j2, j3], v=50, a=50)` | Moves the joints to these angles (deg). Does not need `Home()`. |
| `robot.JogJoint([dj1, dj2, dj3], v, a)` | Turns each joint by this many degrees. Without `v`/`a` it uses the jog speed of the control panel. |
| `robot.WristRoll(deg, v=50, a=50)` | Turns the gripper about its own axis, -90 … 90°. |
| `robot.WristPitch(deg, v=50, a=50)` | Tilts the gripper, -90 … 90° to the horizontal (0 = level, + = up). |

A move that is out of reach or outside a joint limit is refused; the log shows why.

### Tool: `tool = Tool()`

| Command | What it does |
|---|---|
| `tool.SetTool("gripper")` | Selects the tool by its name in the robot settings. Call it once before the other tool commands. |
| `tool.MoveTo(pos)` | Servo tool (gripper): 0 = closed … 100 = open. |
| `tool.State("HIGH")` / `tool.State("LOW")` | Relay tool (vacuum, magnet): on / off. |

### Inputs and outputs: `IO = IO()`

| Command | What it does |
|---|---|
| `IO.SetIO(n, "OUTPUT")` / `IO.SetIO(n, "INPUT")` | Makes IO number `n` an output or an input (the pin is set in the robot settings). |
| `IO.digitalWrite(n, "HIGH")` / `IO.digitalWrite(n, "LOW")` | Switches output `n` on / off. |
| `IO.digitalRead(n)` | `True` when input `n` is on. |

### Waiting and plain Python

`time.sleep(seconds)` waits. Everything else in Python works as usual: `for` and `while`
loops, `if`, variables, functions, `print()` (shown in the log).

**About waiting between moves:**
- **USB:** every move command returns when the robot has finished the move, so
  `time.sleep(1)` after a move is a real 1 s pause.
- **Bluetooth:** a move command returns as soon as it is sent, so the wait runs while the arm is
  still moving. Make the wait longer than the move.

### Example

```python
from robot_library import Move
import time

robot = Move()
robot.Home()

robot.MoveJ([200, 0, 250], 40, 40)      # above the pick place
robot.WristPitch(-30, 40, 40)           # gripper down
robot.OffsetL([0, 0, -50], 20, 20)      # 50 mm down in a straight line
time.sleep(1)
robot.OffsetL([0, 0, 50], 20, 20)       # back up
robot.WristPitch(0, 40, 40)             # level again
robot.WristRoll(90, 40, 40)             # turn the gripper
robot.MoveJ([150, 100, 250], 40, 40)    # to the place position
```

The program field starts with a servo test that moves every servo on its own.

## Blockly

The ARM uses the blocks in **Robot 3 axis**. The 5- and 6-axis blocks do not work on it.

| Category | Blocks | Python it runs |
|---|---|---|
| Robot 3 axis | MoveJ, MoveL, OffsetJ, OffsetL (X Y Z v a) | `robot.MoveJ(...)` etc. |
| | Wrist roll (deg v a) | `robot.WristRoll(...)` |
| | Wrist pitch (deg v a) | `robot.WristPitch(...)` |
| Tool | set tool, tool move to, tool state | `tool.SetTool / MoveTo / State` |
| I/O | io pin, set io pin, digital write, digital read | `IO.SetIO / digitalWrite / digitalRead` |
| Wait | Wait … s | `time.sleep(...)` |
| Loops, Logic, Math, Text, Lists, Variables, Functions | standard Blockly | plain Python |

Blockly has no block for `Home`, `MoveJointPos` or `JogJoint`: home the robot with the *Home*
button before running a Blockly program that uses MoveJ / MoveL / Offset.

## Text programs (Kawasaki AS / G-code)

Choose **Text (Kawasaki AS / G-code)** in the dropdown and type the program as plain text. The
app checks the whole program first: if a line has an error, the log lists every error with
its line number and nothing moves. While the program runs, the log shows each line.

- Kawasaki AS and G-code lines can be mixed; every line is read on its own. Upper or lower case.
- Comments: `;` to the end of the line, and `( … )` in G-code lines.
- Units: mm, degrees, seconds; speed and acceleration in %.
- Text programs move J1–J3 and X Y Z only; the wrist roll and pitch stay where they are.
- Home the robot (`HOME` / `G28` or the *Home* button) before position moves, as in Python.

### Kawasaki AS

| Command | What it does |
|---|---|
| `HOME` | home the robot (in the simulation: joints to 0) |
| `JMOVE pose` | move to a pose, every joint on its own path |
| `LMOVE pose` | move to a Cartesian pose in a straight line |
| `DRAW dx, dy, dz` | move by dx dy dz from where the tool is, in a straight line |
| `SPEED n` (`SPEED n ALWAYS`) | speed in % for the following moves (default 50) |
| `ACCEL n` / `DECEL n` | acceleration in % for the following moves (one value for both) |
| `TWAIT t` | wait t seconds |
| `OPENI` / `CLOSEI` (or `OPEN` / `CLOSE`) | open / close the gripper |
| `SIGNAL n` / `SIGNAL -n` | switch output n on / off |
| `POINT name = TRANS(x, y, z)` | name a Cartesian pose |
| `POINT #name = #PPOINT(j1, j2, j3)` | name a joint pose (joint angles in degrees) |
| `.PROGRAM name()` … `.END` | program frame, may be left out |

A pose is `TRANS(x, y, z)`, `#PPOINT(j1, j2, j3)` or a name defined with `POINT`.
`TRANS(x, y, z, o, a, t)` is accepted, but O A T are ignored (warning in the log).
Not supported: `IF`, `FOR`, `WHILE`, `GOTO`, labels, `SWAIT`, `TDRAW`, `HERE`.

### G-code

| Code | What it does |
|---|---|
| `G0 X Y Z` | move, every joint on its own path |
| `G1 X Y Z F` | move in a straight line |
| `G4 P<ms>` / `G4 S<s>` | wait |
| `G28` | home |
| `G90` / `G91` | absolute (default) / relative coordinates for the following moves |
| `G21` | millimetres (the default) |
| `F<mm/min>` | speed: 9000 mm/min = 100 % (150 mm/s); kept for the following moves |
| `M3` / `M5` | close / open the gripper |
| `N…` | line number, ignored |

Axes that a line leaves out keep their value: `G1 Z50` only changes Z. Not supported: inches
(`G20`), arcs (`G2`, `G3`) and every other code.

### Example

```
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
```

## Control panel

- **Jog Joint:** `-` / `+` for J1–J3, **Roll** and **Pitch**, by the jog distance.
- **Jog Axis:** moves the tip along X, Y, Z (needs Home).
- **Tool:** opens / closes the gripper.
- **Mode:** the big **ROBOT / SIMULATION** toggle chooses where programs and jog buttons act.
  It starts in SIMULATION, switches to ROBOT when a robot connects and back when it
  disconnects; switching to ROBOT by hand without a robot asks first.
- **Robot:** *Connect robot*, *Home*, **Pause / Resume** (holds the robot in the middle of a
  move and continues it) and the red **STOP** (ends the program and the move at once; jog,
  Home and Run work again right after it).

## Firmware commands

What the app sends to the robot, for testing with a serial monitor (57600 baud, one command per
line, every command is answered with `END`):

| Command | Meaning |
|---|---|
| `CONNECT` | answers `ROBOT_CONNECTED` (and an I2C report if the servo driver is not found) |
| `Home` | home all joints, roll and pitch |
| `MoveJoint A<j1>B<j2>C<j3>D<v>E<a>` | joint angles |
| `jogJ A<dj1>B<dj2>C<dj3>D<v>E<a>` | relative joint angles |
| `MoveJ A<x>B<y>C<z>D<v>E<a>` / `MoveL …` | position, joint path / straight line |
| `OffsetJ …` / `OffsetL …` | relative position |
| `WristRoll A<deg>B<v>C<a>` | wrist roll, answers `ROLL: <deg>` |
| `WristPitch A<deg>B<v>C<a>` | wrist pitch, answers `PITCH: <deg>` and the new position |
| `Tool_move_to pos(<0-100>)` | gripper |
| `stop` / `pauze` / `play` | stop, pause, continue the current move |
