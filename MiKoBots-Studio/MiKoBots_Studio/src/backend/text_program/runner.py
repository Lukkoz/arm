"""Runs the steps of a parsed text program through robot_library (robot or simulation).

Every move goes through robot_library.Move, so stop / pause and the simulation work the same
way as for Python programs. The runner keeps the modal state of the program (speed,
acceleration, absolute or relative G-code coordinates) and the last known tool position, which
G-code needs for axes that a line leaves out.
"""
import os
import time

import backend.core.variables as var
from backend.simulation import check_simulation_on
from robot_library import Move, Tool, IO

HOME_JOINTS_SIM = [0.0, 0.0, 0.0]   # the simulation has no Home command: go to joint 0


class TextProgramRunner:
    def __init__(self):
        self.robot = Move()
        self.tool = None
        self.io = None
        self.speed = 50.0
        self.accel = 50.0
        self.relative = False
        self.position = None         # last known X Y Z of the tool, None = ask the robot

    # ---- state
    def current_position(self, step):
        if self.position is None:
            pos = var.POS_AXIS_SIM if check_simulation_on() else var.POS_AXIS
            if not pos or len(pos) < 3:
                raise RuntimeError(f"Line {step.line}: the robot position is unknown, "
                                   "start with HOME / G28 or give X, Y and Z")
            self.position = [float(p) for p in pos[:3]]
        return list(self.position)

    def get_tool(self):
        if self.tool is None:
            self.tool = Tool()
            names = [os.path.splitext(t[0])[0] for t in (var.TOOL_SETTINGS or [])]
            if not names:
                raise RuntimeError("no tool is set up for this robot")
            self.tool.SetTool(names[0])
        return self.tool

    # ---- steps
    def run(self, steps):
        for step in steps:
            print(f"Line {step.line}: {step.source}")
            getattr(self, "step_" + step.kind)(step)

    def step_home(self, step):
        if check_simulation_on():
            self.robot.MoveJointPos(HOME_JOINTS_SIM, self.speed, self.accel)
        else:
            self.robot.Home()
        self.position = None

    def step_move_cart(self, step):
        xyz = step.args["xyz"]
        move = self.robot.MoveL if step.args["linear"] else self.robot.MoveJ
        move(list(xyz), self.speed, self.accel)
        self.position = list(xyz)

    def step_move_joint(self, step):
        self.robot.MoveJointPos(list(step.args["joints"]), self.speed, self.accel)
        self.position = None

    def step_offset(self, step):
        d = step.args["xyz"]
        move = self.robot.OffsetL if step.args["linear"] else self.robot.OffsetJ
        move(list(d), self.speed, self.accel)
        if self.position is not None:
            self.position = [p + dp for p, dp in zip(self.position, d)]

    def step_gmove(self, step):
        axes, linear = step.args["axes"], step.args["linear"]
        if self.relative:
            d = [axes.get(a, 0.0) for a in "XYZ"]
            self.step_offset(type(step)("offset", step.line, step.source, {"xyz": d, "linear": linear}))
        else:
            xyz = self.current_position(step) if len(axes) < 3 else [0.0, 0.0, 0.0]
            for i, a in enumerate("XYZ"):
                if a in axes:
                    xyz[i] = axes[a]
            self.step_move_cart(type(step)("move_cart", step.line, step.source, {"xyz": xyz, "linear": linear}))

    def step_wait(self, step):
        time.sleep(step.args["seconds"])

    def step_tool(self, step):
        self.get_tool().MoveTo(100 if step.args["open"] else 0)

    def step_signal(self, step):
        if self.io is None:
            self.io = IO()
        n = step.args["number"]
        self.io.SetIO(n, "OUTPUT")
        self.io.digitalWrite(n, "HIGH" if step.args["on"] else "LOW")

    def step_speed(self, step):
        self.speed = step.args["pct"]

    def step_accel(self, step):
        self.accel = step.args["pct"]

    def step_mode(self, step):
        self.relative = step.args["relative"]


def run_steps(steps):
    TextProgramRunner().run(steps)
