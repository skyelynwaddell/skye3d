# Lua Scripting Docs

## [SV] Server Side

```lua
-- Networking
function start_server()                    -- call at start of sv_init
function get_player_count()                -- returns player count
function sendflags_sync()                  -- syncs sendflags to server
```

```lua
-- Instance creation & management
function instance_create(x, y, z)          -- spawns GameObject3D at world position
function instances_get_all()               -- returns table of all instances
function instances_get(name)               -- get instance by target_name
function get_player_instance(client_id)    -- returns player instance by client id
function get_instance_count()              -- get total amount of gameobjects spawned

-- Object methods
obj:destroy()
obj:get(var)
obj:set(var, value)

obj:set_think(think_func)     -- sets object's per-frame think/update function
obj:set_trigger(trigger_func) -- sets object's trigger callback
obj:on_trigger()              -- fires this object's trigger callback

obj:get_position()            -- returns {x=0, y=0, z=0}
obj:set_position(x, y, z)    -- sets world position

obj:get_velocity()            -- returns {x=0, y=0, z=0}
obj:set_velocity(x, y, z)    -- set current velocity

obj:get_acceleration()        -- returns acceleration vector
obj:set_acceleration(x, y, z)

obj:get_speed()               -- returns target speed (float)
obj:set_speed(speed)

obj:get_size()                -- returns {collision_box{xyz}, offset{xyz}}
obj:set_size({x,y,z}, {x,y,z}) -- collision box size and offset

obj:get_collision_box()       -- returns {width=0, height=0, length=0}
obj:set_collision_box(w, h, l)

obj:get_target_name()
obj:set_target_name(name)

obj:get_target()
obj:set_target(name)

obj:set_model(model_path)           -- load model by filepath
obj:set_model_scale(w, h, l)       -- default 1,1,1
obj:set_angle(angle)               -- set Y rotation angle

find_closest_object(max_dist)      -- returns nearest object within range
find_all_objects_in_range(max_dist) -- returns table of all objects in range
get_aabb()                         -- returns BoundingBox {min={xyz}, max={xyz}}
```

```lua
-- Entity / think example
function door_think(self, dt)
  print("think func!")
end

function door_trigger(self)
  print("triggered")
end

-- Called automatically when BSP loads an entity whose classname matches
function door(self, origin, tags)
  self:set_think(door_think)
  self:set_trigger(door_trigger)
end
```

---

## [SH] Shared

```lua
SIDE = "CLIENT"  -- or "SERVER"
```

```lua
-- Networking
function send_packet_number(client_id, event_name, value)  -- send a float
function send_packet_vector3(client_id, event_name, vec3)  -- send {x,y,z}
function get_packet()  -- returns {client_id=0, name="", data=nil}
```

```lua
-- Timing
function set_target_fps(fps)    -- 0 = unlimited
function get_target_fps()       -- returns current fps cap
function get_frame_time()       -- returns dt in seconds
function get_time()             -- returns elapsed time in seconds
function get_fps()              -- returns current measured fps
```

```lua
-- RNG
function set_random_seed(seed)
function get_random_value(min, max)  -- returns integer in [min, max]
```

---

## [CL] Client Side

```lua
-- Keyboard
function is_key_pressed(key)    -- true on the frame the key goes down
function is_key_down(key)       -- true every frame the key is held
function is_key_released(key)   -- true on the frame the key comes up
function is_key_up(key)         -- true when key is not held
function get_key_pressed()      -- returns keycode of most recent keypress, 0 if none
```

```lua
-- Mouse
function get_mouse_x()
function get_mouse_y()
function get_mouse_delta()         -- returns {x=0, y=0}
function get_mouse_wheel_move()    -- returns float (positive = scroll up)
function set_mouse_cursor(cursor_id)

function is_mouse_button_pressed(btn)
function is_mouse_button_down(btn)
function is_mouse_button_released(btn)
function is_mouse_button_up(btn)
```

```lua
-- Gamepad
function is_gamepad_available(i)
function get_gamepad_name(i)
function is_gamepad_button_pressed(i, b)
function is_gamepad_button_down(i, b)
function is_gamepad_button_released(i, b)
function is_gamepad_button_up(i, b)
function get_gamepad_button_pressed()
function get_gamepad_axis_count(i)
function get_gamepad_axis_movement(i, axis)
function set_gamepad_vibration(i, left, right, duration)
```

