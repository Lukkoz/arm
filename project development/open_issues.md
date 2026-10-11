# Open issues

Known issues deferred to later. The letters are the ones used in the review.
"App" means MiKoBots Studio (`MiKoBots-Studio-main/MiKoBots_Studio/src`), "firmware" means this repository.

---

## E · App inverse kinematics is wrong in part of the work area (verify on hardware)
**Where:** app, `backend/calculations/kinematics_3_axis.py`, `InverseKinematics_3.run`

**Problem:** the app computes `J1 = atan(Y / X)` and `a2 = atan((Z - L1) / r)`. `atan` only returns −90°…90° and loses the sign of the denominator, so it gives wrong joint angles in two cases:
- the target is behind the robot (X < 0);
- the wrist pivot is behind the J1 axis (r < 0).

A test with 3000 random reachable ARM poses gave a wrong answer for 1097 of them (FK(IK(p)) ≠ p).

**Effect:** the firmware's IK (`kinematics.cpp`) uses `atan2` and is correct there. In those areas the app simulation and its jog/target calculations can therefore show a different arm pose than the real robot takes. Moves sent as Cartesian targets (MoveJ, MoveL, Offset*) are solved by the firmware, so the real robot goes to the correct point; the app's display and its reachability check (`checkJointPos`) may still be wrong.

**Possible fix:** replace both with `math.atan2(Y, X)` and `math.atan2(Z - L1, r)`.

**Before fixing, check with hardware:** move the real ARM to targets with X < 0 and to targets close to the base, and compare the app's displayed pose with the real one. That shows how much this matters in practice.

---

## I · Cartesian moves only for 3-axis robots
**Where:** firmware, `kinematics.cpp` (`kin_supported()` is true only for 3 joints)

**Problem:** MoveJ, MoveL, OffsetJ, OffsetL and jogL answer `ERROR: Cartesian moves are only implemented for 3-axis robots` on 5- and 6-axis robots. Joint moves (MoveJoint, jogJ, Home) work for any joint count. For other robots the `POS:` line reports X/Y/Z as 0.

**Possible fix:** port the 5- and 6-axis kinematics. The old code is in `Kinematics/Kinematics_5.h` and `Kinematics_6.h` (not compiled), and the app's versions are in `kinematics_5_axis.py` / `kinematics_6_axis.py`. Use the same conventions as the app, as was done for 3 axes.

---

## J · IK always aims J1 at the target
**Where:** firmware `kin_inverse()` (the app's IK does the same)

**Problem:** a 3-axis arm can reach most points in two ways: facing the point, or turned 180° and reaching back over the base. The IK only computes the first. If that posture breaks a joint limit, the move is refused, even when the mirrored posture would be within limits.

**Possible fix:** when the first solution fails the limit check, also try `J1 + 180°` with the mirrored J2/J3 solution, and take the valid one closest to the current position.

---

## K · A command that arrives during a move blocks `stop`
**Where:** firmware, `communication.cpp` (`comm_poll_motion`)

**Problem:**
- During a move the firmware only acts on `stop`, `pauze` and `play`.
- Any other line is held in the receive buffer until the move ends, and reading stops while it waits there. A `stop` sent after that line is only seen once the move is over.

**When it happens:** the app waits for END before sending, so this only happens if the app gives up waiting. Since fix C, that requires the robot to be silent for 10 s, which it never is during a move because it sends `BUSY` every second. So in practice it needs a stalled robot or a third-party sender.

**Possible fix:** while a line is held, keep reading into a small second buffer and check only for `stop` / `pauze` / `play`.

---

## N · PCA9685 ON value is always 0
**Where:** firmware, `pca9685.cpp` (`setPwm`, `writeMicroseconds`)

**Problem:**
- Every channel starts its pulse at count 0, so all servos draw their current spike at the same moment each 20 ms frame.
- The `on` parameter of `setPwm()` is therefore never anything but 0.

**Possible fix:** stagger the channels (`on = channel × 256`), or remove the parameter. The explanation and both options are in `design_notes.md`, section "PCA9685 servo driver".
