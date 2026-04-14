# Scripting Docs

### [SV] Server Side

```cpp
// Networking
void server.start_server(); // call at start of sv_init.nut
int server.get_player_count(); // returns player count of the connected server
```

```cpp
// instance creation & sub funcs
local obj = server.instance_create(float x, float y, float z); // spawns a GameObject3D
local objs = server.instances_get_all(); // returns an array of ALL gameobject references
local objs = server.instances_get(string target_name); // returns an array of references to gameobjects with the same target_name (use "player" to return an array of all the players)
local player = server.get_player_instance(int client_id); // returns reference of player object instance by client_id
int server.get_instance_count(); // returns amount of spawned GameObject3D's in the world (including players)

obj.destroy();  // destroys the GameObject3D
obj.get(string variable_name); // gets a value of type [int, float, string, bool] by name
obj.set(string variable_name, variant<int, float, string, bool> value); // sets a value of either [int, float, string, bool] by name

// transformation & physics
obj.get_position(); // returns Vector3 table {x, y, z}
obj.set_position(float x, float y, float z);
obj.get_velocity(); // returns Vector3 table {x, y, z}
obj.set_velocity(float x, float y, float z);
obj.get_acceleration(); // returns Vector3 table {x, y, z}
obj.set_acceleration(float x, float y, float z);
obj.get_speed(); // returns float
obj.set_speed(float speed);
obj.get_size(); // returns Vector3 table {x, y, z}
obj.set_size(float x, float y, float z);

// collision [position + collision_box = collision_area]
obj.get_collision_box(); // returns table {width,height,length}
obj.set_collision_box(float width, float height, float length);

// targeting & identification
obj.get_target_name();  // gets their target_name string
obj.set_target_name(string name);
obj.get_target(); // gets their target string
obj.set_target(string name);

// Create a function for an instance & run it
function helloworld()
{
    // 'this' refers to the caller of this function
    print("name: " + this.get("name"));
    print("health: " + this.get("health"));
    print("mana: " + this.get("mana"));
    print("eliminated: " + this.get("eliminated"));
};
obj = server.instance_create(0, 0, 0);
obj.set("name", "skye");
obj.set("health", 100);
obj.set("mana", 3.4);
obj.set("eliminated", false);
obj.helloworld();

```

### [CL] Client Side

```cpp

```

### [SH] Shared

```cpp
// Variables
string side_name; // returns either CLIENT or SERVER side depending on if user is host

// Networking
void connect_to_server(); // connects client to a server
void sendflags_sync(); // syncs the updated sendflags variable to server
void send_packet_number(int client_id, string event_name, float data); // sends a packet to Server or Client
void send_packet_vector3(int client_id, string event_name, Vector3 data); // sends a packet to Server or Client
Packet get_packet(); // returns a Packet object {client_id, name, data} when received from Server or Client

// Input Keys
bool is_key_pressed(int key); // returns true the frame the key is pressed
bool is_key_down(int key); // returns true if key is down
bool is_key_released(int key); // returns true the frame they key is released
bool is_key_up(int key); // returns true if the key is up
float get_mouse_x(); // returns mouse_x
float get_mouse_y(); // returns mouse_y
float get_mouse_delta(); // returns mouse delta
float get_mouse_wheel_move(); // returns mouse wheel move
void set_mouse_cursor(int cursor_id); // set the mouse cursor by cursor id

// Input Mouse
bool is_mouse_button_pressed(int key); // returns true the frame the mb is pressed
bool is_mouse_button_down(int key); // returns true if mb is down
bool is_mouse_button_released(int key); // returns true the frame the key is released
bool is_mouse_button_up(int key); // returns true if they mb is up

// Gamepad
bool is_gamepad_available(int index);
string get_gamepad_name(int index);
bool is_gamepad_button_pressed(int index, int button);
bool is_gamepad_button_down(int index, int button);
bool is_gamepad_button_released(int index, int button);
bool is_gamepad_button_up(int index, int button);
int get_gamepad_button_pressed(); // returns the enum value of the last pressed button
int get_gamepad_axis_count(int index);
float get_gamepad_axis_movement(int index, int axis); // returns -1.0 to 1.0
void set_gamepad_vibration(int index, float left_motor, float right_motor, float duration);

// System
Vector3 get_camera_position(); // Gets the 3d camera's x y z position
void toggle_fullscreen(); // Toggles window fullscreen
void toggle_borderless_windowed(); // Toggles window bordless windowed
void maximize_window(); // Maximizes window
void minimize_window(); // Minimizes window
void restore_window(); // Restores window
void set_window_icon(string filepath); // Set window icon by filepath
void set_window_title(string title); // Set window title
void set_window_position(float x, float y); // Set window position by xy
float get_screen_width(); // returns window width
float get_screen_height(); // returns window height
float get_render_width(); // returns render width
float get_render_height(); // returns render height

// Cursor
void show_cursor(); // shows cursor if hidden
void hide_cursor(); // hides cursor if visible
bool is_cursor_hidden(); // returns true if cursor is hidden
void enable_cursor(); // enable cursor
void disable_cursor(); // disable cursor
bool is_cursor_on_screen(); // returns true if the mouse cursor is currently on screen

// Drawing
void clear_background(Color color); // clears background with a Color [ie. COLOR.GOLD]
void begin_scissor_mode(int x, int y, int width, int height); // begin scissor mode
void end_scissor_mode(); // end scissor mode

// Timing
void set_target_fps(int target_fps); // set the windows target fps (doesn't mean fps will be it)
float get_frame_time(); // delta time
float get_time(); // returns current time of pc
float get_fps(); // returns current fps game is running at

// RNG
void set_random_seed(int new_seed); // sets a new random seed of choice
int get_random_value(int min, int max) // returns an inclusive range of min-max

// RText
void load_font(string filename, int font_size); // load a font from file location
void draw_text(string text, int x, int y, int font_size, Color color); // draw text on screen

// RShapes
void draw_rectangle(int x, int y, int width, int height, Color color); // draws a 2d rectangle

// RModels
void draw_cube(float x, float y, float z, float width, float height, float length);

```

