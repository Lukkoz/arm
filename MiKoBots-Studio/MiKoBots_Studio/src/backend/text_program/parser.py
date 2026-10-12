"""Parser for plain-text robot programs: a simple subset of Kawasaki AS and of CNC G-code.

parse(text) checks the whole program first and returns (steps, errors, warnings). Every step
is a Step(kind, line, source, args); the runner (runner.py) executes them. Nothing here touches
the robot, so the parser can be tested on its own.

Both dialects may be used in the same program; every line is recognised on its own.
Units: mm, degrees, seconds. The wrist (pitch, roll) is not controlled from text programs.
"""
import re
from dataclasses import dataclass, field

# G-code feed F is in mm/min; 100 % equals the firmware's linear speed of 150 mm/s
FEED_AT_100_PCT = 150.0 * 60.0


@dataclass
class Step:
    kind: str            # home, move_cart, move_joint, offset, wait, tool, signal, speed, accel, mode
    line: int            # line number in the program, from 1
    source: str          # the program line, for the log
    args: dict = field(default_factory=dict)


NUMBER = r"[-+]?(?:\d+\.?\d*|\.\d+)"
CALL_RE = re.compile(r"^(#?)(TRANS|PPOINT)\s*\((.*)\)$", re.IGNORECASE)
NAME_RE = re.compile(r"^#?[A-Za-z_][A-Za-z0-9_]*$")
GWORD_RE = re.compile(r"([A-Z])\s*(" + NUMBER + r")")


def _numbers(text, line):
    values = []
    for part in text.split(","):
        part = part.strip()
        if not re.fullmatch(NUMBER, part):
            raise ValueError(f"'{part}' is not a number")
        values.append(float(part))
    return values


def _strip_comment(raw):
    text = raw.split(";", 1)[0]
    # ( ... ) is a G-code comment; Kawasaki lines keep their brackets: TRANS(...)
    if _looks_like_gcode(text) or text.lstrip().startswith("("):
        text = re.sub(r"\([^)]*\)", "", text)
    return text.strip()


def _looks_like_gcode(text):
    return re.match(r"^\s*(N\d+\s*)?[GMF]\s*" + NUMBER, text, re.IGNORECASE) is not None


