# Lua Scripting Docs

## [SV] Server Side

```lua
-- Networking
function start_server() -- call at start of sv_init
function get_player_count() -- returns player count
function sendflags_sync() -- syncs sendflags to server
```

```lua
-- Instance creation & management
function instance_create(x, y, z) -- spawns GameObject3D
function instances_get_all() -- returns all instances
function instances_get(name) -- get by target_name
function get_player_instance(client_id) -- returns player intance by client id
function get_instance_count() -- get total amount of gameobjects spawned

-- Object methods
function obj:destroy()
function obj:get(var)
function obj:set(var, value)

function obj:set_think(think_func) -- sets objects think/update func
function obj:set_trigger(trigger_func) -- sets objects trigger func
function obj:on_trigger() -- triggers an objects 'on_trigger' func

function obj:get_position() -- returns {x=0,y=0,z=0}
function obj:set_position(x,y,z) -- sets the world position of the object

function obj:get_velocity() -- returns {x=0,y=0,z=0}
function obj:set_velocity(x,y,z) -- set how fast we are currently moving

function obj:get_acceleration() -- returns float of our acceleration speed
function obj:set_acceleration(x,y,z) -- set how fast we accelerate from standing still

function obj:get_speed() -- returns object target speed
function obj:set_speed(speed) -- set object target speed

function obj:get_size() -- returns {collision_box{xyz}, offset{xyz}}
function obj:set_size({x,y,z}, {x,y,z}) -- collisionbox, and collision_offset - set collision box box size

function obj:get_collision_box() -- returns {width=0,height=0,length=0}
function obj:set_collision_box(w,h,l) -- set collision size

function obj:get_target_name() -- gets our targetname
function obj:set_target_name(name) -- set our targetname

function obj:get_target()   -- gets who we are targeting
function obj:set_target(name)   -- sets who we should target

function obj:set_model(model_path) -- set model by filepath, client must also have file in same dir
function obj:set_model_scale(w, h, l) -- set model scale (default 1,1,1)

function obj:set_angle(angle) -- set the Y rotation angle of a object

function find_closest_object(max_dist)  -- returns single closest object in range to caller
function find_all_objects_in_range(max_dist) -- returns an array of all objects in range to caller
function get_aabb() -- returns BoundingBox {Vector3 min, Vector3 max}
```

```lua
-- think/update/step event (called every frame)
function door_think(self, dt)
    print("think func! :)");
end

-- on_trigger function
-- what happens when something calles on_trigger on us
function door_trigger(self)
    print("i've been triggered")
end

-- when the map loads, and an entity with a classname is found,
-- it will automatically call the function with the same classname as the entity.
-- with the self,origin, and tags the entity has ie. door(self, origin, tags)
-- it will be called for each entity with that classname
function door(self, origin, tags)
    self:set_think(player_think)        -- step/update/think event
    self:set_trigger(player_trigger)    -- what happens when this object is triggered
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
function send_packet_number(client_id, event_name, data)
function send_packet_vector3(client_id, event_name, vec3)
function get_packet() -- returns {client_id=0, name="", data=nil}
end
```

```lua
-- Timing
function set_target_fps(fps)
function get_frame_time()
function get_time()
function get_fps()
```

```lua
-- RNG
function set_random_seed(seed) math.randomseed(seed)
function get_random_value(min, max) -- returns math.random(min, max)
end
```

---

## [CL] Client Side

```lua
-- Networking
function connect_to_server()
```

```lua
-- Input (Keyboard)
function is_key_pressed(key)
function is_key_down(key)
function is_key_released(key)
function is_key_up(key)

function get_mouse_x()
function get_mouse_y()
function get_mouse_delta()
function get_mouse_wheel_move()

function set_mouse_cursor(cursor_id)
```

```lua
-- Mouse buttons
function is_mouse_button_pressed(btn)
function is_mouse_button_down(btn)
function is_mouse_button_released(btn)
function is_mouse_button_up(btn)
```

