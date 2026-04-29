-- sh_input.lua  — Rebindable input system.
-- Defines the BINDINGS table (read by button_*_pressed/released),
-- plus KEY_NAMES / MOUSE_NAMES helpers used by the controls UI.
-- Loaded in BOTH client and server VMs; client-API calls are inside
-- functions that the server never invokes, so no runtime errors occur.

-- ============================================================
--  Input type constants
-- ============================================================
INPUT_KEY        = "key"
INPUT_MOUSE      = "mouse"

-- Virtual mouse-wheel codes (match MOUSE_BUTTON.WHEEL_UP/DOWN and
-- EBIND_WHEEL_UP/DOWN in input_bindings.h)
MOUSE_WHEEL_UP   = 10
MOUSE_WHEEL_DOWN = 11

-- ============================================================
--  Key / mouse button display name tables
-- ============================================================
KEY_NAMES = {
  -- Letters
  [KEY.A]="A", [KEY.B]="B", [KEY.C]="C", [KEY.D]="D", [KEY.E]="E",
  [KEY.F]="F", [KEY.G]="G", [KEY.H]="H", [KEY.I]="I", [KEY.J]="J",
  [KEY.K]="K", [KEY.L]="L", [KEY.M]="M", [KEY.N]="N", [KEY.O]="O",
  [KEY.P]="P", [KEY.Q]="Q", [KEY.R]="R", [KEY.S]="S", [KEY.T]="T",
  [KEY.U]="U", [KEY.V]="V", [KEY.W]="W", [KEY.X]="X", [KEY.Y]="Y",
  [KEY.Z]="Z",
  -- Digits
  [KEY.ZERO]="0",  [KEY.ONE]="1",  [KEY.TWO]="2",  [KEY.THREE]="3",
  [KEY.FOUR]="4",  [KEY.FIVE]="5", [KEY.SIX]="6",  [KEY.SEVEN]="7",
  [KEY.EIGHT]="8", [KEY.NINE]="9",
  -- Special
  [KEY.SPACE]        = "Space",
  [KEY.ENTER]        = "Enter",
  [KEY.ESCAPE]       = "Escape",
  [KEY.TAB]          = "Tab",
  [KEY.BACKSPACE]    = "Backspace",
  [KEY.INSERT]       = "Insert",
  [KEY.DELETE]       = "Delete",
  [KEY.UP]           = "Up",
  [KEY.DOWN]         = "Down",
  [KEY.LEFT]         = "Left",
  [KEY.RIGHT]        = "Right",
  [KEY.PAGE_UP]      = "Page Up",
  [KEY.PAGE_DOWN]    = "Page Down",
  [KEY.HOME]         = "Home",
  [KEY.END]          = "End",
  [KEY.CAPS_LOCK]    = "Caps Lock",
  [KEY.PRINT_SCREEN] = "Print Screen",
  [KEY.GRAVE]        = "~",
  [KEY.MINUS]        = "-",
  [KEY.EQUAL]        = "=",
  [KEY.LEFT_BRACKET] = "[",
  [KEY.RIGHT_BRACKET]= "]",
  [KEY.BACKSLASH]    = "\\",
  [KEY.SEMICOLON]    = ";",
  [KEY.APOSTROPHE]   = "'",
  [KEY.COMMA]        = ",",
  [KEY.PERIOD]       = ".",
  [KEY.SLASH]        = "/",
  -- Modifiers
  [KEY.LEFT_SHIFT]   = "Left Shift",
  [KEY.LEFT_CONTROL] = "Left Ctrl",
  [KEY.LEFT_ALT]     = "Left Alt",
  [KEY.RIGHT_SHIFT]  = "Right Shift",
  [KEY.RIGHT_CONTROL]= "Right Ctrl",
  [KEY.RIGHT_ALT]    = "Right Alt",
  -- Function keys
  [KEY.F1]="F1",   [KEY.F2]="F2",   [KEY.F3]="F3",   [KEY.F4]="F4",
  [KEY.F5]="F5",   [KEY.F6]="F6",   [KEY.F7]="F7",   [KEY.F8]="F8",
  [KEY.F9]="F9",   [KEY.F10]="F10", [KEY.F11]="F11", [KEY.F12]="F12",
}

MOUSE_NAMES = {
  [MOUSE_BUTTON.LEFT]       = "Mouse Left",
  [MOUSE_BUTTON.RIGHT]      = "Mouse Right",
  [MOUSE_BUTTON.MIDDLE]     = "Mouse Middle",
  [MOUSE_BUTTON.SIDE]       = "Mouse Side",
  [MOUSE_BUTTON.EXTRA]      = "Mouse Extra",
  [MOUSE_BUTTON.WHEEL_UP]   = "Wheel Up",
  [MOUSE_BUTTON.WHEEL_DOWN] = "Wheel Down",
}

