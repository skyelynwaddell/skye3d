-- sh_input.lua

-- keyboard / mouse controls
BTN_UP       = KEY.UP
BTN_DOWN     = KEY.DOWN
BTN_LEFT     = KEY.LEFT
BTN_RIGHT    = KEY.RIGHT
BTN_JUMP     = KEY.SPACE
BTN_RELOAD   = KEY.R
BTN_SHOOT    = MOUSE_BUTTON.LEFT
BTN_PAUSE    = KEY.ESCAPE
BTN_INTERACT = KEY.E

-- gamepad controls
GP_UP       = GAMEPAD_BUTTON.LEFT_FACE_UP
GP_DOWN     = GAMEPAD_BUTTON.LEFT_FACE_DOWN
GP_LEFT     = GAMEPAD_BUTTON.LEFT_FACE_LEFT
GP_RIGHT    = GAMEPAD_BUTTON.LEFT_FACE_RIGHT
GP_JUMP     = GAMEPAD_BUTTON.RIGHT_FACE_DOWN
GP_RELOAD   = GAMEPAD_BUTTON.RIGHT_FACE_LEFT
GP_SHOOT    = GAMEPAD_BUTTON.RIGHT_TRIGGER_2
GP_PAUSE    = GAMEPAD_BUTTON.MIDDLE_RIGHT
GP_INTERACT = GAMEPAD_BUTTON.RIGHT_FACE_UP

gp = 0  -- gamepad index

-- Get movement from Left Analog Stick
function analog_left()
  local _x = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.LEFT_X)
  local _y = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.LEFT_Y)
  return { x = _x, y = _y }
end

-- Get movement from Right Analog Stick
function analog_right()
  local _x = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.RIGHT_X)
  local _y = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.RIGHT_Y)
  return { x = _x, y = _y }
end

-- DPAD UP
function button_up_pressed()
  return is_key_pressed(BTN_UP) or is_gamepad_button_pressed(gp, GP_UP)
end
function button_up_released()
  return is_key_released(BTN_UP) or is_gamepad_button_released(gp, GP_UP)
end

-- DPAD DOWN
function button_down_pressed()
  return is_key_pressed(BTN_DOWN) or is_gamepad_button_pressed(gp, GP_DOWN)
end
function button_down_released()
  return is_key_released(BTN_DOWN) or is_gamepad_button_released(gp, GP_DOWN)
end

-- DPAD LEFT
function button_left_pressed()
  return is_key_pressed(BTN_LEFT) or is_gamepad_button_pressed(gp, GP_LEFT)
end
function button_left_released()
  return is_key_released(BTN_LEFT) or is_gamepad_button_released(gp, GP_LEFT)
end

-- DPAD RIGHT
function button_right_pressed()
  return is_key_pressed(BTN_RIGHT) or is_gamepad_button_pressed(gp, GP_RIGHT)
end
function button_right_released()
  return is_key_released(BTN_RIGHT) or is_gamepad_button_released(gp, GP_RIGHT)
end

-- SHOOT
function button_shoot_pressed()
  return is_mouse_button_pressed(BTN_SHOOT) or is_gamepad_button_pressed(gp, GP_SHOOT)
end
function button_shoot_released()
  return is_mouse_button_released(BTN_SHOOT) or is_gamepad_button_released(gp, GP_SHOOT)
end

-- JUMP
function button_jump_pressed()
  return is_key_pressed(BTN_JUMP) or is_gamepad_button_pressed(gp, GP_JUMP)
end
function button_jump_released()
  return is_key_released(BTN_JUMP) or is_gamepad_button_released(gp, GP_JUMP)
end

-- RELOAD
function button_reload_pressed()
  return is_key_pressed(BTN_RELOAD) or is_gamepad_button_pressed(gp, GP_RELOAD)
end
function button_reload_released()
  return is_key_released(BTN_RELOAD) or is_gamepad_button_released(gp, GP_RELOAD)
end

-- INTERACT
function button_interact_pressed()
  return is_key_pressed(BTN_INTERACT) or is_gamepad_button_pressed(gp, GP_INTERACT)
end
function button_interact_released()
  return is_key_released(BTN_INTERACT) or is_gamepad_button_released(gp, GP_INTERACT)
end

-- PAUSE
function button_pause_pressed()
  return is_key_pressed(BTN_PAUSE) or is_gamepad_button_pressed(gp, GP_PAUSE)
end
function button_pause_released()
  return is_key_released(BTN_PAUSE) or is_gamepad_button_released(gp, GP_PAUSE)
end