```lua
-- Gamepad
function is_gamepad_available(i)
function get_gamepad_name(i)

function is_gamepad_button_pressed(i,b)
function is_gamepad_button_down(i,b)
function is_gamepad_button_released(i,b)
function is_gamepad_button_up(i,b)

function get_gamepad_button_pressed()
function get_gamepad_axis_count(i)
function get_gamepad_axis_movement(i,a)

function set_gamepad_vibration(i,l,r,d)
```

```lua
-- Window
function toggle_fullscreen()
function toggle_borderless_windowed()
function maximize_window()
function minimize_window()
function restore_window()

function set_window_icon(path)
function set_window_title(title)
function set_window_position(x,y)

function get_screen_width()
function get_screen_height()
function get_render_width()
function get_render_height()
```

```lua
-- Cursor
function show_cursor()
function hide_cursor()
function is_cursor_hidden()

function enable_cursor()
function disable_cursor()
function is_cursor_on_screen()
```

```lua
-- System
function get_camera_position() -- returns {x=0,y=0,z=0}
```

```lua
-- Drawing
function clear_background(color)

function begin_scissor_mode(x,y,w,h)
function end_scissor_mode()
```

```lua
-- Text
function load_font(file, size)
function draw_text(text,x,y,size,color)
```

```lua
-- Shapes
function draw_rectangle(x,y,w,h,color)
```

```lua
-- Models
function draw_cube(x,y,z,w,h,l)
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
    NULL = 0,

    -- Alphanumeric
    APOSTROPHE = 39,
    COMMA = 44,
    MINUS = 45,
    PERIOD = 46,
    SLASH = 47,
    ZERO = 48,
    ONE = 49,
    TWO = 50,
    THREE = 51,
    FOUR = 52,
    FIVE = 53,
    SIX = 54,
    SEVEN = 55,
    EIGHT = 56,
    NINE = 57,
    SEMICOLON = 59,
    EQUAL = 61,

    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70,
    G = 71, H = 72, I = 73, J = 74, K = 75, L = 76,
    M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82,
    S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,

    LEFT_BRACKET = 91,
    BACKSLASH = 92,
    RIGHT_BRACKET = 93,
    GRAVE = 96,

    -- Function keys
    SPACE = 32,
    ESCAPE = 256,
    ENTER = 257,
    TAB = 258,
    BACKSPACE = 259,
    INSERT = 260,
    DELETE = 261,

    RIGHT = 262,
    LEFT = 263,
    DOWN = 264,
    UP = 265,

    PAGE_UP = 266,
    PAGE_DOWN = 267,
    HOME = 268,
    END = 269,

    CAPS_LOCK = 280,
    SCROLL_LOCK = 281,
    NUM_LOCK = 282,
    PRINT_SCREEN = 283,
    PAUSE = 284,

    F1 = 290, F2 = 291, F3 = 292, F4 = 293,
    F5 = 294, F6 = 295, F7 = 296, F8 = 297,
    F9 = 298, F10 = 299, F11 = 300, F12 = 301,

    LEFT_SHIFT = 340,
    LEFT_CONTROL = 341,
    LEFT_ALT = 342,
    LEFT_SUPER = 343,
    RIGHT_SHIFT = 344,
    RIGHT_CONTROL = 345,
    RIGHT_ALT = 346,
    RIGHT_SUPER = 347,
    KB_MENU = 348,

    -- Keypad
    KP_0 = 320,
    KP_1 = 321,
    KP_2 = 322,
    KP_3 = 323,
    KP_4 = 324,
    KP_5 = 325,
    KP_6 = 326,
    KP_7 = 327,
    KP_8 = 328,
    KP_9 = 329,
    KP_DECIMAL = 330,
    KP_DIVIDE = 331,
    KP_MULTIPLY = 332,
    KP_SUBTRACT = 333,
    KP_ADD = 334,
    KP_ENTER = 335,
    KP_EQUAL = 336,

    -- Android
    BACK = 4,
    MENU = 82,
    VOLUME_UP = 24,
    VOLUME_DOWN = 25
}
```
