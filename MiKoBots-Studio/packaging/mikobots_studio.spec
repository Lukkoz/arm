# PyInstaller recipe for MiKoBots Studio (ARM version).
# Build with packaging\build.ps1, or from arm\MiKoBots-Studio:
#   .venv\Scripts\python -m PyInstaller packaging\mikobots_studio.spec --noconfirm
#
# Result: <distpath>\MiKoBots Studio\MiKoBots Studio.exe (build.ps1 uses the release folder outside the repo). Everything sits next to the exe
# (contents_directory "."), because the app looks for its files there when frozen:
#   assets\   images, language file, default robot data   (file_management.py, folder_check.py)
#   blockly\  the Blockly editor, found relative to the program field module
import os
import sys
from pathlib import Path

from PyInstaller.utils.hooks import collect_submodules

DEBUG_CONSOLE = os.environ.get("MIKO_CONSOLE") == "1"   # set to 1 to see errors in a console window

ROOT = Path(SPECPATH).parent
SRC = ROOT / "MiKoBots_Studio" / "src"
ASSETS = ROOT / "MiKoBots_Studio" / "assets"

# The venv is based on an Anaconda Python, which keeps the DLLs behind _ctypes, _bz2,
# _lzma, pyexpat and _sqlite3 in Library\bin, where PyInstaller does not look.
CONDA_BIN = Path(sys.base_prefix) / "Library" / "bin"
CONDA_DLLS = ["ffi.dll", "ffi-7.dll", "ffi-8.dll", "libbz2.dll", "liblzma.dll", "libexpat.dll",
              "sqlite3.dll", "zlib-ng2.dll"]
EXTRA_BINARIES = [(str(CONDA_BIN / dll), ".") for dll in CONDA_DLLS if (CONDA_BIN / dll).exists()]

a = Analysis(
    [str(SRC / "main.py")],
    pathex=[str(SRC)],
    binaries=EXTRA_BINARIES,
    datas=[
        (str(ASSETS), "assets"),
        (str(SRC / "blockly"), "blockly"),
    ],
    # the robot library is only imported by the user programs run in the program field
    hiddenimports=collect_submodules("robot_library"),
    excludes=["tkinter", "matplotlib", "IPython", "pytest"],
    noarchive=False,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="MiKoBots Studio",
    icon=str(ASSETS / "images" / "mikobot.ico"),
    console=DEBUG_CONSOLE,
    contents_directory=".",
)
coll = COLLECT(exe, a.binaries, a.datas, name="MiKoBots Studio")
