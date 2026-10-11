"""Generate the MiKoBots Studio profile (settings.json + placed STLs) for the ARM robot.

The ORIGINAL_*.stl files are exported from CAD in assembly coordinates, i.e. every
part sits where it belongs in one common frame. This script measures the pivot holes
of each part (see PIVOTS), derives the DH parameters from them, and expresses every
mesh in the DH frame it is attached to in MiKoBots Studio:

    base    -> world frame (not moving)
    link1   -> frame J1 (turntable)
    link2   -> frame J2 (upper arm)
    link3   -> frame J3 (forearm)
    gripper -> TOOL frame (passive level wrist, J4 = -(J2 + J3))

Run with the MiKoBots Studio venv from this folder:
    ..\\..\\..\\MiKoBots-Studio-main\\.venv\\Scripts\\python fit_arm.py
"""
import json
import shutil
import sys
from pathlib import Path

import numpy as np
from stl import mesh

HERE = Path(__file__).resolve().parent
NAME = HERE.name  # robot name == folder name, as MiKoBots Studio expects

# Pivot hole centres measured in the CAD assembly (mm), as (x, z); all axes are
# parallel to CAD Y except J1 which is vertical through (x, y) = J1_AXIS.
J1_AXIS = (-4.0, 1.0)
PIVOTS = {
    "link1_shoulder": (-4.0, 114.1),   # bearing bore in the turntable tower
    "link2_shoulder": (-2.9, 117.1),   # upper arm, proximal bore
    "link2_elbow": (145.1, 116.9),     # upper arm, distal bore
    "link3_elbow": (149.9, 116.2),     # forearm, proximal bore
    "link3_wrist": (49.4, 169.2),      # forearm tip tab
    "gripper_wrist": (19.0, 169.9),    # gripper mounting tab
}
TCP_FROM_JAW_TIP = 15.0  # tool centre point sits this far back from the jaw tips


def cad_to_robot(p):
    """CAD assembly -> robot base frame: J1 axis at the origin, reach direction +X."""
    p = np.asarray(p, dtype=float)
    q = np.empty_like(p)
    q[..., 0] = -(p[..., 0] - J1_AXIS[0])
    q[..., 1] = -(p[..., 1] - J1_AXIS[1])
    q[..., 2] = p[..., 2]
    return q


def xz(name):
    x, z = PIVOTS[name]
    p = cad_to_robot([x, J1_AXIS[1], z])
    return np.array([p[0], p[2]])


def dh_transform(theta, alpha, d, a):
    # identical to backend/calculations/kinematics_3_axis.py
    ct, st = np.cos(np.radians(theta)), np.sin(np.radians(theta))
    ca, sa = np.cos(np.radians(alpha)), np.sin(np.radians(alpha))
    return np.array([[ct, -st * ca, st * sa, a * ct],
                     [st, ct * ca, -ct * sa, a * st],
                     [0, sa, ca, d],
                     [0, 0, 0, 1]])


def forward(dh, joints):
    """Frames J1..J4 for a 3-axis arm with a level-keeping extra joint (as the app does)."""
    frames, T, acc = [], np.eye(4), 0.0
    for i, (theta, alpha, d, a) in enumerate(dh):
        if i == 3:
            theta = theta - acc
        else:
            theta = theta + joints[i]
            if i > 0:
                acc += theta
        T = T @ dh_transform(theta, alpha, d, a)
        frames.append(T)
    return frames


def app_rotation(rx, ry, rz):
    # identical to the rotation used by robot_3d_model.py / tool_managment.py
    rx, ry, rz = np.radians([rx, ry, rz])
    Rx = np.array([[1, 0, 0], [0, np.cos(rx), -np.sin(rx)], [0, np.sin(rx), np.cos(rx)]])
    Ry = np.array([[np.cos(ry), 0, np.sin(ry)], [0, 1, 0], [-np.sin(ry), 0, np.cos(ry)]])
    Rz = np.array([[np.cos(rz), -np.sin(rz), 0], [np.sin(rz), np.cos(rz), 0], [0, 0, 1]])
    return Rx @ Ry @ Rz


