"""Tests for the text program parser (Kawasaki AS + G-code).

Run from arm\\MiKoBots-Studio with the app's Python:
    <venv>\\Scripts\\python tests\\test_text_program.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "MiKoBots_Studio" / "src"))
from backend.text_program.parser import parse  # noqa: E402


def kinds(steps):
    return [s.kind for s in steps]


def ok(text):
    steps, errors, warnings = parse(text)
    assert not errors, errors
    return steps, warnings


def fails(text, line, fragment):
    _, errors, _ = parse(text)
    assert any(e.startswith(f"Line {line}:") and fragment in e for e in errors), errors


def test_example_program():
    steps, warnings = ok("""
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
""")
    assert kinds(steps) == ["home", "speed", "move_cart", "offset", "wait", "offset",
                            "mode", "speed", "gmove", "wait", "home"], kinds(steps)
    assert steps[2].args == {"xyz": [200, 0, 250], "linear": False}
    assert steps[3].args == {"xyz": [0, 0, -40], "linear": True}
    assert steps[8].args == {"axes": {"X": 150, "Y": 100, "Z": 250}, "linear": True}
    assert steps[9].args["seconds"] == 0.5
    assert steps[2].line == 6 and steps[2].source == "JMOVE pick"
    assert not warnings


def test_kawasaki_commands():
    steps, warnings = ok("""
.PROGRAM demo()
POINT #up = #PPOINT(0, 20, -10)
JMOVE #up
JMOVE #PPOINT(10, 0, 0)
LMOVE TRANS(150, 50, 200, 0, 90, 0)
SPEED 40 ALWAYS
ACCEL 60
DECEL 70
OPENI
CLOSEI
SIGNAL 2
SIGNAL -2
.END
""")
    assert kinds(steps) == ["move_joint", "move_joint", "move_cart", "speed", "accel", "accel",
                            "tool", "tool", "signal", "signal"], kinds(steps)
    assert steps[0].args["joints"] == [0, 20, -10]
    assert steps[2].args == {"xyz": [150, 50, 200], "linear": True}
    assert steps[6].args["open"] and not steps[7].args["open"]
    assert steps[8].args == {"number": 2, "on": True}
    assert steps[9].args == {"number": 2, "on": False}
    assert len(warnings) == 1 and "O, A, T" in warnings[0]


def test_lowercase_and_comments():
    steps, _ = ok("jmove trans(1,2,3)   ; go\n(gcode comment)\nN10 g0 x5 (inline)\n")
    assert kinds(steps) == ["move_cart", "gmove"]
    assert steps[1].args["axes"] == {"X": 5}


def test_gcode_commands():
    steps, _ = ok("G21\nG91\nG0 Z-10\nG1 X5 F900000\nM3\nM5\nG4 S2\nF60\n")
    assert kinds(steps) == ["mode", "gmove", "speed", "gmove", "tool", "tool", "wait", "speed"]
    assert steps[0].args["relative"] is True
    assert steps[1].args == {"axes": {"Z": -10}, "linear": False}
    assert steps[2].args["pct"] == 100            # clamped
    assert steps[4].args["open"] is False and steps[5].args["open"] is True
    assert steps[6].args["seconds"] == 2
    assert steps[7].args["pct"] == 1              # 60 mm/min -> 0.67 %, clamped to 1


def test_feed_conversion():
    steps, _ = ok("G1 X0 Y0 Z0 F4500")
    assert steps[0].kind == "speed" and steps[0].args["pct"] == 50.0


def test_errors_have_line_numbers():
    fails("HOME\nJMOVE nowhere", 2, "not defined")
    fails("FLY 3", 1, "unknown command")
    fails("G2 X1 Y1 I5", 1, "not supported")
    fails("G20", 1, "inches")
    fails("G1 F100", 1, "at least one of X, Y, Z")
    fails("LMOVE #PPOINT(1,2,3)", 1, "Cartesian")
    fails("JMOVE TRANS(1,2)", 1, "3 or 6 values")
    fails("SPEED 150", 1, "1-100")
    fails("TWAIT", 1, "seconds")
    fails("POINT #a = TRANS(1,2,3)", 1, "name")
    fails("G4", 1, "P<milliseconds>")


def test_errors_stop_everything():
    steps, errors, _ = parse("HOME\nJMOVE TRANS(1,2,3)\nOOPS")
    assert errors and steps          # steps are returned, but run_text_program refuses to run them



# ---- runner, with robot_library and the simulation check replaced by recorders
def make_runner(simulation, position=(100.0, 0.0, 200.0)):
    import types
    calls = []

    class Recorder:
        def __getattr__(self, name):
            return lambda *args: calls.append((name, args))

    fake_lib = types.ModuleType("robot_library")
    fake_lib.Move = fake_lib.Tool = fake_lib.IO = Recorder
    fake_sim = types.ModuleType("backend.simulation")
    fake_sim.check_simulation_on = lambda: simulation
    import backend.core.variables as var          # plain values, safe to use in a test
    var.POS_AXIS = list(position)
    var.POS_AXIS_SIM = list(position)
    var.TOOL_SETTINGS = [["gripper.stl"]]
    saved = {k: sys.modules.get(k) for k in ("robot_library", "backend.simulation",
                                              "backend.text_program.runner")}
    sys.modules.update({"robot_library": fake_lib, "backend.simulation": fake_sim})
    sys.modules.pop("backend.text_program.runner", None)
    try:
        import importlib
        runner = importlib.import_module("backend.text_program.runner")   # fresh, with the recorders
        return runner.TextProgramRunner(), calls
    finally:
        for k, v in saved.items():
            if v is None:
                sys.modules.pop(k, None)
            else:
                sys.modules[k] = v


def run_text(text, simulation=True):
    steps, errors, _ = parse(text)
    assert not errors, errors
    runner, calls = make_runner(simulation)
    import io, contextlib
    with contextlib.redirect_stdout(io.StringIO()):
        runner.run(steps)
    return calls


def test_runner_example_in_simulation():
    calls = run_text("""HOME
