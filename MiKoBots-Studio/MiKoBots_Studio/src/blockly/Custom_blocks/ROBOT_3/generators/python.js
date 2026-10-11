// block MoveJ
python.pythonGenerator.forBlock['MoveJ_3'] = function(block, generator) 
{
  const number_x = block.getFieldValue('X');
  const number_y = block.getFieldValue('Y');
  const number_z = block.getFieldValue('Z');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.MoveJ([${number_x}, ${number_y}, ${number_z}], ${number_v}, ${number_a})\n`;
  return code;
}

// block MoveL
python.pythonGenerator.forBlock['MoveL_3'] = function(block, generator) 
{
  const number_x = block.getFieldValue('X');
  const number_y = block.getFieldValue('Y');
  const number_z = block.getFieldValue('Z');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.MoveL([${number_x}, ${number_y}, ${number_z}], ${number_v}, ${number_a})\n`;
  return code;
}

// block OffsetJ
python.pythonGenerator.forBlock['OffsetJ_3'] = function(block, generator) 
{
  const number_x = block.getFieldValue('X');
  const number_y = block.getFieldValue('Y');
  const number_z = block.getFieldValue('Z');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.OffsetJ([${number_x}, ${number_y}, ${number_z}], ${number_v}, ${number_a})\n`;
  return code;
}

// block OffsetL
python.pythonGenerator.forBlock['OffsetL_3'] = function(block, generator) 
{
  const number_x = block.getFieldValue('X');
  const number_y = block.getFieldValue('Y');
  const number_z = block.getFieldValue('Z');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.OffsetL([${number_x}, ${number_y}, ${number_z}], ${number_v}, ${number_a})\n`;
  return code;
}
// block WristRoll (ARM)
python.pythonGenerator.forBlock['WristRoll_3'] = function(block, generator) 
{
  const number_deg = block.getFieldValue('deg');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.WristRoll(${number_deg}, ${number_v}, ${number_a})\n`;
  return code;
}

// block WristPitch (ARM)
python.pythonGenerator.forBlock['WristPitch_3'] = function(block, generator) 
{
  const number_deg = block.getFieldValue('deg');
  const number_v = block.getFieldValue('v');
  const number_a = block.getFieldValue('a');

  const code = `robot.WristPitch(${number_deg}, ${number_v}, ${number_a})\n`;
  return code;
}