# Enums

### COLOR

```c
// [ie. COLOR.GOLD]
// Colors
enum COLOR {
    LIGHTGRAY,
    GRAY,
    DARKGRAY,
    YELLOW,
    GOLD,
    ORANGE,
    PINK,
    RED,
    MAROON,
    GREEN,
    LIME,
    DARKGREEN,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    PURPLE,
    VIOLET,
    DARKPURPLE,
    BEIGE,
    BROWN,
    DARKBROWN,
    WHITE,
    BLACK,
    BLANK,
    MAGENTA,
    RAYWHITE,
};

```

### PACKET

```c
// [ie. PACKET.NUMBER]
// Packets
enum PACKET {
    NUMBER = 0,
    VECTOR3 = 1,
    STRING = 2,
    BOOL = 3,
};
```

### MOUSE_BUTTON

```c
// [ie. MOUSE_BUTTON.LEFT]
// Mouse buttons
enum MOUSE_BUTTON {
    LEFT    = 0,       // Mouse button left
    RIGHT   = 1,       // Mouse button right
    MIDDLE  = 2,       // Mouse button middle (pressed wheel)
    SIDE    = 3,       // Mouse button side (advanced mouse device)
    EXTRA   = 4,       // Mouse button extra (advanced mouse device)
    FORWARD = 5,       // Mouse button forward (advanced mouse device)
    BACK    = 6,       // Mouse button back (advanced mouse device)
};

```

### GAMEPAD_BUTTON

```c
// [ie. GAMEPAD_BUTTON.RIGHT_FACE_DOWN]
enum GAMEPAD_BUTTON {
    LEFT_FACE_UP, LEFT_FACE_RIGHT, LEFT_FACE_DOWN, LEFT_FACE_LEFT,      // dpad
    RIGHT_FACE_UP, RIGHT_FACE_RIGHT, RIGHT_FACE_DOWN, RIGHT_FACE_LEFT,  // abxy
    LEFT_TRIGGER_1, LEFT_TRIGGER_2, RIGHT_TRIGGER_1, RIGHT_TRIGGER_2,   // top triggers
    MIDDLE_LEFT, MIDDLE, MIDDLE_RIGHT                                   // start, select, middle btn
};
```

### GAMEPAD_AXIS

```c
// [ie. GAMEPAD_AXIS.LEFT_X]
enum GAMEPAD_AXIS {
    LEFT_X,         // Left stick horizontal
    LEFT_Y,         // Left stick vertical
    RIGHT_X,        // Right stick horizontal
    RIGHT_Y,        // Right stick vertical
    LEFT_TRIGGER,   // Pressure sensitive L2
    RIGHT_TRIGGER   // Pressure sensitive R2
};
```

### KEY