```lua
-- Cursor
function show_cursor()
function hide_cursor()
function is_cursor_hidden()   -- returns bool
function enable_cursor()
function disable_cursor()
function is_cursor_on_screen()
```

```lua
-- Window
function get_screen_width()           -- current window pixel width
function get_screen_height()          -- current window pixel height
function get_configured_width()       -- saved SCREEN_WIDTH (matches set_window_size)
function get_configured_height()      -- saved SCREEN_HEIGHT
function get_render_width()
function get_render_height()

function set_window_size("WxH")       -- e.g. "1920x1080" — MUST be called before BeginDrawing
                                      -- use pending_video_apply pattern (see Video Settings)
function set_window_mode(mode)        -- 0=windowed, 1=fullscreen, 2=borderless windowed
function set_window_title(title)
function set_window_position(x, y)
function set_window_icon(path)

function toggle_fullscreen()
function toggle_borderless_windowed()
function maximize_window()
function minimize_window()
function restore_window()
function quit_game()
```

```lua
-- Video Settings (getters read current engine state; use for syncing UI on open)
function get_brightness()        -- float 0.0–1.0 (post-process exposure)
function get_window_mode()       -- int 0/1/2
function get_vsync()             -- bool
function get_msaa4x()            -- bool (takes effect on restart)
function get_texture_filter()    -- int: 0=Point 1=Bilinear 2=Trilinear 3=Aniso4x 4=Aniso8x 5=Aniso16x
function get_fov()               -- int degrees
function get_gui_scale()         -- float multiplier applied to entire GUI FBO

function set_brightness(f)       -- immediately updates post-process exposure
function set_window_mode(i)      -- immediately changes window state
function set_vsync(b)            -- attempts runtime swap-interval change
function set_msaa4x(b)           -- saved; takes full effect on restart
function set_texture_filter(i)   -- re-applies to all loaded BSP textures immediately
function set_fov(i)              -- updates active camera fovy immediately
function set_gui_scale(f)        -- rescales the GUI render pass immediately
function save_settings()         -- writes all settings to gamedata/settings.cfg
```

```lua
-- Safe deferred window resize pattern
-- set_window_size() inside BeginDrawing causes FBO dimension mismatch (split screen).
-- Instead, write to the global pending_video_apply and consume it in draw() (cs_draw.lua)
-- BEFORE BeginDrawing:

-- cs_hud.lua (Apply button):
pending_video_apply = { res = "1920x1080", mode = 0 }

-- cs_draw.lua (called before BeginDrawing each frame):
function draw(dt)
  if pending_video_apply then
    local p = pending_video_apply
    pending_video_apply = nil
    if p.res then set_window_size(p.res) end
    set_window_mode(p.mode)
  end
end
```

```lua
-- Camera
function get_camera_position()   -- returns {x=0, y=0, z=0}
```

```lua
-- Drawing
function clear_background(color)
function begin_scissor_mode(x, y, w, h)
function end_scissor_mode()
function draw_fps(x, y)          -- enables the FPS counter and sets its position
```

```lua
-- Text
function load_font(path, size)   -- loads a TTF; drawn with draw_text from then on
function draw_text(text, x, y, size, color)
-- color can be a COLOR enum value or a custom table {r=255, g=210, b=50, a=255}

-- Example: resolution-scaled text
local scale  = get_gui_scale()
local lw     = get_screen_width() / scale
local lh     = get_screen_height() / scale
local sx     = lw / 1280          -- scale relative to 1280x720 design resolution
local sy     = lh / 720
draw_text("Hello", 50 * sx, 100 * sy, math.floor(40 * sy), COLOR.WHITE)
```

```lua
-- Shapes
function draw_rectangle(x, y, w, h, color)

-- Semi-transparent overlay example:
local bg = COLOR.BLACK
bg.a = 180
draw_rectangle(0, 0, 10000, 10000, bg)
```

```lua
-- Models
function draw_cube(x, y, z, w, h, l, color)
```

---

## Menu System (UI)

The menu system exposes panel-based UI drawn in the GUI FBO. All menu calls must
happen inside `draw_gui` (or functions called from it).

### MENU item types