-- ============================================================
--  Default bindings
-- ============================================================
-- Each entry: { type, code, display, alt_type, alt_code, alt_display }
-- alt_code = -1 means no alternate bound.
BINDINGS_DEFAULT = {
  up         = { type=INPUT_KEY,   code=KEY.W,           display="W",
                 alt_type=INPUT_KEY, alt_code=KEY.UP,    alt_display="Up"    },
  down       = { type=INPUT_KEY,   code=KEY.S,           display="S",
                 alt_type=INPUT_KEY, alt_code=KEY.DOWN,  alt_display="Down"  },
  left       = { type=INPUT_KEY,   code=KEY.A,           display="A",
                 alt_type=INPUT_KEY, alt_code=KEY.LEFT,  alt_display="Left"  },
  right      = { type=INPUT_KEY,   code=KEY.D,           display="D",
                 alt_type=INPUT_KEY, alt_code=KEY.RIGHT, alt_display="Right" },
  jump       = { type=INPUT_KEY,   code=KEY.SPACE,       display="Space",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  reload     = { type=INPUT_KEY,   code=KEY.R,           display="R",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  interact   = { type=INPUT_KEY,   code=KEY.E,           display="E",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  shoot      = { type=INPUT_MOUSE, code=MOUSE_BUTTON.LEFT, display="Mouse Left",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  flashlight = { type=INPUT_KEY,   code=KEY.F,           display="F",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  walk       = { type=INPUT_KEY,   code=KEY.LEFT_SHIFT,  display="Left Shift",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  crouch     = { type=INPUT_KEY,   code=KEY.LEFT_CONTROL,display="Left Ctrl",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  scoreboard = { type=INPUT_KEY,   code=KEY.TAB,         display="Tab",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  chat       = { type=INPUT_KEY,   code=KEY.T,           display="T",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
  console    = { type=INPUT_KEY,   code=KEY.GRAVE,       display="~",
                 alt_type=INPUT_KEY, alt_code=-1,        alt_display=""      },
}

-- Live bindings table — copy of defaults at startup, mutated by the UI.
BINDINGS = {}

function reset_bindings_to_defaults()
  for k, v in pairs(BINDINGS_DEFAULT) do
    BINDINGS[k] = {
      type        = v.type,
      code        = v.code,
      display     = v.display,
      alt_type    = v.alt_type,
      alt_code    = v.alt_code,
      alt_display = v.alt_display,
    }
  end
end

-- ============================================================
--  Bindings persistence  (gamedata/bindings.cfg)
--
--  Format — one flat key=value per line, e.g.:
--    up_type=key
--    up_code=87
--    up_disp=W
--    up_atype=key
--    up_acode=38
--    up_adisp=Up
-- ============================================================
local BINDINGS_FILE = "gamedata/bindings.cfg"

function save_bindings()
  local f = io.open(BINDINGS_FILE, "w")
  if not f then
    print("[bindings] ERROR: could not write " .. BINDINGS_FILE)
    return
  end
  f:write("# Keybindings — auto-generated\n")
  for bkey, b in pairs(BINDINGS) do
    f:write(bkey .. "_type="  .. tostring(b.type        or "key") .. "\n")
    f:write(bkey .. "_code="  .. tostring(b.code        or -1)    .. "\n")
    f:write(bkey .. "_disp="  .. tostring(b.display     or "")    .. "\n")
    f:write(bkey .. "_atype=" .. tostring(b.alt_type    or "key") .. "\n")
    f:write(bkey .. "_acode=" .. tostring(b.alt_code    or -1)    .. "\n")
    f:write(bkey .. "_adisp=" .. tostring(b.alt_display or "")    .. "\n")
  end
  f:close()
end

-- Load saved bindings on top of whatever is currently in BINDINGS.
-- Safe to call before set_engine_binding (client init loop handles that).
function load_bindings()
  local f = io.open(BINDINGS_FILE, "r")
  if not f then return end  -- no save file yet — keep defaults
  local data = {}
  for line in f:lines() do
    if line:sub(1, 1) ~= "#" and line ~= "" then
      local k, v = line:match("^([^=]+)=(.*)$")
      if k then data[k] = v end
    end
  end
  f:close()
  for bkey, b in pairs(BINDINGS) do
    local t  = data[bkey .. "_type"]
    local c  = data[bkey .. "_code"]
    local d  = data[bkey .. "_disp"]
    local at = data[bkey .. "_atype"]
    local ac = data[bkey .. "_acode"]
    local ad = data[bkey .. "_adisp"]
    if t  then b.type        = t                  end
    if c  then b.code        = tonumber(c) or -1  end
    if d  then b.display     = d                  end
    if at then b.alt_type    = at                 end
    if ac then b.alt_code    = tonumber(ac) or -1 end
    if ad then b.alt_display = ad                 end
  end
end

reset_bindings_to_defaults()
load_bindings()   -- overlay persisted bindings on top of defaults

-- ============================================================
--  Gamepad constants
-- ============================================================
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

-- Pause is always Escape (not rebindable; the menu itself uses it)
BTN_PAUSE = KEY.ESCAPE

-- ============================================================
--  Type-aware press / release helpers (client only)
-- ============================================================

-- Checks a single (type, code) pair.
local function bind_pressed(type_, code)
  if not code or code < 0 then return false end
  if type_ == INPUT_MOUSE then
    if code == MOUSE_WHEEL_UP   then return get_mouse_wheel_move() > 0 end
    if code == MOUSE_WHEEL_DOWN then return get_mouse_wheel_move() < 0 end
    return is_mouse_button_pressed(code)
  end
  if code == 0 then return false end  -- KEY_NULL
  return is_key_pressed(code)
end

local function bind_released(type_, code)
  if not code or code < 0 then return false end
  if type_ == INPUT_MOUSE then
    if code == MOUSE_WHEEL_UP   then return get_mouse_wheel_move() == 0 end
    if code == MOUSE_WHEEL_DOWN then return get_mouse_wheel_move() == 0 end
    return is_mouse_button_released(code)
  end
  if code == 0 then return false end  -- KEY_NULL
  return is_key_released(code)
end

-- Checks both primary and alternate bindings.
local function btn_pressed(b)
  if not b then return false end
  return bind_pressed(b.type, b.code) or bind_pressed(b.alt_type, b.alt_code)
end

local function btn_released(b)
  if not b then return false end
  return bind_released(b.type, b.code) or bind_released(b.alt_type, b.alt_code)
end

-- ============================================================
--  Analog sticks
-- ============================================================
function analog_left()
  return { x = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.LEFT_X),
           y = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.LEFT_Y) }
end

function analog_right()
  return { x = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.RIGHT_X),
           y = get_gamepad_axis_movement(gp, GAMEPAD_AXIS.RIGHT_Y) }
end

-- ============================================================
--  Action functions — read from BINDINGS, fall back to gamepad
-- ============================================================
function button_up_pressed()      return btn_pressed(BINDINGS.up)       or is_gamepad_button_pressed(gp,  GP_UP)      end
function button_up_released()     return btn_released(BINDINGS.up)      or is_gamepad_button_released(gp, GP_UP)      end
function button_down_pressed()    return btn_pressed(BINDINGS.down)     or is_gamepad_button_pressed(gp,  GP_DOWN)    end
function button_down_released()   return btn_released(BINDINGS.down)    or is_gamepad_button_released(gp, GP_DOWN)    end
function button_left_pressed()    return btn_pressed(BINDINGS.left)     or is_gamepad_button_pressed(gp,  GP_LEFT)    end
function button_left_released()   return btn_released(BINDINGS.left)    or is_gamepad_button_released(gp, GP_LEFT)    end
function button_right_pressed()   return btn_pressed(BINDINGS.right)    or is_gamepad_button_pressed(gp,  GP_RIGHT)   end
function button_right_released()  return btn_released(BINDINGS.right)   or is_gamepad_button_released(gp, GP_RIGHT)   end
function button_jump_pressed()    return btn_pressed(BINDINGS.jump)     or is_gamepad_button_pressed(gp,  GP_JUMP)    end
function button_jump_released()   return btn_released(BINDINGS.jump)    or is_gamepad_button_released(gp, GP_JUMP)    end
function button_reload_pressed()  return btn_pressed(BINDINGS.reload)   or is_gamepad_button_pressed(gp,  GP_RELOAD)  end
function button_reload_released() return btn_released(BINDINGS.reload)  or is_gamepad_button_released(gp, GP_RELOAD)  end
function button_shoot_pressed()   return btn_pressed(BINDINGS.shoot)    or is_gamepad_button_pressed(gp,  GP_SHOOT)   end
function button_shoot_released()  return btn_released(BINDINGS.shoot)   or is_gamepad_button_released(gp, GP_SHOOT)   end
function button_interact_pressed()  return btn_pressed(BINDINGS.interact)  or is_gamepad_button_pressed(gp,  GP_INTERACT) end
function button_interact_released() return btn_released(BINDINGS.interact) or is_gamepad_button_released(gp, GP_INTERACT) end
function button_pause_pressed()   return is_key_pressed(BTN_PAUSE)      or is_gamepad_button_pressed(gp,  GP_PAUSE)   end
function button_pause_released()  return is_key_released(BTN_PAUSE)     or is_gamepad_button_released(gp, GP_PAUSE)   end
