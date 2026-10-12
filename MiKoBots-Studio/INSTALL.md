# Installing MiKoBots Studio ARM

MiKoBots Studio ARM is the program that controls and simulates the ARM robot arm.
It is one installer file: **`MiKoBots Studio ARM setup.exe`** (about 210 MB).
Nothing else needs to be installed: Python and all libraries are included.

## Requirements

- Windows 10 or 11, 64-bit
- About 1 GB of free disk space
- To control the real robot: a USB-C cable to the robot's XIAO board, or Bluetooth on the PC
- The robot must already have the ARM firmware (see [Robot firmware](#robot-firmware))

## Install

1. Run **`MiKoBots Studio ARM setup.exe`**.
2. Windows may show **"Windows protected your PC"**, because the installer is not digitally
   signed. Click **More info**, then **Run anyway**.
3. Choose whether to install for yourself only (no administrator rights needed) or for all users.
4. Keep the suggested folder or choose another one, tick **Create a desktop shortcut** if you
   want one, and click **Install**.
5. Leave **Launch MiKoBots Studio ARM** ticked and click **Finish**.

The first start takes up to a minute: the program creates its data folder
`Documents\MyAppData\MiKoBots` with the ARM robot and its 3D model.

If the PC already had MiKoBots Studio (the folder `Documents\MyAppData\MiKoBots` exists), the
program adds the ARM robot to it and moves the other robots to
`Documents\MyAppData\MiKoBots\Robot_library_disabled`. Nothing is deleted. This version only
works with the ARM: it does this check at every start, so for the other robots use the
original MiKoBots Studio (and move their folders back into `Robot_library`).

## Connect the robot

1. Switch on the servo power supply of the robot and connect the robot's USB-C cable to the PC.
2. In MiKoBots Studio click **Connect robot**.
3. **USB:** click **Scan com ports**, choose the port of the robot (shown as *USB Serial Device
   (COMx)*), click **Connect to ROBOT**.
   **Bluetooth:** click **Scan bluetooth devices**, choose **ARM_TEST**, click
   **Connect to bluetooth ROBOT**.
4. The log shows `ROBOT_CONNECTED`. If it also shows `ERROR: PCA9685 servo driver not found`,
   check the cable between the XIAO and the servo driver board.
5. Click **Home** before the first move to a position.

Only one program can use the robot's COM port at a time: close Arduino IDE or a serial monitor
before connecting.

## Try it without a robot

Click the big **ROBOT** button under *Mode* so it shows **SIMULATION** (blue): the jog buttons
then move the 3D model instead of the robot, and the green **Run** button next to the program
runs the program on the model. Click it again to switch back to **ROBOT** (green).

## Programming

The dropdown above the program field chooses the program type: **Python**, **Blockly** or
**Text (Kawasaki AS / G-code)**. The Python field starts with a servo test that moves every
joint on its own; the Text field starts with a short Kawasaki / G-code example. All commands
are described in **PROGRAMMING.md** (English) and **PROGRAMMING.pl.md** (Polish), next to this
file in the project.

## Update or uninstall

- **Update:** run the new `MiKoBots Studio ARM setup.exe`; it replaces the old version.
- **Uninstall:** *Settings → Apps → MiKoBots Studio ARM → Uninstall*, or *Uninstall MiKoBots
  Studio ARM* in the Start menu.
- Your robot settings and programs stay in `Documents\MyAppData\MiKoBots`. Delete that folder for
  a completely fresh start: the program recreates it with the ARM robot.

## Robot firmware

The installer contains only the PC program. The robot's XIAO ESP32-S3 board needs the ARM
firmware from this repository (the files in the repository root). With
[arduino-cli](https://arduino.github.io/arduino-cli/) and the ESP32 core installed:

```powershell
arduino-cli compile -b esp32:esp32:XIAO_ESP32S3:USBMode=hwcdc -u -p COM10 arm
```

Use the robot's COM port instead of COM10. In VS Code the task **Flash XIAO ESP32-S3** does the
same.

## Troubleshooting

| Problem | What to do |
|---|---|
| "Windows protected your PC" | *More info → Run anyway* (the installer is not signed) |
| The robot's COM port is not listed | Check the USB-C cable (some are charge-only); try another USB port |
| "Access is denied" / port busy | Close any other program using the port (Arduino IDE, serial monitor) |
| `ROBOT_CONNECTED` but the servos do not move | Servo power supply on? Click **Home** first. The log shows the reason of a refused move. |
| `ERROR: PCA9685 servo driver not found` | Check SDA/SCL/3V3/GND between the XIAO and the driver board, then reconnect |
| Bluetooth: ARM_TEST not found | Disconnect the USB connection in the program, switch Bluetooth on, scan again |