```lua
MENU.BUTTON       -- clickable button; fires item.onpress()
MENU.LABEL        -- non-interactive text row
MENU.SEPARATOR    -- horizontal divider line
MENU.SLIDER       -- continuous float slider with a handle
MENU.SLIDERBAR    -- filled progress-bar style slider
MENU.PROGRESSBAR  -- read-only progress bar
MENU.CHECKBOX     -- toggle; reads/writes item.value_b
MENU.TOGGLE       -- single toggle button
MENU.DROPDOWN     -- drop-down list; reads/writes item.value_i (zero-based index)
                  -- item.text = "Option A;Option B;Option C"
MENU.SPINNER      -- integer stepper; reads/writes item.value_i
MENU.SPINNERF     -- float stepper;   reads/writes item.value_f
MENU.TEXTBOX      -- editable string;  reads/writes item.value_s
MENU.KEYBIND      -- two-cell keybind row; primary=item.value_s, alt=item.text2
MENU.KEYBIND_HEADER -- column header row for a keybind table
```

### MenuItem

```lua
local i = MenuItem.new()
i.type    = MENU.BUTTON          -- one of the MENU.* constants above
i.text    = "Label text"
i.onpress = function() ... end   -- BUTTON / KEYBIND: called on click

-- Value fields (read and written by the UI each frame)
i.value_f = 0.5                  -- SLIDER / SLIDERBAR / SPINNERF
i.value_b = true                 -- CHECKBOX / TOGGLE
i.value_i = 0                    -- DROPDOWN (index) / SPINNER (int)
i.value_s = "hello"              -- TEXTBOX (max 255 chars)

-- Range fields
i.min_f   = 0.0
i.max_f   = 1.0
i.step_f  = 0.1                  -- SPINNERF step size
i.min_i   = 0
i.max_i   = 100

-- Keybind only
i.text2   = "Alt key display"    -- alternate binding label

-- Textbox only
i.buf_size = 64                  -- max characters (default 256)
```

### PanelState

```lua
local state   = PanelState.new()
state.x       = 24       -- panel X position (draggable; updated by the panel)
state.y       = 24       -- panel Y position
state.w       = 400      -- panel width
state.h       = 300      -- panel height
state.active_tab = 0     -- currently selected tab index (0-based, read/write)
```

### ui_panel

```lua
-- Draws a titled, draggable, scrollable panel.
-- Returns true while open; false when the X button is clicked.
--
-- title       : "#flags#Title text"  flags are optional raygui icon codes
-- items       : table of MenuItem
-- state       : PanelState (position/size/tab persisted across frames)
-- item_h      : height in pixels of each item row
-- tabs        : (optional) table of tab label strings {"Tab1","Tab2",...}
-- bottom      : (optional) table of MenuItem drawn in a fixed bottom bar

local open = ui_panel(title, items, state, item_h [, tabs [, bottom]])

-- Example: simple options panel with two tabs and Apply/Cancel buttons
local state = PanelState.new()
state.x, state.y, state.w, state.h = 24, 24, 420, 300

local brightness = MenuItem.new()
brightness.type  = MENU.SLIDERBAR
brightness.text  = "Brightness"
brightness.value_f, brightness.min_f, brightness.max_f = 0.3, 0.0, 1.0

local vsync = MenuItem.new()
vsync.type    = MENU.CHECKBOX
vsync.text    = "VSync"
vsync.value_b = true

local apply_btn = MenuItem.new()
apply_btn.type    = MENU.BUTTON
apply_btn.text    = "Apply"
apply_btn.onpress = function()
  set_brightness(brightness.value_f)
  set_vsync(vsync.value_b)
  save_settings()
end

local cancel_btn = MenuItem.new()
cancel_btn.type    = MENU.BUTTON
cancel_btn.text    = "Cancel"
cancel_btn.onpress = function() options_open = false end

if options_open then
  if not ui_panel("#142#Options", {brightness, vsync}, state, 26,
                  {"Audio","Video"}, {apply_btn, cancel_btn}) then
    options_open = false  -- X button clicked
  end
end
```

### ui_item_list

```lua
-- Bare vertical list with no panel chrome (no title, no border, no scrollbar).
-- Good for embedding a list inside custom-drawn UI.
ui_item_list(items, x, y, w, h)
```

### set_menu_mode / is_menu_mode

```lua
-- Shows cursor + freezes player input while a menu is open.
set_menu_mode(true)   -- show cursor, freeze input
set_menu_mode(false)  -- hide cursor, resume input
is_menu_mode()        -- returns bool
```

### Dropdown example