def euler_xyz(R):
    """Angles (deg) with R == Rx(a) @ Ry(b) @ Rz(c)."""
    b = np.arcsin(np.clip(R[0, 2], -1, 1))
    a = np.arctan2(-R[1, 2], R[2, 2])
    c = np.arctan2(-R[0, 1], R[0, 0])
    return [float(np.round(np.degrees(v), 3)) + 0.0 for v in (a, b, c)]


def placement(cad_mesh, M):
    """App origin setting [x, y, z, rx, ry, rz] that maps the CAD mesh by the 4x4 M."""
    rx, ry, rz = euler_xyz(M[:3, :3])
    R = app_rotation(rx, ry, rz)
    assert np.allclose(R, M[:3, :3], atol=1e-4), "Euler decomposition failed"
    pts = cad_mesh.vectors.reshape(-1, 3)
    placed = pts @ R.T + M[:3, 3]  # rounded angles, as the app will apply them
    lo = np.round(placed.min(axis=0), 2)
    return [float(lo[0]), float(lo[1]), float(lo[2]), rx, ry, rz]


def apply_placement(src, dst, origin):
    """Write the placed mesh exactly like the app does when 'origin' is applied."""
    m = mesh.Mesh.from_file(str(src))
    R = app_rotation(*origin[3:])
    m.vectors = np.einsum("ij,nkj->nki", R, m.vectors)
    m.vectors += np.array(origin[:3]) - m.vectors.reshape(-1, 3).min(axis=0)
    m.save(str(dst))
    return m


def homogeneous(R=np.eye(3), t=(0, 0, 0)):
    M = np.eye(4)
    M[:3, :3], M[:3, 3] = R, t
    return M


