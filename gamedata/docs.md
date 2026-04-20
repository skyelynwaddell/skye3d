Here’s your documentation rewritten cleanly in **Lua-style Markdown (MD) format**, keeping everything consistent and idiomatic:

---

# Lua Scripting Docs

## [SV] Server Side

```lua
-- Networking
function start_server() end -- call at start of sv_init
function get_player_count() return 0 end -- returns player count
function sendflags_sync() end -- syncs sendflags to server
```

```lua
-- Instance creation & management
function instance_create(x, y, z) return {} end -- spawns GameObject3D
function instances_get_all() return {} end -- returns all instances
function instances_get(name) return {} end -- get by target_name
function get_player_instance(client_id) return {} end
function get_instance_count() return 0 end

-- Object methods
obj = {}

function obj:destroy() end
function obj:get(var) return nil end
function obj:set(var, value) end
```

```lua
-- Transformation & physics
function obj:get_position() return {x=0,y=0,z=0} end
function obj:set_position(x,y,z) end

function obj:get_velocity() return {x=0,y=0,z=0} end
function obj:set_velocity(x,y,z) end

function obj:get_acceleration() return {x=0,y=0,z=0} end
function obj:set_acceleration(x,y,z) end

function obj:get_speed() return 0 end
function obj:set_speed(speed) end

function obj:get_size() return {x=0,y=0,z=0} end
function obj:set_size(x,y,z) end
```

```lua
-- Collision
function obj:get_collision_box()
    return {width=0,height=0,length=0}
end

function obj:set_collision_box(w,h,l) end
```

```lua
-- Targeting & identification
function obj:get_target_name() return "" end
function obj:set_target_name(name) end

function obj:get_target() return "" end
function obj:set_target(name) end
```

```lua
-- Example: Think function
function player_think(self, dt)
    print("name: " .. tostring(self:get("name")))
    print("health: " .. tostring(self:get("health")))
    print("mana: " .. tostring(self:get("mana")))
    print("eliminated: " .. tostring(self:get("eliminated")))
end

function info_player_start(self, origin, tags)
    self:set_think(player_think)
end
```

---

## [SH] Shared

```lua
-- Variables
SIDE = "CLIENT" -- or "SERVER"
```

```lua
-- Networking
function send_packet_number(client_id, event_name, data) end
function send_packet_vector3(client_id, event_name, vec3) end
function get_packet()
    return {client_id=0, name="", data=nil}
end
```

```lua
-- Timing
function set_target_fps(fps) end
function get_frame_time() return 0 end
function get_time() return os.time() end
function get_fps() return 0 end
```

```lua
-- RNG
function set_random_seed(seed) math.randomseed(seed) end
function get_random_value(min, max)
    return math.random(min, max)
end
```

---

## [CL] Client Side

```lua
-- Networking
function connect_to_server() end
```

```lua
-- Input (Keyboard)
function is_key_pressed(key) return false end
function is_key_down(key) return false end
function is_key_released(key) return false end
function is_key_up(key) return true end

function get_mouse_x() return 0 end
function get_mouse_y() return 0 end
function get_mouse_delta() return 0 end
function get_mouse_wheel_move() return 0 end

function set_mouse_cursor(cursor_id) end
```

```lua
-- Mouse buttons
function is_mouse_button_pressed(btn) return false end
function is_mouse_button_down(btn) return false end
function is_mouse_button_released(btn) return false end
function is_mouse_button_up(btn) return true end
```

```lua
-- Gamepad
function is_gamepad_available(i) return false end
function get_gamepad_name(i) return "" end

function is_gamepad_button_pressed(i,b) return false end
function is_gamepad_button_down(i,b) return false end
function is_gamepad_button_released(i,b) return false end
function is_gamepad_button_up(i,b) return true end

function get_gamepad_button_pressed() return 0 end
function get_gamepad_axis_count(i) return 0 end
function get_gamepad_axis_movement(i,a) return 0 end

function set_gamepad_vibration(i,l,r,d) end
```

```lua
-- Window
function toggle_fullscreen() end
function toggle_borderless_windowed() end
function maximize_window() end
function minimize_window() end
function restore_window() end

function set_window_icon(path) end
function set_window_title(title) end
function set_window_position(x,y) end

function get_screen_width() return 0 end
function get_screen_height() return 0 end
function get_render_width() return 0 end
function get_render_height() return 0 end
```

```lua
-- Cursor
function show_cursor() end
function hide_cursor() end
function is_cursor_hidden() return false end

function enable_cursor() end
function disable_cursor() end
function is_cursor_on_screen() return true end
```

```lua
-- System
function get_camera_position()
    return {x=0,y=0,z=0}
end
```

```lua
-- Drawing
function clear_background(color) end

function begin_scissor_mode(x,y,w,h) end
function end_scissor_mode() end
```

```lua
-- Text
function load_font(file, size) end
function draw_text(text,x,y,size,color) end
```

```lua
-- Shapes
function draw_rectangle(x,y,w,h,color) end
```

```lua
-- Models
function draw_cube(x,y,z,w,h,l) end
```

---

# Enums (Lua Style)

## COLOR

```lua
COLOR = {
    LIGHTGRAY=1, GRAY=2, DARKGRAY=3,
    YELLOW=4, GOLD=5, ORANGE=6,
    PINK=7, RED=8, MAROON=9,
    GREEN=10, LIME=11, DARKGREEN=12,
    SKYBLUE=13, BLUE=14, DARKBLUE=15,
    PURPLE=16, VIOLET=17, DARKPURPLE=18,
    BEIGE=19, BROWN=20, DARKBROWN=21,
    WHITE=22, BLACK=23, BLANK=24,
    MAGENTA=25, RAYWHITE=26
}
```

## PACKET_TYPE

```lua
PACKET_TYPE = {
    NUMBER = 0,
    VECTOR3 = 1,
    STRING = 2,
    BOOL = 3
}
```

## MOUSE_BUTTON

```lua
MOUSE_BUTTON = {
    LEFT=0, RIGHT=1, MIDDLE=2,
    SIDE=3, EXTRA=4, FORWARD=5, BACK=6
}
```

## GAMEPAD_BUTTON

```lua
GAMEPAD_BUTTON = {
    LEFT_FACE_UP=1, LEFT_FACE_RIGHT=2, LEFT_FACE_DOWN=3, LEFT_FACE_LEFT=4,
    RIGHT_FACE_UP=5, RIGHT_FACE_RIGHT=6, RIGHT_FACE_DOWN=7, RIGHT_FACE_LEFT=8,
    LEFT_TRIGGER_1=9, LEFT_TRIGGER_2=10,
    RIGHT_TRIGGER_1=11, RIGHT_TRIGGER_2=12,
    MIDDLE_LEFT=13, MIDDLE=14, MIDDLE_RIGHT=15
}
```

## GAMEPAD_AXIS

```lua
GAMEPAD_AXIS = {
    LEFT_X=1, LEFT_Y=2,
    RIGHT_X=3, RIGHT_Y=4,
    LEFT_TRIGGER=5, RIGHT_TRIGGER=6
}
```

## KEY (partial example)

```lua
KEY = {
    SPACE = 32,
    ENTER = 257,
    ESCAPE = 256,

    A = 65, B = 66, C = 67,
    D = 68, E = 69, F = 70,
    -- continue as needed...
}
```