```lua
local res = MenuItem.new()
res.type    = MENU.DROPDOWN
res.text    = "640x480;1280x720;1920x1080"
res.value_i = 1   -- zero-based: selects "1280x720" by default

-- Read chosen index each frame:
local chosen = RES_LABELS[res.value_i + 1]   -- Lua tables are 1-based
```

### Full video-settings pattern

```lua
-- Sync UI from engine state when the tab opens
local function sync_video()
  brightness.value_f  = get_brightness()
  vsync.value_b       = get_vsync()
  window_mode.value_i = get_window_mode()
  -- texture filter C++ enum is reversed vs dropdown:
  texture_filter.value_i = 5 - get_texture_filter()
end

-- Apply — defers window resize to avoid split-screen artifact
local function apply_video()
  set_brightness(brightness.value_f)
  set_vsync(vsync.value_b)
  set_texture_filter(5 - texture_filter.value_i)
  pending_video_apply = {
    res  = RES_LABELS[resolution.value_i + 1],
    mode = window_mode.value_i,
  }
end
```

---

## Input Bindings

```lua
-- Push a keybind change into the C++ engine binding table.
-- name    : binding key, e.g. "jump", "up", "shoot"
-- type    : INPUT_KEY or INPUT_MOUSE  (strings "key" / "mouse")
-- code    : raylib key or mouse-button integer; -1 = unbound
function set_engine_binding(name, type, code)
function set_engine_binding_alt(name, type, code)

-- Shared constants (defined in sh_input.lua)
INPUT_KEY        = "key"
INPUT_MOUSE      = "mouse"
MOUSE_WHEEL_UP   = 10    -- virtual code for scroll up
MOUSE_WHEEL_DOWN = 11    -- virtual code for scroll down

-- BINDINGS table (live, mutated by controls UI)
-- Each entry: { type, code, display, alt_type, alt_code, alt_display }
BINDINGS["jump"] = { type=INPUT_KEY, code=KEY.SPACE, display="Space",
                     alt_type=INPUT_KEY, alt_code=-1, alt_display="" }

-- Persist bindings to gamedata/bindings.cfg
function save_bindings()

-- Reset BINDINGS table to BINDINGS_DEFAULT values
function reset_bindings_to_defaults()
```

---

# Enums

## COLOR

```lua
-- Named colors (immutable engine values):
COLOR.LIGHTGRAY   COLOR.GRAY      COLOR.DARKGRAY
COLOR.YELLOW      COLOR.GOLD      COLOR.ORANGE
COLOR.PINK        COLOR.RED       COLOR.MAROON
COLOR.GREEN       COLOR.LIME      COLOR.DARKGREEN
COLOR.SKYBLUE     COLOR.BLUE      COLOR.DARKBLUE
COLOR.PURPLE      COLOR.VIOLET    COLOR.DARKPURPLE
COLOR.BEIGE       COLOR.BROWN     COLOR.DARKBROWN
COLOR.WHITE       COLOR.BLACK     COLOR.BLANK
COLOR.MAGENTA     COLOR.RAYWHITE

-- Custom color table — accepted anywhere a color is needed:
{ r = 255, g = 210, b = 50, a = 255 }

-- Custom color with transparency:
local c = COLOR.BLACK
c.a = 180
draw_rectangle(0, 0, 9999, 9999, c)
```

## MOUSE_BUTTON

```lua
MOUSE_BUTTON.LEFT    = 0
MOUSE_BUTTON.RIGHT   = 1
MOUSE_BUTTON.MIDDLE  = 2
MOUSE_BUTTON.SIDE    = 3
MOUSE_BUTTON.EXTRA   = 4
MOUSE_BUTTON.FORWARD = 5
MOUSE_BUTTON.BACK    = 6
```

## GAMEPAD_BUTTON

```lua
GAMEPAD_BUTTON.LEFT_FACE_UP     = 1
GAMEPAD_BUTTON.LEFT_FACE_RIGHT  = 2
GAMEPAD_BUTTON.LEFT_FACE_DOWN   = 3
GAMEPAD_BUTTON.LEFT_FACE_LEFT   = 4
GAMEPAD_BUTTON.RIGHT_FACE_UP    = 5
GAMEPAD_BUTTON.RIGHT_FACE_RIGHT = 6
GAMEPAD_BUTTON.RIGHT_FACE_DOWN  = 7
GAMEPAD_BUTTON.RIGHT_FACE_LEFT  = 8
GAMEPAD_BUTTON.LEFT_TRIGGER_1   = 9
GAMEPAD_BUTTON.LEFT_TRIGGER_2   = 10
GAMEPAD_BUTTON.RIGHT_TRIGGER_1  = 11
GAMEPAD_BUTTON.RIGHT_TRIGGER_2  = 12
GAMEPAD_BUTTON.MIDDLE_LEFT      = 13
GAMEPAD_BUTTON.MIDDLE           = 14
GAMEPAD_BUTTON.MIDDLE_RIGHT     = 15
```