def main():
    stl = HERE / "ROBOT_STL"
    tools = HERE / "TOOLS_STL"
    cad = {n: mesh.Mesh.from_file(str(stl / f"ORIGINAL_{n}.stl")) for n in ("base", "link1", "link2", "link3", "link4")}

    # ---- kinematics from the pivot holes (robot frame, XZ plane) -------------
    sh1, sh2, el2 = xz("link1_shoulder"), xz("link2_shoulder"), xz("link2_elbow")
    el3, wr3, wr4 = xz("link3_elbow"), xz("link3_wrist"), xz("gripper_wrist")
    d1 = sh1[1]
    L2 = np.linalg.norm(el2 - sh2)
    L3 = np.linalg.norm(wr3 - el3)

    grip = cad_to_robot(cad["link4"].vectors.reshape(-1, 3))
    tcp = np.array([grip[:, 0].max() - TCP_FROM_JAW_TIP, 0.0, grip[:, 2].mean()])
    L4 = tcp[0] - wr4[0]            # horizontal reach of the gripper past the wrist pivot
    tcp_up = tcp[2] - wr4[1]        # vertical TCP offset (tool frame Y is "up")

    dh = [[0.0, 90.0, d1, 0.0],
          [90.0, 0.0, 0.0, L2],
          [-90.0, 0.0, 0.0, L3],
          [0.0, 0.0, 0.0, L4]]
    tool_frame = [0.0, round(float(tcp_up), 2), 0.0, 0.0, 0.0, 0.0]

    # joint angles of the pose the CAD assembly was exported in
    phi2 = np.degrees(np.arctan2(*(el2 - sh2)[::-1]))   # upper arm direction
    phi3 = np.degrees(np.arctan2(*(wr3 - el3)[::-1]))   # forearm direction
    wrap = lambda a: float((a + 180.0) % 360.0 - 180.0)
    cad_pose = [0.0, wrap(phi2 - 90.0), wrap(phi3 - phi2 + 90.0)]
    F = forward(dh, cad_pose)
    T_tool = F[3] @ np.array([[1, 0, 0, 0], [0, 1, 0, tool_frame[1]], [0, 0, 1, 0], [0, 0, 0, 1]])

    # shift every part so its own proximal bore lands on the kinematic pivot
    def shift(kin_xz, own_xz):
        d = kin_xz - own_xz
        return homogeneous(t=(d[0], 0.0, d[1]))

    W = homogeneous(R=np.diag([-1.0, -1.0, 1.0]), t=(J1_AXIS[0], J1_AXIS[1], 0.0))  # cad_to_robot
    elbow_kin = F[1][[0, 2], 3]
    wrist_kin = F[2][[0, 2], 3]
    parts = {
        "base": np.eye(4) @ W,
        "link1": np.linalg.inv(F[0]) @ W,
        "link2": np.linalg.inv(F[1]) @ shift(sh1, sh2) @ W,
        "link3": np.linalg.inv(F[2]) @ shift(elbow_kin, el3) @ W,
        "gripper": np.linalg.inv(T_tool) @ shift(wrist_kin, wr4) @ W,
    }

    # ---- write the placed meshes ---------------------------------------------
    files_3d, colours = [], {"base": "red", "link1": "red", "link2": "red", "link3": "red"}
    labels = {"base": "Base", "link1": "Link 1", "link2": "Link 2", "link3": "Link 3"}
    for n in ("base", "link1", "link2", "link3"):
        origin = placement(cad[n], parts[n])
        apply_placement(stl / f"ORIGINAL_{n}.stl", stl / f"{n}.stl", origin)
        files_3d.append([f"{n}.stl", f"/Robot_library/{NAME}/ROBOT_STL/ORIGINAL_{n}.stl",
                         f"/Robot_library/{NAME}/ROBOT_STL/{n}.stl", colours[n], origin, labels[n]])

    tools.mkdir(exist_ok=True)
    shutil.copyfile(stl / "ORIGINAL_link4.stl", tools / "ORIGINAL_gripper.stl")
    grip_origin = placement(cad["link4"], parts["gripper"])
    apply_placement(tools / "ORIGINAL_gripper.stl", tools / "gripper.stl", grip_origin)
    tool = ["gripper", f"/Robot_library/{NAME}/TOOLS_STL/ORIGINAL_gripper.stl",
            f"/Robot_library/{NAME}/TOOLS_STL/gripper.stl", "red", grip_origin, tool_frame,
            "Tool pin 1", "Servo", [40.0, 120.0], [0.0, 0.0, 0.0], "0", ["0", "0", "0", "0"]]

    # ---- settings.json --------------------------------------------------------
    s = lambda v: f"{v:.2f}".rstrip("0").rstrip(".")
    settings_path = HERE / "settings.json"
    old = json.loads(settings_path.read_text())
    n = 3
    settings = {
        "Set_robot_name": [NAME, ""],
        "Set_number_of_joints": [str(n), ""],
        "Set_extra_joint": [1, ""],
        "Set_motor_type": [["1"] * n, ""],                       # 1 = servo
        "Set_servo_pin": [["0", "1", "2"], ""],                  # PCA9685 channels (pinout.h)
        "Set_servo_pulse": [["2500", "500", "180"] * n, ""],
        "Set_servo_position": [["90"] * n, ""],
        "Set_motor_pin": [[0] * (2 * n), ""],
        "Set_ena_pin": [[0] * n, ""],
        "Set_switch_pin": [[0] * n, ""],
        "Set_lim_pos": [[0] * n, ""],
        "Set_step_deg": [[0] * n, ""],
        "Set_dir_joints": [["0"] * n, ""],
        "Set_max_pos": [["-90", "90", "-60", "95", "-90", "60"], ""],  # [min, max] per joint
        "Set_speed": [["100", "1000"] * n, ""],
        "Set_home_settings": [["1", "0", "2", "0", "3", "0"], ""],
        "Set_dh_par": [[[s(v) for v in row] for row in dh], ""],
        "Set_tools": [["6", "0", "0", "0", "0"], "IO"],          # gripper servo on PCA9685 channel 6
        "Set_io_pin": old["Settings"].get("Set_io_pin", [["0"] * 10, "IO"]),
    }
    out = {"Name": NAME, "Settings": settings, "3Dfiles": files_3d, "Tools": [tool],
           "IOSettings": old.get("IOSettings", "IOSettings")}
    settings_path.write_text(json.dumps(out, indent=4))

    print(f"d1={d1:.1f}  L2={L2:.1f}  L3={L3:.1f}  L4={L4:.1f}  tool Y={tcp_up:.1f}")
    print("CAD export pose (J1, J2, J3):", [round(v, 1) for v in cad_pose])
    for f in files_3d + [tool]:
        print(f"  {f[0]:12s} {f[4]}")
    return dh, tool_frame, cad_pose


if __name__ == "__main__":
    sys.exit(main() and 0)
