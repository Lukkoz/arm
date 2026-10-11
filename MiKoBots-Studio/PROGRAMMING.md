# Programming the ARM in MiKoBots Studio

The program field can hold a **Python** program or a **Blockly** program (tick *Blockly* to switch).
Both drive the robot when it is connected, or the simulation when *Simulation* is on.

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

## Control panel

- **Jog Joint:** `-` / `+` for J1–J3, **Roll** and **Pitch**, by the jog distance.
- **Jog Axis:** moves the tip along X, Y, Z (needs Home).
- **Tool:** opens / closes the gripper.

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