## GAMEPAD_AXIS

```lua
GAMEPAD_AXIS.LEFT_X       = 1
GAMEPAD_AXIS.LEFT_Y       = 2
GAMEPAD_AXIS.RIGHT_X      = 3
GAMEPAD_AXIS.RIGHT_Y      = 4
GAMEPAD_AXIS.LEFT_TRIGGER = 5
GAMEPAD_AXIS.RIGHT_TRIGGER= 6
```

## PACKET_TYPE

```lua
PACKET_TYPE.NUMBER  = 0
PACKET_TYPE.VECTOR3 = 1
PACKET_TYPE.STRING  = 2
PACKET_TYPE.BOOL    = 3
```

## KEY

```lua
KEY.NULL = 0

-- Alphanumeric
KEY.APOSTROPHE=39  KEY.COMMA=44    KEY.MINUS=45    KEY.PERIOD=46
KEY.SLASH=47       KEY.ZERO=48     KEY.ONE=49      KEY.TWO=50
KEY.THREE=51       KEY.FOUR=52     KEY.FIVE=53     KEY.SIX=54
KEY.SEVEN=55       KEY.EIGHT=56    KEY.NINE=57     KEY.SEMICOLON=59
KEY.EQUAL=61

KEY.A=65  KEY.B=66  KEY.C=67  KEY.D=68  KEY.E=69  KEY.F=70
KEY.G=71  KEY.H=72  KEY.I=73  KEY.J=74  KEY.K=75  KEY.L=76
KEY.M=77  KEY.N=78  KEY.O=79  KEY.P=80  KEY.Q=81  KEY.R=82
KEY.S=83  KEY.T=84  KEY.U=85  KEY.V=86  KEY.W=87  KEY.X=88
KEY.Y=89  KEY.Z=90

KEY.LEFT_BRACKET=91   KEY.BACKSLASH=92   KEY.RIGHT_BRACKET=93   KEY.GRAVE=96

-- Control
KEY.SPACE=32      KEY.ESCAPE=256    KEY.ENTER=257     KEY.TAB=258
KEY.BACKSPACE=259 KEY.INSERT=260    KEY.DELETE=261

-- Arrow
KEY.RIGHT=262  KEY.LEFT=263  KEY.DOWN=264  KEY.UP=265

-- Navigation
KEY.PAGE_UP=266   KEY.PAGE_DOWN=267  KEY.HOME=268  KEY.END=269

-- Lock
KEY.CAPS_LOCK=280  KEY.SCROLL_LOCK=281  KEY.NUM_LOCK=282
KEY.PRINT_SCREEN=283  KEY.PAUSE=284

-- Function
KEY.F1=290   KEY.F2=291   KEY.F3=292   KEY.F4=293
KEY.F5=294   KEY.F6=295   KEY.F7=296   KEY.F8=297
KEY.F9=298   KEY.F10=299  KEY.F11=300  KEY.F12=301

-- Modifiers
KEY.LEFT_SHIFT=340    KEY.LEFT_CONTROL=341  KEY.LEFT_ALT=342   KEY.LEFT_SUPER=343
KEY.RIGHT_SHIFT=344   KEY.RIGHT_CONTROL=345 KEY.RIGHT_ALT=346  KEY.RIGHT_SUPER=347
KEY.KB_MENU=348

-- Keypad
KEY.KP_0=320  KEY.KP_1=321  KEY.KP_2=322  KEY.KP_3=323  KEY.KP_4=324
KEY.KP_5=325  KEY.KP_6=326  KEY.KP_7=327  KEY.KP_8=328  KEY.KP_9=329
KEY.KP_DECIMAL=330  KEY.KP_DIVIDE=331  KEY.KP_MULTIPLY=332
KEY.KP_SUBTRACT=333 KEY.KP_ADD=334    KEY.KP_ENTER=335  KEY.KP_EQUAL=336

-- Android
KEY.BACK=4  KEY.MENU=82  KEY.VOLUME_UP=24  KEY.VOLUME_DOWN=25
```