SPEED 30
JMOVE TRANS(200,0,250)
DRAW 0,0,-40
G90
G1 Z50 F4500
G28""")
    assert calls == [
        ("MoveJointPos", ([0.0, 0.0, 0.0], 50.0, 50.0)),     # HOME in the simulation
        ("MoveJ", ([200.0, 0.0, 250.0], 30.0, 50.0)),
        ("OffsetL", ([0.0, 0.0, -40.0], 30.0, 50.0)),
        ("MoveL", ([200.0, 0.0, 50.0], 50.0, 50.0)),         # X, Y kept from the tracked position
        ("MoveJointPos", ([0.0, 0.0, 0.0], 50.0, 50.0)),
    ], calls


def test_runner_robot_and_relative_gcode():
    calls = run_text("""G28
G91
G0 X10
G1 Y-5 Z2
G90
G0 X1 Y2 Z3
M3
SIGNAL 4
ACCEL 20
JMOVE #PPOINT(1,2,3)""", simulation=False)
    assert calls == [
        ("Home", ()),
        ("OffsetJ", ([10.0, 0.0, 0.0], 50.0, 50.0)),
        ("OffsetL", ([0.0, -5.0, 2.0], 50.0, 50.0)),
        ("MoveJ", ([1.0, 2.0, 3.0], 50.0, 50.0)),
        ("SetTool", ("gripper",)), ("MoveTo", (0,)),
        ("SetIO", (4, "OUTPUT")), ("digitalWrite", (4, "HIGH")),
        ("MoveJointPos", ([1.0, 2.0, 3.0], 50.0, 20.0)),
    ], calls


def test_runner_missing_axis_uses_robot_position():
    calls = run_text("G1 Z10")          # no HOME: X, Y from the reported position (100, 0, 200)
    assert calls == [("MoveL", ([100.0, 0.0, 10.0], 50.0, 50.0))], calls

if __name__ == "__main__":
    tests = [(name, fn) for name, fn in globals().items() if name.startswith("test_")]
    for name, fn in tests:
        fn()
        print(f"ok   {name}")
    print(f"{len(tests)} tests passed")
