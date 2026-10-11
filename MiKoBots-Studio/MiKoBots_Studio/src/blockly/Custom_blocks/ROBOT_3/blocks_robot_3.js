
Blockly.defineBlocksWithJsonArray([
  // block MoveJ
  {
    "type": "MoveJ_3",
    "tooltip": "",
    "helpUrl": "",
    "message0": "MoveJ %1 X %2 %3 Y %4 %5 Z %6 %7 v %8 %9 a %10 %11",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "X",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "X"
      },
      {
        "type": "field_number",
        "name": "Y",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Y"
      },
      {
        "type": "field_number",
        "name": "Z",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Z"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  },

  // block MoveL
  {
    "type": "MoveL_3",
    "tooltip": "",
    "helpUrl": "",
    "message0": "MoveL %1 X %2 %3 Y %4 %5 Z %6 %7 v %8 %9 a %10 %11",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "X",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "X"
      },
      {
        "type": "field_number",
        "name": "Y",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Y"
      },
      {
        "type": "field_number",
        "name": "Z",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Z"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  },

  // block OffsetJ
  {
    "type": "OffsetJ_3",
    "tooltip": "",
    "helpUrl": "",
    "message0": "OffsetJ %1 X %2 %3 Y %4 %5 Z %6 %7 v %8 %9 a %10 %11",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "X",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "X"
      },
      {
        "type": "field_number",
        "name": "Y",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Y"
      },
      {
        "type": "field_number",
        "name": "Z",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Z"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  },

  // block OffsetL
  {
    "type": "OffsetL_3",
    "tooltip": "",
    "helpUrl": "",
    "message0": "OffsetL %1 X %2 %3 Y %4 %5 Z %6 %7 v %8 %9 a %10 %11",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "X",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "X"
      },
      {
        "type": "field_number",
        "name": "Y",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Y"
      },
      {
        "type": "field_number",
        "name": "Z",
        "value": 0
      },
      {
        "type": "input_dummy",
        "name": "Z"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  }
  ,

  // block WristRoll (ARM): turns the gripper about its own axis, 0 = straight
  {
    "type": "WristRoll_3",
    "tooltip": "Wrist roll of the ARM, -90 to 90 deg (0 = gripper straight)",
    "helpUrl": "",
    "message0": "Wrist roll %1 deg %2 %3 v %4 %5 a %6 %7",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "deg",
        "value": 0,
        "min": -90,
        "max": 90
      },
      {
        "type": "input_dummy",
        "name": "deg"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  }
  ,

  // block WristPitch (ARM): gripper up / down, angle to the horizontal, 0 = level
  {
    "type": "WristPitch_3",
    "tooltip": "Wrist pitch of the ARM, -90 to 90 deg to the horizontal (0 = level, + = up)",
    "helpUrl": "",
    "message0": "Wrist pitch %1 deg %2 %3 v %4 %5 a %6 %7",
    "args0": [
      {
        "type": "input_dummy",
        "name": "NAME"
      },
      {
        "type": "field_number",
        "name": "deg",
        "value": 0,
        "min": -90,
        "max": 90
      },
      {
        "type": "input_dummy",
        "name": "deg"
      },
      {
        "type": "field_number",
        "name": "v",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "v"
      },
      {
        "type": "field_number",
        "name": "a",
        "value": 50,
        "min": 1,
        "max": 100
      },
      {
        "type": "input_dummy",
        "name": "a"
      }
    ],
    "previousStatement": null,
    "nextStatement": null,
    "colour": 15,
    "inputsInline": true
  }

])