class _Parser:
    def __init__(self):
        self.steps, self.errors, self.warnings = [], [], []
        self.points = {}          # name -> ("cart", [x, y, z]) or ("joint", [j1, j2, j3])

    # ---- helpers
    def error(self, line, message):
        self.errors.append(f"Line {line}: {message}")

    def warn(self, line, message):
        self.warnings.append(f"Line {line}: {message}")

    def add(self, kind, line, source, **args):
        self.steps.append(Step(kind, line, source, args))

    def pose(self, text, line):
        """TRANS(x,y,z[,o,a,t]), #PPOINT(j1,j2,j3) or a POINT name -> ('cart'|'joint', values)."""
        text = text.strip()
        match = CALL_RE.match(text)
        if match:
            hashed, func, inner = match.group(1), match.group(2).upper(), match.group(3)
            values = _numbers(inner, line)
            if func == "TRANS":
                if hashed:
                    raise ValueError("TRANS is written without '#'")
                if len(values) not in (3, 6):
                    raise ValueError(f"TRANS needs 3 or 6 values (x, y, z[, o, a, t]), got {len(values)}")
                if len(values) == 6:
                    self.warn(line, "O, A, T are ignored: the wrist is not controlled from text programs")
                return "cart", values[:3]
            if not hashed:
                raise ValueError("PPOINT is written as #PPOINT(j1, j2, j3)")
            if len(values) != 3:
                raise ValueError(f"#PPOINT needs 3 joint values (j1, j2, j3), got {len(values)}")
            return "joint", values
        if NAME_RE.match(text):
            key = text.upper()
            if key not in self.points:
                raise ValueError(f"pose '{text}' is not defined (define it first with POINT)")
            return self.points[key]
        raise ValueError(f"'{text}' is not a pose: use TRANS(x, y, z), #PPOINT(j1, j2, j3) or a POINT name")

    # ---- Kawasaki AS
    def kawasaki(self, text, line, source):
        word, _, rest = text.partition(" ")
        word, rest = word.upper(), rest.strip()

        if word in (".PROGRAM", ".END", "END"):
            return
        if word == "HOME":
            self.add("home", line, source)
        elif word in ("JMOVE", "LMOVE"):
            if not rest:
                raise ValueError(f"{word} needs a pose")
            kind, values = self.pose(rest, line)
            if kind == "joint":
                if word == "LMOVE":
                    raise ValueError("LMOVE needs a Cartesian pose (TRANS or a POINT name without '#')")
                self.add("move_joint", line, source, joints=values)
            else:
                self.add("move_cart", line, source, xyz=values, linear=word == "LMOVE")
        elif word == "DRAW":
            values = _numbers(rest, line) if rest else []
            if not 1 <= len(values) <= 3:
                raise ValueError("DRAW needs dx[, dy[, dz]]")
            values += [0.0] * (3 - len(values))
            self.add("offset", line, source, xyz=values, linear=True)
        elif word in ("SPEED", "ACCEL", "DECEL"):
            parts = rest.split()
            if not parts or not re.fullmatch(NUMBER, parts[0]):
                raise ValueError(f"{word} needs a value in %")
            if len(parts) > 1 and not (word == "SPEED" and parts[1].upper() == "ALWAYS"):
                raise ValueError(f"unexpected '{' '.join(parts[1:])}' after {word}")
            value = float(parts[0])
            if not 1 <= value <= 100:
                raise ValueError(f"{word} must be 1-100 %")
            self.add("speed" if word == "SPEED" else "accel", line, source, pct=value)
        elif word == "TWAIT":
            values = _numbers(rest, line) if rest else []
            if len(values) != 1 or values[0] < 0:
                raise ValueError("TWAIT needs a time in seconds")
            self.add("wait", line, source, seconds=values[0])
        elif word in ("OPEN", "OPENI", "CLOSE", "CLOSEI"):
            self.add("tool", line, source, open=word.startswith("OPEN"))
        elif word == "SIGNAL":
            values = _numbers(rest, line) if rest else []
            if len(values) != 1 or values[0] == 0 or values[0] != int(values[0]):
                raise ValueError("SIGNAL needs an output number, negative to switch it off")
            self.add("signal", line, source, number=abs(int(values[0])), on=values[0] > 0)
        elif word == "POINT":
            name, eq, value = rest.partition("=")
            name = name.strip()
            if not eq or not NAME_RE.match(name):
                raise ValueError("POINT needs: POINT name = TRANS(...) or POINT #name = #PPOINT(...)")
            kind, values = self.pose(value, line)
            if (kind == "joint") != name.startswith("#"):
                raise ValueError("joint poses (#PPOINT) need a name with '#', Cartesian poses (TRANS) one without")
            self.points[name.upper()] = (kind, values)
        else:
            raise ValueError(f"unknown command '{word}'")

    # ---- CNC G-code
    def gcode(self, text, line, source):
        text = re.sub(r"^N\d+\s*", "", text.upper())
        words = GWORD_RE.findall(text)
        if re.sub(r"\s", "", text) != "".join(letter + number for letter, number in words):
            raise ValueError("could not read this G-code line")

        codes = [(letter, float(number)) for letter, number in words if letter in "GM"]
        values = {letter: float(number) for letter, number in words if letter not in "GM"}
        for letter in values:
            if letter not in "XYZFPS":
                raise ValueError(f"'{letter}' is not supported")

        if "F" in values:
            if values["F"] <= 0:
                raise ValueError("F must be greater than 0")
            pct = min(100.0, max(1.0, values["F"] / FEED_AT_100_PCT * 100.0))
            self.add("speed", line, source, pct=round(pct, 1))
        if len([c for c in codes if c[0] == "G"]) > 1 or len([c for c in codes if c[0] == "M"]) > 1:
            raise ValueError("only one G and one M code per line")

        for letter, number in codes:
            code = f"{letter}{number:g}"
            if code in ("G0", "G1"):
                axes = {a: values[a] for a in "XYZ" if a in values}
                if not axes:
                    raise ValueError(f"{code} needs at least one of X, Y, Z")
                self.add("gmove", line, source, axes=axes, linear=code == "G1")
            elif code == "G4":
                if "P" in values:
                    seconds = values["P"] / 1000.0
                elif "S" in values:
                    seconds = values["S"]
                else:
                    raise ValueError("G4 needs P<milliseconds> or S<seconds>")
                self.add("wait", line, source, seconds=seconds)
            elif code == "G28":
                self.add("home", line, source)
            elif code in ("G90", "G91"):
                self.add("mode", line, source, relative=code == "G91")
            elif code == "G21":
                pass
            elif code == "G20":
                raise ValueError("G20 (inches) is not supported, use millimetres (G21)")
            elif code in ("G2", "G3"):
                raise ValueError("arcs (G2/G3) are not supported")
            elif code in ("M3", "M5"):
                self.add("tool", line, source, open=code == "M5")
            else:
                raise ValueError(f"{code} is not supported")
        if not codes and "F" not in values:
            raise ValueError("expected a G or M code")

    # ---- program
    def parse(self, text):
        for number, raw in enumerate(text.splitlines(), start=1):
            source = raw.strip()
            text_line = _strip_comment(raw)
            if not text_line:
                continue
            try:
                if _looks_like_gcode(text_line):
                    self.gcode(text_line, number, source)
                else:
                    self.kawasaki(text_line, number, source)
            except ValueError as e:
                self.error(number, str(e))
        return self.steps, self.errors, self.warnings


def parse(text):
    """Returns (steps, errors, warnings) for a text program. Run the steps only if errors is empty."""
    return _Parser().parse(text)