```c
// [ie. KEY.SPACE]
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
enum KEY {
    NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    APOSTROPHE      = 39,       // Key: '
    COMMA           = 44,       // Key: ,
    MINUS           = 45,       // Key: -
    PERIOD          = 46,       // Key: .
    SLASH           = 47,       // Key: /
    ZERO            = 48,       // Key: 0
    ONE             = 49,       // Key: 1
    TWO             = 50,       // Key: 2
    THREE           = 51,       // Key: 3
    FOUR            = 52,       // Key: 4
    FIVE            = 53,       // Key: 5
    SIX             = 54,       // Key: 6
    SEVEN           = 55,       // Key: 7
    EIGHT           = 56,       // Key: 8
    NINE            = 57,       // Key: 9
    SEMICOLON       = 59,       // Key: ;
    EQUAL           = 61,       // Key: =
    A               = 65,       // Key: A | a
    B               = 66,       // Key: B | b
    C               = 67,       // Key: C | c
    D               = 68,       // Key: D | d
    E               = 69,       // Key: E | e
    F               = 70,       // Key: F | f
    G               = 71,       // Key: G | g
    H               = 72,       // Key: H | h
    I               = 73,       // Key: I | i
    J               = 74,       // Key: J | j
    K               = 75,       // Key: K | k
    L               = 76,       // Key: L | l
    M               = 77,       // Key: M | m
    N               = 78,       // Key: N | n
    O               = 79,       // Key: O | o
    P               = 80,       // Key: P | p
    Q               = 81,       // Key: Q | q
    R               = 82,       // Key: R | r
    S               = 83,       // Key: S | s
    T               = 84,       // Key: T | t
    U               = 85,       // Key: U | u
    V               = 86,       // Key: V | v
    W               = 87,       // Key: W | w
    X               = 88,       // Key: X | x
    Y               = 89,       // Key: Y | y
    Z               = 90,       // Key: Z | z
    LEFT_BRACKET    = 91,       // Key: [
    BACKSLASH       = 92,       // Key: '\'
    RIGHT_BRACKET   = 93,       // Key: ]
    GRAVE           = 96,       // Key: `
    // Function keys
    SPACE           = 32,       // Key: Space
    ESCAPE          = 256,      // Key: Esc
    ENTER           = 257,      // Key: Enter
    TAB             = 258,      // Key: Tab
    BACKSPACE       = 259,      // Key: Backspace
    INSERT          = 260,      // Key: Ins
    DELETE          = 261,      // Key: Del
    RIGHT           = 262,      // Key: Cursor right
    LEFT            = 263,      // Key: Cursor left
    DOWN            = 264,      // Key: Cursor down
    UP              = 265,      // Key: Cursor up
    PAGE_UP         = 266,      // Key: Page up
    PAGE_DOWN       = 267,      // Key: Page down
    HOME            = 268,      // Key: Home
    END             = 269,      // Key: End
    CAPS_LOCK       = 280,      // Key: Caps lock
    SCROLL_LOCK     = 281,      // Key: Scroll down
    NUM_LOCK        = 282,      // Key: Num lock
    PRINT_SCREEN    = 283,      // Key: Print screen
    PAUSE           = 284,      // Key: Pause
    F1              = 290,      // Key: F1
    F2              = 291,      // Key: F2
    F3              = 292,      // Key: F3
    F4              = 293,      // Key: F4
    F5              = 294,      // Key: F5
    F6              = 295,      // Key: F6
    F7              = 296,      // Key: F7
    F8              = 297,      // Key: F8
    F9              = 298,      // Key: F9
    F10             = 299,      // Key: F10
    F11             = 300,      // Key: F11
    F12             = 301,      // Key: F12
    LEFT_SHIFT      = 340,      // Key: Shift left
    LEFT_CONTROL    = 341,      // Key: Control left
    LEFT_ALT        = 342,      // Key: Alt left
    LEFT_SUPER      = 343,      // Key: Super left
    RIGHT_SHIFT     = 344,      // Key: Shift right
    RIGHT_CONTROL   = 345,      // Key: Control right
    RIGHT_ALT       = 346,      // Key: Alt right
    RIGHT_SUPER     = 347,      // Key: Super right
    KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    KP_0            = 320,      // Key: Keypad 0
    KP_1            = 321,      // Key: Keypad 1
    KP_2            = 322,      // Key: Keypad 2
    KP_3            = 323,      // Key: Keypad 3
    KP_4            = 324,      // Key: Keypad 4
    KP_5            = 325,      // Key: Keypad 5
    KP_6            = 326,      // Key: Keypad 6
    KP_7            = 327,      // Key: Keypad 7
    KP_8            = 328,      // Key: Keypad 8
    KP_9            = 329,      // Key: Keypad 9
    KP_DECIMAL      = 330,      // Key: Keypad .
    KP_DIVIDE       = 331,      // Key: Keypad /
    KP_MULTIPLY     = 332,      // Key: Keypad *
    _KP_SUBTRACT     = 333,      // Key: Keypad -
    KP_ADD          = 334,      // Key: Keypad +
    KP_ENTER        = 335,      // Key: Keypad Enter
    KP_EQUAL        = 336,      // Key: Keypad =
    // Android key buttons
    BACK            = 4,        // Key: Android back button
    _MENU            = 82,       // Key: Android menu button
    VOLUME_UP       = 24,       // Key: Android volume up button
    VOLUME_DOWN     = 25        // Key: Android volume down button
};

```
