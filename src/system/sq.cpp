// sq.h
#include "sq.h"

#include <sqstdaux.h>
#include <sqstdmath.h>
#include <sqstdio.h>
#include <sqstdstring.h>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "global.h"
#include "enet.h"
#include "net_utils.h"
#include <vector>
#include "camera3d.h"
#include "gameobject3d.h"
#include <cstring>
#include <string>
#include <variant>
#include <unordered_map>
#include <bsp.h>

static inline std::string GAMEDATA_PATH = "gamedata/";

// -----------------------------------------------------------------------
// Squirrel Utilities
// -----------------------------------------------------------------------

/*
SQCharToString
Converts a Squirrel string to std::string, handles unicode build if needed.
*/
static std::string SQCharToString(const SQChar *name)
{
  std::string name_str = "";
#ifdef _UNICODE
  std::wstring wname(name);
  name_str = std::string(wname.begin(), wname.end());
#else
  name_str = name;
#endif
  return name_str;
};

/*
sq_getcolor
Extracts a Raylib Color from a Squirrel table at the given stack index
*/
static Color SQGetColor(HSQUIRRELVM v, SQInteger index)
{
  Color c = {0, 0, 0, 255};

  auto get_field = [&](const char *field) -> unsigned char
  {
    sq_pushstring(v, _SC(field), -1);
    if (SQ_SUCCEEDED(sq_get(v, index)))
    {
      SQInteger val;
      sq_getinteger(v, -1, &val);
      sq_pop(v, 1);
      return (unsigned char)val;
    }
    sq_pop(v, 1);
    return 0;
  };

  c.r = get_field("r");
  c.g = get_field("g");
  c.b = get_field("b");
  // Default alpha to 255 if not found to avoid invisible objects
  sq_pushstring(v, _SC("a"), -1);
  if (SQ_SUCCEEDED(sq_get(v, index)))
  {
    SQInteger val;
    sq_getinteger(v, -1, &val);
    c.a = (unsigned char)val;
    sq_pop(v, 1);
  }
  else
  {
    sq_pop(v, 1);
    c.a = 255;
  }

  return c;
};

// Helper to wrap a C++ pointer into a Squirrel Userdata with the method delegate
static void PushGameObjectAsUserdata(HSQUIRRELVM vm, GameObject3D *obj)
{
  GameObject3D **userdata = (GameObject3D **)sq_newuserdata(vm, sizeof(GameObject3D *));
  *userdata = obj;

  sq_newtable(vm); // The delegate table for methods

  auto bindfunc = [&](const char *name, SQFUNCTION func)
  {
    sq_pushstring(vm, name, -1);
    sq_newclosure(vm, func, 0);
    sq_newslot(vm, -3, SQFalse);
  };

  // functions callable from an instance
  bindfunc("_get", sq_instance_get); // allows us to attach functions to the returned object
  bindfunc("get", sq_instance_get_var);
  bindfunc("set", sq_instance_set_var);
  bindfunc("destroy", sq_instance_destroy);
  bindfunc("get_target_name", sq_instance_get_target_name);
  bindfunc("set_target_name", sq_instance_set_target_name);
  bindfunc("get_target", sq_instance_get_target);
  bindfunc("set_target", sq_instance_set_target);
  bindfunc("set_position", sq_instance_set_position);
  bindfunc("get_position", sq_instance_get_position);
  bindfunc("set_velocity", sq_instance_set_velocity);
  bindfunc("get_velocity", sq_instance_get_velocity);
  bindfunc("set_acceleration", sq_instance_set_acceleration);
  bindfunc("get_acceleration", sq_instance_get_acceleration);
  bindfunc("set_speed", sq_instance_set_speed);
  bindfunc("get_speed", sq_instance_get_speed);
  bindfunc("set_size", sq_instance_set_size);
  bindfunc("get_size", sq_instance_get_size);
  bindfunc("set_collision_box", sq_instance_set_collision_box);
  bindfunc("get_collision_box", sq_instance_get_collision_box);

  sq_setdelegate(vm, -2); // Set table as delegate for userdata
}

// -----------------------------------------------------------------------
// Squirrel-bound C++ functions
// These are registered and callable from .nut scripts
// -----------------------------------------------------------------------

// Check if a gamepad is available (index 0-3 usually)
SQInteger sq_is_gamepad_available(HSQUIRRELVM vm)
{
  SQInteger index;
  sq_getinteger(vm, 2, &index);
  sq_pushbool(vm, IsGamepadAvailable((int)index));
  return 1;
}

// Get the name of the gamepad (e.g., "Xbox Controller")
SQInteger sq_get_gamepad_name(HSQUIRRELVM vm)
{
  SQInteger index;
  sq_getinteger(vm, 2, &index);
  sq_pushstring(vm, GetGamepadName((int)index), -1);
  return 1;
}

// Button States
SQInteger sq_is_gamepad_button_pressed(HSQUIRRELVM vm)
{
  SQInteger index, button;
  sq_getinteger(vm, 2, &index);
  sq_getinteger(vm, 3, &button);
  sq_pushbool(vm, IsGamepadButtonPressed((int)index, (int)button));
  return 1;
}

SQInteger sq_is_gamepad_button_down(HSQUIRRELVM vm)
{
  SQInteger index, button;
  sq_getinteger(vm, 2, &index);
  sq_getinteger(vm, 3, &button);
  sq_pushbool(vm, IsGamepadButtonDown((int)index, (int)button));
  return 1;
}

SQInteger sq_is_gamepad_button_released(HSQUIRRELVM vm)
{
  SQInteger index, button;
  sq_getinteger(vm, 2, &index);
  sq_getinteger(vm, 3, &button);
  sq_pushbool(vm, IsGamepadButtonReleased((int)index, (int)button));
  return 1;
}

SQInteger sq_is_gamepad_button_up(HSQUIRRELVM vm)
{
  SQInteger index, button;
  sq_getinteger(vm, 2, &index);
  sq_getinteger(vm, 3, &button);
  sq_pushbool(vm, IsGamepadButtonUp((int)index, (int)button));
  return 1;
}

// Get the last button pressed
SQInteger sq_get_gamepad_button_pressed(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetGamepadButtonPressed());
  return 1;
}

// Axis handling (Joysticks/Triggers)
SQInteger sq_get_gamepad_axis_count(HSQUIRRELVM vm)
{
  SQInteger index;
  sq_getinteger(vm, 2, &index);
  sq_pushinteger(vm, GetGamepadAxisCount((int)index));
  return 1;
}

SQInteger sq_get_gamepad_axis_movement(HSQUIRRELVM vm)
{
  SQInteger index, axis;
  sq_getinteger(vm, 2, &index);
  sq_getinteger(vm, 3, &axis);
  sq_pushfloat(vm, GetGamepadAxisMovement((int)index, (int)axis));
  return 1;
}

// Haptics/Vibration
SQInteger sq_set_gamepad_vibration(HSQUIRRELVM vm)
{
  SQInteger index;
  SQFloat left, right, duration;
  sq_getinteger(vm, 2, &index);
  sq_getfloat(vm, 3, &left);
  sq_getfloat(vm, 4, &right);
  // SetGamepadVibration((int)index, (float)left, (float)right, (float)duration);
  return 0;
}

/*
sq_get_camera_position
Returns the camera position as a {x, y, z} table.
*/
SQInteger sq_get_camera_position(HSQUIRRELVM vm)
{
  if (!camera)
    return 0;

  sq_newtable(vm);

  sq_pushstring(vm, _SC("x"), -1);
  sq_pushfloat(vm, camera->position.x);
  sq_newslot(vm, -3, SQFalse);

  sq_pushstring(vm, _SC("y"), -1);
  sq_pushfloat(vm, camera->position.y);
  sq_newslot(vm, -3, SQFalse);

  sq_pushstring(vm, _SC("z"), -1);
  sq_pushfloat(vm, camera->position.z);
  sq_newslot(vm, -3, SQFalse);

  return 1;
};

/*
sq_send_packet_number
Squirrel: send_packet_number(target_id, "name", value)
  Client  → sends float packet to Server CPP over ENet
  Server  → pushes float packet to SQ Client queue
*/
/*
sq_send_packet_number
Squirrel: send_packet_number(target_id, "name", value)
  Client  → sends float packet to Server CPP over ENet
  Server  → sends float packet to Client:
            - If target_id == local host player_id → push to SQ queue (local)
            - If target_id == different client → send over ENet to that client
*/
SQInteger sq_send_packet_number(HSQUIRRELVM vm)
{
  SQInteger target_id;
  const SQChar *name;
  SQFloat val;

  sq_getinteger(vm, 2, &target_id);
  sq_getstring(vm, 3, &name);
  sq_getfloat(vm, 4, &val);

  std::string name_str = SQCharToString(name);

  if (vm == client_vm)
  {
    // Client → Server CPP (over ENet)
    Net_ToCPPServer(name_str, (float)val);
  }
  else
  {
    // Server is sending to a client
    // Check if target is local host (same process) or remote client

    if (target_id == my_local_player_id)
    {
      // Target is the host (local) - use SQ queue
      Net_ToSQClient((int)target_id, name_str, (float)val);
    }
    else if (target_id >= 0 && target_id < MAX_PLAYERS && users[target_id].peer)
    {
      // Target is a remote client - send over ENet
      Net_ToCPPClient(users[target_id].peer, name_str, (float)val);
    }
    else if (target_id == -1)
    {
      // Broadcast to all clients
      // Send to local SQ client if host exists
      if (my_local_player_id >= 0)
      {
        Net_ToSQClient(my_local_player_id, name_str, (float)val);
      }
      // Send to all remote clients
      for (int i = 0; i < users_count; i++)
      {
        if (users[i].peer && users[i].player_id != my_local_player_id)
        {
          Net_ToCPPClient(users[i].peer, name_str, (float)val);
        }
      }
    }
  }

  return 0;
};

/*
sq_send_packet_vector3
Squirrel: send_packet_vector3(target_id, "name", {x, y, z})
  Client  → sends Vector3 packet to Server CPP over ENet
  Server  → pushes Vector3 packet to SQ Client queue
*/
SQInteger sq_send_packet_vector3(HSQUIRRELVM vm)
{
  SQInteger target_id;
  const SQChar *name;

  sq_getinteger(vm, 2, &target_id);
  sq_getstring(vm, 3, &name);
  std::string name_str = SQCharToString(name);

  float x = 0, y = 0, z = 0;
  if (sq_gettype(vm, 4) == OT_TABLE)
  {
    auto get = [&](const SQChar *k, float &out)
    {
      sq_pushstring(vm, k, -1);
      if (SQ_SUCCEEDED(sq_get(vm, 4)))
      {
        sq_getfloat(vm, -1, &out);
        sq_pop(vm, 1);
      }
    };
    get(_SC("x"), x);
    get(_SC("y"), y);
    get(_SC("z"), z);
  }

  Vector3 vec = {x, y, z};

  if (vm == client_vm)
  {
    // Client → Server CPP (over ENet)
    Net_ToCPPServer(name_str, vec);
  }
  else
  {
    // Server is sending to a client
    if (target_id == my_local_player_id)
    {
      // Target is the host (local) - use SQ queue
      Net_ToSQClient((int)target_id, name_str, vec);
    }
    else if (target_id >= 0 && target_id < MAX_PLAYERS && users[target_id].peer)
    {
      // Target is a remote client - send over ENet
      Net_ToCPPClient(users[target_id].peer, name_str, vec);
    }
    else if (target_id == -1)
    {
      // Broadcast to all clients
      if (my_local_player_id >= 0)
      {
        Net_ToSQClient(my_local_player_id, name_str, vec);
      }
      for (int i = 0; i < users_count; i++)
      {
        if (users[i].peer && users[i].player_id != my_local_player_id)
        {
          Net_ToCPPClient(users[i].peer, name_str, vec);
        }
      }
    }
  }

  return 0;
};

/*
sq_get_packet
Squirrel: get_packet()
Pops the next packet from the appropriate queue and returns it as a table:
  { client_id, name, value }
Returns null if queue is empty.
*/
SQInteger sq_get_packet(HSQUIRRELVM vm)
{
  bool is_server = (vm == server_vm);
  auto &queue = is_server ? client_to_server_packets : server_to_client_packets;
  int target_id = my_local_player_id;

  if (queue.empty())
  {
    sq_pushnull(vm);
    return 1;
  }

  for (auto it = queue.begin(); it != queue.end(); ++it)
  {
    if (is_server || it->client_id == target_id || it->client_id == -1)
    {
      Packet p = *it;
      queue.erase(it);

      sq_newtable(vm);

      // client_id
      sq_pushstring(vm, _SC("client_id"), -1);
      sq_pushinteger(vm, p.client_id);
      sq_newslot(vm, -3, SQFalse);

      // name
      sq_pushstring(vm, _SC("name"), -1);
#ifdef _UNICODE
      std::wstring wname(p.name.begin(), p.name.end());
      sq_pushstring(vm, wname.c_str(), -1);
#else
      sq_pushstring(vm, p.name.c_str(), -1);
#endif
      sq_newslot(vm, -3, SQFalse);

      // value — push the correct Squirrel type based on what's in the variant
      sq_pushstring(vm, _SC("value"), -1);
      std::visit([&](auto &&arg)
                 {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            sq_pushinteger(vm, arg);
        else if constexpr (std::is_same_v<T, float>)
            sq_pushfloat(vm, arg);
        else if constexpr (std::is_same_v<T, std::string>)
            sq_pushstring(vm, arg.c_str(), -1);
        else if constexpr (std::is_same_v<T, bool>)
            sq_pushbool(vm, arg ? SQTrue : SQFalse);
        else if constexpr (std::is_same_v<T, Vector3>)
        {
            sq_newtable(vm);
            sq_pushstring(vm, _SC("x"), -1); sq_pushfloat(vm, arg.x); sq_newslot(vm, -3, SQFalse);
            sq_pushstring(vm, _SC("y"), -1); sq_pushfloat(vm, arg.y); sq_newslot(vm, -3, SQFalse);
            sq_pushstring(vm, _SC("z"), -1); sq_pushfloat(vm, arg.z); sq_newslot(vm, -3, SQFalse);
        } }, p.value);
      sq_newslot(vm, -3, SQFalse);

      return 1;
    }
  }

  sq_pushnull(vm);
  return 1;
};

/*
sq_is_mouse_button_pressed
Squirrel: is_mouse_button_pressed(MOUSE_BUTTON.LEFT)
Returns true the frame a mb is first pressed.
*/
SQInteger sq_is_mouse_button_pressed(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsMouseButtonPressed(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_mouse_button_down
Squirrel: is_mouse_button_down(MOUSE_BUTTON.LEFT)
Returns true the frames a mb is pressed.
*/
SQInteger sq_is_mouse_button_down(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsMouseButtonDown(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_mouse_button_released
Squirrel: is_mouse_button_released(MOUSE_BUTTON.LEFT)
Returns true the frames a mb is released.
*/
SQInteger sq_is_mouse_button_released(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsMouseButtonReleased(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_mouse_button_up
*/
SQInteger sq_is_mouse_button_up(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsMouseButtonUp(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_get_mouse_x
*/
SQInteger sq_get_mouse_x(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetMouseX());
  return 1;
};

/*
sq_get_mouse_y
*/
SQInteger sq_get_mouse_y(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetMouseY());
  return 1;
};

/*
sq_get_mouse_delta
*/
SQInteger sq_get_mouse_delta(HSQUIRRELVM vm)
{
  Vector2 delta = GetMouseDelta();
  sq_newtable(vm);

  sq_pushstring(vm, "x", -1);  // Key
  sq_pushfloat(vm, delta.x);   // Value
  sq_newslot(vm, -3, SQFalse); // Add to table at index -3

  sq_pushstring(vm, "y", -1);  // Key
  sq_pushfloat(vm, delta.y);   // Value
  sq_newslot(vm, -3, SQFalse); // Add to table at index -3

  return 1;
};

/*
sq_get_mouse_wheel_move
*/
SQInteger sq_get_mouse_wheel_move(HSQUIRRELVM vm)
{
  sq_pushfloat(vm, GetMouseWheelMove());
  return 1;
};

/*
sq_set_mouse_cursor
*/
SQInteger sq_set_mouse_cursor(HSQUIRRELVM vm)
{
  SQInteger cursor_id;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &cursor_id)))
  {
    SetMouseCursor(static_cast<int>(cursor_id));
    return 0;
  }
  return sq_throwerror(vm, "Invalid cursor ID");
};

/*
sq_is_key_pressed
Squirrel: is_key_pressed(KEY.X)
Returns true the frame a key is first pressed.
*/
SQInteger sq_is_key_pressed(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsKeyPressed(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_key_down
Squirrel: is_key_down(KEY.X)
Returns true every frame a key is held.
*/
SQInteger sq_is_key_down(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsKeyDown(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_key_released
Squirrel: is_key_released(KEY.X)
Returns true every frame a key is held.
*/
SQInteger sq_is_key_released(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsKeyReleased(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_is_key_up
*/
SQInteger sq_is_key_up(HSQUIRRELVM vm)
{
  SQInteger key;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &key)))
  {
    sq_pushbool(vm, IsKeyUp(static_cast<int>(key)) ? SQTrue : SQFalse);
    return 1;
  }
  return sq_throwerror(vm, "Invalid key code");
};

/*
sq_toggle_fullscreen
*/
SQInteger sq_toggle_fullscreen(HSQUIRRELVM vm)
{
  ToggleFullscreen();
  return 0;
};

/*
sq_toggle_borderless_window
*/
SQInteger sq_toggle_borderless_windowed(HSQUIRRELVM vm)
{
  ToggleBorderlessWindowed();
  return 0;
};

/*
sq_maximize_window
*/
SQInteger sq_maximize_window(HSQUIRRELVM vm)
{
  MaximizeWindow();
  return 0;
};

/*
sq_minimize_window
*/
SQInteger sq_minimize_window(HSQUIRRELVM vm)
{
  MinimizeWindow();
  return 0;
};

/*
sq_restore_window
*/
SQInteger sq_restore_window(HSQUIRRELVM vm)
{
  RestoreWindow();
  return 0;
};

/*
sq_set_window_icon
*/
SQInteger sq_set_window_icon(HSQUIRRELVM vm)
{
  const SQChar *path;
  if (SQ_SUCCEEDED(sq_getstring(vm, 2, &path)))
  {
    Image icon = LoadImage(path);
    if (icon.data != nullptr)
    {
      SetWindowIcon(icon);
      UnloadImage(icon);
      return 0;
    }
    return sq_throwerror(vm, "Failed to load icon image");
  }
  return sq_throwerror(vm, "Invalid path: expected string");
};

/*
sq_set_window_title
*/
SQInteger sq_set_window_title(HSQUIRRELVM vm)
{
  const SQChar *title;
  if (SQ_SUCCEEDED(sq_getstring(vm, 2, &title)))
  {
    SetWindowTitle(title);
    return 0;
  }
  return sq_throwerror(vm, "Invalid: expected string");
};

/*
sq_set_window_position
Expects two integers: x and y
Usage in Squirrel: set_window_position(100, 100);
*/
SQInteger sq_set_window_position(HSQUIRRELVM vm)
{
  SQInteger x, y;

  // Get X from slot 2 and Y from slot 3
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &x)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &y)))
  {
    SetWindowPosition(static_cast<int>(x), static_cast<int>(y));
    return 0;
  }

  return sq_throwerror(vm, "Invalid arguments: expected (int, int)");
};

/*
sq_get_screen_width
Returns the current width of the screen
*/
SQInteger sq_get_screen_width(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetScreenWidth());
  return 1;
};

/*
sq_get_screen_height
Returns the current height of the screen
*/
SQInteger sq_get_screen_height(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetScreenHeight());
  return 1;
};

/*
sq_get_render_width
Returns the current render width (can be different if using render textures or high-DPI)
*/
SQInteger sq_get_render_width(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetRenderWidth());
  return 1;
};

/*
sq_get_render_height
Returns the current render height
*/
SQInteger sq_get_render_height(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetRenderHeight());
  return 1;
};

/*
sq_show_cursor
Shows the mouse cursor
*/
SQInteger sq_show_cursor(HSQUIRRELVM vm)
{
  ShowCursor();
  return 0;
};

/*
sq_hide_cursor
Hides the mouse cursor
*/
SQInteger sq_hide_cursor(HSQUIRRELVM vm)
{
  HideCursor();
  return 0;
};

/*
sq_is_cursor_hidden
Returns true if the cursor is currently hidden
*/
SQInteger sq_is_cursor_hidden(HSQUIRRELVM vm)
{
  sq_pushbool(vm, IsCursorHidden());
  return 1;
};

/*
sq_enable_cursor
Enables the cursor (unlocks it and makes it visible)
*/
SQInteger sq_enable_cursor(HSQUIRRELVM vm)
{
  EnableCursor();
  return 0;
};

/*
sq_disable_cursor
Disables the cursor (locks it to the window and hides it)
*/
SQInteger sq_disable_cursor(HSQUIRRELVM vm)
{
  DisableCursor();
  return 0;
};

/*
sq_is_cursor_on_screen
Returns true if the cursor is within the window's client area
*/
SQInteger sq_is_cursor_on_screen(HSQUIRRELVM vm)
{
  sq_pushbool(vm, IsCursorOnScreen());
  return 1;
};

/*
sq_draw_fps
Usage: draw_fps(int x, int y);
*/
SQInteger sq_draw_fps(HSQUIRRELVM vm)
{
  SQInteger x, y;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &x)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &y)))
  {
    global_draw_fps = true;
    global_fps_x = x;
    global_fps_x = y;
  }
  return 0;
};

/*
sq_clear_background
Usage: ClearBackground(COLOR.BLACK);
*/
SQInteger sq_clear_background(HSQUIRRELVM vm)
{
  Color color = SQGetColor(vm, 2);
  ClearBackground(color);
  return 0;
};

/*
sq_begin_scissor_mode
Usage: BeginScissorMode(x, y, width, height);
*/
SQInteger sq_begin_scissor_mode(HSQUIRRELVM vm)
{
  SQInteger x, y, w, h;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &x)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &y)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 4, &w)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 5, &h)))
  {
    BeginScissorMode((int)x, (int)y, (int)w, (int)h);
    return 0;
  }
  return sq_throwerror(vm, "Invalid arguments for BeginScissorMode");
};

/*
sq_end_scissor_mode
Usage: EndScissorMode();
*/
SQInteger sq_end_scissor_mode(HSQUIRRELVM vm)
{
  EndScissorMode();
  return 0;
};

// ----- TIMING -----

/*
sq_set_target_fps
Usage: SetTargetFPS(60);
*/
SQInteger sq_set_target_fps(HSQUIRRELVM vm)
{
  SQInteger fps;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &fps)))
  {
    SetTargetFPS((int)fps);
    return 0;
  }
  return sq_throwerror(vm, "Invalid FPS value");
};

/*
sq_get_frame_time
Returns the time in seconds for the last frame (delta time)
*/
SQInteger sq_get_frame_time(HSQUIRRELVM vm)
{
  sq_pushfloat(vm, GetFrameTime());
  return 1;
};

/*
sq_get_time
Returns elapsed time in seconds since InitWindow()
*/
SQInteger sq_get_time(HSQUIRRELVM vm)
{
  sq_pushfloat(vm, (float)GetTime());
  return 1;
};

/*
sq_get_fps
Returns current FPS
*/
SQInteger sq_get_fps(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, GetFPS());
  return 1;
};

// ----- RNG -----

/*
sq_set_random_seed
Usage: SetRandomSeed(12345);
*/
SQInteger sq_set_random_seed(HSQUIRRELVM vm)
{
  SQInteger seed;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &seed)))
  {
    SetRandomSeed((unsigned int)seed);
    return 0;
  }
  return sq_throwerror(vm, "Invalid seed value");
};

/*
sq_get_random_value
Usage: GetRandomValue(min, max);
*/
SQInteger sq_get_random_value(HSQUIRRELVM vm)
{
  SQInteger min, max;
  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &min)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &max)))
  {
    sq_pushinteger(vm, GetRandomValue((int)min, (int)max));
    return 1;
  }
  return sq_throwerror(vm, "Invalid range for GetRandomValue");
};

/*
sq_draw_rectangle
Usage in Squirrel: DrawRect(x, y, width, height, COLOR.RED);
*/
SQInteger sq_draw_rectangle(HSQUIRRELVM vm)
{
  SQInteger x, y, w, h; // [rgba]

  if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &x)) && // r
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &y)) && // g
      SQ_SUCCEEDED(sq_getinteger(vm, 4, &w)) && // b
      SQ_SUCCEEDED(sq_getinteger(vm, 5, &h)))   // a
  {
    Color color = SQGetColor(vm, 6);
    DrawRectangle(static_cast<int>(x),
                  static_cast<int>(y),
                  static_cast<int>(w),
                  static_cast<int>(h),
                  color);

    return 0;
  }

  return sq_throwerror(vm, "Invalid arguments for DrawRect: expected (int, int, int, int, table)");
};

/*
sq_draw_cube
Usage in Squirrel: DrawCube(0.0, 5.0, 0.0, 2.0, 2.0, 2.0, COLOR.WHITE);
*/
SQInteger sq_draw_cube(HSQUIRRELVM vm)
{
  // Slot 2: x (Float)
  // Slot 3: y (Float)
  // Slot 4: z (Float)
  // Slot 5: width (Float)
  // Slot 6: height (Float)
  // Slot 7: length (Float)
  // Slot 8: color (Table)

  SQFloat x, y, z, w, h, l;

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &x)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 3, &y)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 4, &z)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 5, &w)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 6, &h)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 7, &l)) &&
      sq_gettype(vm, 8) == OT_TABLE)
  {
    Vector3 pos = {(float)x, (float)y, (float)z};
    Color color = SQGetColor(vm, 8);

    DrawCube(pos, (float)w, (float)h, (float)l, color);

    return 0;
  }

  return sq_throwerror(vm, "Invalid arguments for DrawCube: expected (6 floats, table)");
};

/*
sq_get_player_instance
Usage: local player = server.get_player_instance(id);
*/
SQInteger sq_get_player_instance(HSQUIRRELVM vm)
{
  SQInteger player_id;
  if (SQ_FAILED(sq_getinteger(vm, 2, &player_id)))
    return sq_throwerror(vm, "Argument 1 must be an integer (player_id)");

  if (player_id < 0 || player_id >= MAX_PLAYERS)
  {
    sq_pushnull(vm);
    return 1;
  }

  GameObject3D *obj = users[player_id].object_ref;
  if (!obj)
  {
    sq_pushnull(vm);
    return 1;
  }

  PushGameObjectAsUserdata(vm, obj);

  return 1;
};

/*
sq_get_player_count
Squirrel: server.get_player_count()
Returns the current number of connected players.
*/
SQInteger sq_get_player_count(HSQUIRRELVM vm)
{
  int count = 0;
  for (auto &obj : gameobjects)
  {
    if (obj->client_id >= 0)
      count++;
  }
  sq_pushinteger(vm, count);
  return 1;
};

/*
sq_sendflags_sync(player_id, flags)
*/
SQInteger sq_sendflags_sync(HSQUIRRELVM vm)
{
  SQInteger player_id;
  SQInteger flags_val;

  if (SQ_FAILED(sq_getinteger(vm, 2, &player_id)))
    return sq_throwerror(vm, "Argument 1 (player_id) must be an integer");

  if (SQ_FAILED(sq_getinteger(vm, 3, &flags_val)))
    return sq_throwerror(vm, "Argument 2 (flags) must be an integer");

  if (player_id < 0 || player_id >= MAX_PLAYERS || users[player_id].object_ref == nullptr)
    return 0;

  GameObject3D *obj = users[player_id].object_ref;
  obj->sendflags = static_cast<int>(flags_val);

  printf("Server Set Flags for Player %lld: %d\n", (long long)player_id, obj->sendflags);

  return 0;
};

/*
sq_instance_create(spawn_pos)
*/
SQInteger sq_instance_create(HSQUIRRELVM vm)
{
  SQInteger x, y, z;

  if (SQ_FAILED(sq_getinteger(vm, 2, &x)))
    return sq_throwerror(vm, "Argument 1 (x) must be an integer");
  if (SQ_FAILED(sq_getinteger(vm, 3, &y)))
    return sq_throwerror(vm, "Argument 2 (y) must be an integer");
  if (SQ_FAILED(sq_getinteger(vm, 4, &z)))
    return sq_throwerror(vm, "Argument 3 (z) must be an integer");

  auto obj = InstanceCreate<GameObject3D>(Vector3{
      (float)x,
      (float)y,
      (float)z});
  PushGameObjectAsUserdata(vm, obj);

  return 1;
};

SQInteger sq_get_instance_count(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, (SQInteger)gameobjects.size());
  return 1;
};

/* Instances Get All */
SQInteger sq_instances_get_all(HSQUIRRELVM vm)
{
  sq_newarray(vm, 0);

  for (const auto &obj : gameobjects)
  {
    PushGameObjectAsUserdata(vm, obj.get());
    sq_arrayappend(vm, -2);
  }
  return 1;
};

/* Instances Get (by targetname) */
SQInteger sq_instances_get(HSQUIRRELVM vm)
{
  const SQChar *target_name;
  if (SQ_FAILED(sq_getstring(vm, 2, &target_name)))
    return sq_throwerror(vm, "Argument 1 must be a string");

  sq_newarray(vm, 0);

  for (const auto &obj : gameobjects)
  {
    if (std::strcmp(obj->target_name.c_str(), (const char *)target_name) == 0)
    {
      {
        PushGameObjectAsUserdata(vm, obj.get());
        sq_arrayappend(vm, -2);
      }
    }
  }
  return 1;
};

/* Instance Destroy */
SQInteger sq_instance_destroy(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)))
    return sq_throwerror(vm, "Invalid object instance");

  if (ud && *ud)
  {
    (*ud)->Destroy();
    *ud = nullptr;
  }

  return 0;
};

/* Instance Get */
SQInteger sq_instance_get(HSQUIRRELVM vm)
{
  sq_pushroottable(vm);
  sq_push(vm, 2);

  if (SQ_SUCCEEDED(sq_get(vm, -2)))
  {
    return 1;
  }

  sq_pop(vm, 1);
  return sq_throwerror(vm, "Member or Function not found");
};

/* Get - Set Var */
using ScriptValue = std::variant<std::string, float, bool, int>;
SQInteger sq_instance_get_var(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr);

  const SQChar *key;
  sq_getstring(vm, 2, &key);

  auto it = (*ud)->script_vars.find(key);
  if (it == (*ud)->script_vars.end())
  {
    sq_pushnull(vm);
    return 1;
  }

  std::visit([vm](auto &&arg)
             {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>)
            sq_pushstring(vm, arg.c_str(), -1);
        else if constexpr (std::is_same_v<T, float>)
            sq_pushfloat(vm, arg);
        else if constexpr (std::is_same_v<T, int>)
            sq_pushinteger(vm, arg);
        else if constexpr (std::is_same_v<T, bool>)
            sq_pushbool(vm, arg ? SQTrue : SQFalse); }, it->second);

  return 1;
};

SQInteger sq_instance_set_var(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr);

  const SQChar *key;
  sq_getstring(vm, 2, &key);

  SQObjectType type = sq_gettype(vm, 3);

  if (type == OT_STRING)
  {
    const SQChar *val;
    sq_getstring(vm, 3, &val);
    (*ud)->script_vars[key] = std::string(val);
  }
  else if (type == OT_FLOAT)
  {
    SQFloat val;
    sq_getfloat(vm, 3, &val);
    (*ud)->script_vars[key] = (float)val;
  }
  else if (type == OT_INTEGER)
  {
    SQInteger val;
    sq_getinteger(vm, 3, &val);
    (*ud)->script_vars[key] = (int)val;
  }
  else if (type == OT_BOOL)
  {
    SQBool val;
    sq_getbool(vm, 3, &val);
    (*ud)->script_vars[key] = (val == SQTrue);
  }

  return 0;
};

/* Position */
SQInteger sq_instance_set_position(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)))
    return sq_throwerror(vm, "Invalid object instance");

  SQFloat x, y, z;
  sq_getfloat(vm, 2, &x);
  sq_getfloat(vm, 3, &y);
  sq_getfloat(vm, 4, &z);

  if (ud && *ud)
  {
    (*ud)->position = Vector3{(float)x, (float)y, (float)z};
  }
  return 0;
};

SQInteger sq_instance_get_position(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)))
    return sq_throwerror(vm, "Invalid object instance");

  if (ud && *ud)
  {
    GameObject3D *obj = *ud;

    sq_newtable(vm);

    // Set X
    sq_pushstring(vm, "x", -1);
    sq_pushfloat(vm, obj->position.x);
    sq_newslot(vm, -3, SQFalse);

    // Set Y
    sq_pushstring(vm, "y", -1);
    sq_pushfloat(vm, obj->position.y);
    sq_newslot(vm, -3, SQFalse);

    // Set Z
    sq_pushstring(vm, "z", -1);
    sq_pushfloat(vm, obj->position.z);
    sq_newslot(vm, -3, SQFalse);

    return 1;
  }

  return 0;
};

/* Target Name*/
SQInteger sq_instance_get_target_name(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_pushstring(vm, (*ud)->target_name.c_str(), -1);
  return 1;
};

SQInteger sq_instance_set_target_name(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  const SQChar *name = nullptr;

  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_FAILED(sq_getstring(vm, 2, &name)))
    return sq_throwerror(vm, "Argument 1 must be a string");

  (*ud)->target_name = name;
  return 0;
};

/* Target */
SQInteger sq_instance_get_target(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_pushstring(vm, (*ud)->target.c_str(), -1);
  return 1;
};

SQInteger sq_instance_set_target(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  const SQChar *target_str = nullptr;

  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_FAILED(sq_getstring(vm, 2, &target_str)))
    return sq_throwerror(vm, "Argument 1 must be a string");

  (*ud)->target = target_str;
  return 0;
};

/* Velocity Get/Set */
SQInteger sq_instance_get_velocity(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_newtable(vm);
  sq_pushstring(vm, "x", -1);
  sq_pushfloat(vm, (*ud)->velocity.x);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "y", -1);
  sq_pushfloat(vm, (*ud)->velocity.y);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "z", -1);
  sq_pushfloat(vm, (*ud)->velocity.z);
  sq_newslot(vm, -3, SQFalse);
  return 1;
};

SQInteger sq_instance_set_velocity(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  SQFloat x, y, z;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &x)) && SQ_SUCCEEDED(sq_getfloat(vm, 3, &y)) && SQ_SUCCEEDED(sq_getfloat(vm, 4, &z)))
  {
    (*ud)->velocity = {(float)x, (float)y, (float)z};
    return 0;
  }
  return sq_throwerror(vm, "Invalid arguments: expected (float, float, float)");
};

/* Size Get/Set */
SQInteger sq_instance_get_size(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_newtable(vm);
  sq_pushstring(vm, "x", -1);
  sq_pushfloat(vm, (*ud)->size.x);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "y", -1);
  sq_pushfloat(vm, (*ud)->size.y);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "z", -1);
  sq_pushfloat(vm, (*ud)->size.z);
  sq_newslot(vm, -3, SQFalse);
  return 1;
};

SQInteger sq_instance_set_size(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  SQFloat x, y, z;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &x)) && SQ_SUCCEEDED(sq_getfloat(vm, 3, &y)) && SQ_SUCCEEDED(sq_getfloat(vm, 4, &z)))
  {
    (*ud)->size = {(float)x, (float)y, (float)z};
    return 0;
  }
  return sq_throwerror(vm, "Invalid arguments");
};

/* Acceleration */
SQInteger sq_instance_get_acceleration(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_pushfloat(vm, (*ud)->acceleration);
  return 1;
};

SQInteger sq_instance_set_acceleration(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  SQFloat acc;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &acc)))
  {
    (*ud)->acceleration = (float)acc;
    return 0;
  }
  return sq_throwerror(vm, "Argument 1 must be a float");
};

/* Speed Get/Set */
SQInteger sq_instance_get_speed(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_pushfloat(vm, (*ud)->speed);
  return 1;
};

SQInteger sq_instance_set_speed(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  SQFloat s;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &s)))
  {
    (*ud)->speed = (float)s;
    return 0;
  }
  return sq_throwerror(vm, "Argument 1 must be a float");
};

/* Collision Box Get/Set */
SQInteger sq_instance_get_collision_box(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  sq_newtable(vm);
  sq_pushstring(vm, _SC("width"), -1);
  sq_pushfloat(vm, (*ud)->collision_box.x);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, _SC("height"), -1);
  sq_pushfloat(vm, (*ud)->collision_box.y);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, _SC("length"), -1);
  sq_pushfloat(vm, (*ud)->collision_box.z);
  sq_newslot(vm, -3, SQFalse);

  return 1;
};

SQInteger sq_instance_set_collision_box(HSQUIRRELVM vm)
{
  GameObject3D **ud = nullptr;
  SQFloat x, y, z;
  if (SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer *)&ud, nullptr)) || !ud || !*ud)
    return sq_throwerror(vm, "Invalid object instance");

  if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &x)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 3, &y)) &&
      SQ_SUCCEEDED(sq_getfloat(vm, 4, &z)))
  {
    (*ud)->collision_box = {(float)x, (float)y, (float)z};
    return 0;
  }
  return sq_throwerror(vm, "Invalid arguments: expected (float, float, float)");
};

// rtext
/*
sq_load_font
Usage: LoadFont("assets/fonts/pixel.ttf", 32);
*/
SQInteger sq_load_font(HSQUIRRELVM vm)
{
  const SQChar *filename;
  SQInteger font_size;

  if (SQ_SUCCEEDED(sq_getstring(vm, 2, &filename)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &font_size)))
  {
    if (font_primary.texture.id > 0)
      UnloadFont(font_primary);

    font_primary = LoadFontEx(filename, (int)font_size, NULL, 0);
    return 0;
  }

  return sq_throwerror(vm, "Invalid arguments for LoadFont: expected (string, int)");
};

/*
sq_draw_text
Usage: DrawText("Hello World", 10, 10, 24, COLOR.WHITE);
*/
SQInteger sq_draw_text(HSQUIRRELVM vm)
{
  const SQChar *text;
  SQInteger x, y, font_size;

  if (SQ_SUCCEEDED(sq_getstring(vm, 2, &text)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 3, &x)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 4, &y)) &&
      SQ_SUCCEEDED(sq_getinteger(vm, 5, &font_size)) &&
      sq_gettype(vm, 6) == OT_TABLE)
  {
    Color color = SQGetColor(vm, 6);

    if (font_primary.texture.id == 0)
    {
      DrawText(text, (int)x, (int)y, (int)font_size, color);
    }
    else
    {
      Vector2 position = {(float)x, (float)y};
      DrawTextEx(font_primary, text, position, (float)font_size, 2.0f, color);
    }

    return 0;
  }

  return sq_throwerror(vm, "Invalid arguments for DrawText: expected (string, int, int, int, table)");
};

/*
sq_start_server
Squirrel: server.start_server()
Starts the ENet server.
*/
SQInteger sq_start_server(HSQUIRRELVM vm)
{
  enetserver_start();
  return 0;
};

/*
sq_connect_to_server
Squirrel: connect_to_server()
Connects to the server. Returns true on success.
*/
SQInteger sq_connect_to_server(HSQUIRRELVM vm)
{
  sq_pushbool(vm, enetclient_init() ? SQTrue : SQFalse);
  return 1;
};

// -----------------------------------------------------------------------
// VM Lifecycle
// -----------------------------------------------------------------------

/*
sqRunFunc
Calls a named function in a Squirrel VM.
Optionally passes dt as a float argument.
*/
void sqRunFunc(HSQUIRRELVM vm, const char *func_name, float dt)
{
  if (!vm)
    return;

  sq_pushroottable(vm);
  sq_pushstring(vm, _SC(func_name), -1);

  if (SQ_FAILED(sq_get(vm, -2)))
  {
    sq_pop(vm, 1);
    return;
  }

  sq_pushroottable(vm);

  if (dt == -1.0f)
    sq_call(vm, 1, SQFalse, SQTrue);
  else
  {
    sq_pushfloat(vm, dt);
    sq_call(vm, 2, SQFalse, SQTrue);
  }

  sq_pop(vm, 2);
};

// -----------------------------------------------------------------------
// Print / Error Handlers
// -----------------------------------------------------------------------

/*
sqPrintFunc_Server
Called when a server-side script calls print().
Prefixes output with [SERVER]:
*/
void sqPrintFunc_Server(HSQUIRRELVM /*vm*/, const SQChar *s, ...)
{
  printf("[SERVER]: ");
  va_list args;
  va_start(args, s);
  vfprintf(stdout, s, args);
  va_end(args);
  fflush(stdout);
};

/*
sqPrintFunc_Client
Called when a client-side script calls print().
Prefixes output with [CLIENT]:
*/
void sqPrintFunc_Client(HSQUIRRELVM /*vm*/, const SQChar *s, ...)
{
  printf("[CLIENT]: ");
  va_list args;
  va_start(args, s);
  vfprintf(stdout, s, args);
  va_end(args);
  fflush(stdout);
};

/*
sqErrorFunc
Called on runtime errors in any VM.
*/
void sqErrorFunc(HSQUIRRELVM /*vm*/, const SQChar *s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  va_end(args);
};

/*
sqCompilerErrorHandler
Called when a .nut file fails to compile.
*/
void sqCompilerErrorHandler(HSQUIRRELVM /*v*/, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column)
{
  printf("SCRIPTERROR: %s in %s (Line: %d, Col: %d)\n", desc, source, (int)line, (int)column);
};

// -----------------------------------------------------------------------
// Script Loading
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// BSP Entity Spawning
// -----------------------------------------------------------------------

/*
sqCallEntitySpawnerInVM
Calls a BSP entity spawner function in a specific VM if it exists.
*/
static void sqCallEntitySpawnerInVM(HSQUIRRELVM vm, const std::string &classname, Vector3 origin, const std::unordered_map<std::string, std::string> &tags)
{
  if (!vm)
    return;

  sq_pushroottable(vm);
  sq_pushstring(vm, classname.c_str(), -1);

  if (SQ_FAILED(sq_get(vm, -2)))
  {
    sq_pop(vm, 1);
    return;
  }

  sq_pushroottable(vm); // this

  // arg 1: origin table {x, y, z}
  sq_newtable(vm);
  sq_pushstring(vm, "x", -1);
  sq_pushfloat(vm, origin.x);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "y", -1);
  sq_pushfloat(vm, origin.y);
  sq_newslot(vm, -3, SQFalse);
  sq_pushstring(vm, "z", -1);
  sq_pushfloat(vm, origin.z);
  sq_newslot(vm, -3, SQFalse);

  // arg 2: tags table
  sq_newtable(vm);
  for (auto &[key, val] : tags)
  {
    sq_pushstring(vm, key.c_str(), -1);
    sq_pushstring(vm, val.c_str(), -1);
    sq_newslot(vm, -3, SQFalse);
  }

  sq_call(vm, 3, SQFalse, SQTrue);
  sq_pop(vm, 2);
};

/*
sqCallEntitySpawner
Calls matching BSP entity function in server VM (game logic)
then client VM (visuals).
*/
void sqCallEntitySpawner(const std::string &classname, Vector3 origin, const std::unordered_map<std::string, std::string> &tags)
{
  sqCallEntitySpawnerInVM(server_vm, classname, origin, tags);
  sqCallEntitySpawnerInVM(client_vm, classname, origin, tags);
};

/*
sqSpawnBSPEntities
Call after loading a BSP map. Iterates all entities and calls
any matching Squirrel function found in the client VM.
*/
void sqSpawnBSPEntities()
{
  if (!bsp_renderer.bsp_file)
    return;

  auto entities = bsp_renderer.bsp_file->entities();

  for (auto &e : entities)
  {
    auto class_it = e.tags.find("classname");
    if (class_it == e.tags.end())
      continue;

    const std::string &classname = class_it->second;

    // parse origin
    Vector3 origin = {0, 0, 0};
    auto org = e.tags.find("origin");
    if (org != e.tags.end())
    {
      float qx = 0, qy = 0, qz = 0;
      sscanf(org->second.c_str(), "%f %f %f", &qx, &qy, &qz);
      origin = FromQuake({qx, qy, qz});
    }

    // Now do something with the data, spawn an instance, etc.
    sqCallEntitySpawner(classname, origin, e.tags);
  }
};

/*
sqLoadNutFile
Loads and executes a single .nut file into a VM.
Resolves path based on vm_type: server/, client/, or shared/
*/
void sqLoadNutFile(HSQUIRRELVM vm, std::string nut_filename, VM_TYPE vm_type)
{
  SQInteger top = sq_gettop(vm);

  std::string folder;
  if (vm_type == SERVER)
    folder = "scripts/server/";
  else if (vm_type == CLIENT)
    folder = "scripts/client/";
  else
    folder = "scripts/shared/";

  std::string full_path = GAMEDATA_PATH + folder + nut_filename;
  printf("DEBUG: Loading: %s\n", std::filesystem::absolute(full_path).string().c_str());

  if (SQ_SUCCEEDED(sqstd_loadfile(vm, full_path.c_str(), SQTrue)))
  {
    sq_pushroottable(vm);
    if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
      printf("SCRIPTERROR: Execution of %s failed.\n", nut_filename.c_str());
  }
  else
    printf("SCRIPTERROR: Could not load file at %s\n", full_path.c_str());

  sq_settop(vm, top);
};

/*
sqLoadShared
Loads all scripts listed in sh_manifest.cfg into the given VM.
Call for both client and server so shared constants/utils are available on both sides.
*/
void sqLoadShared(HSQUIRRELVM vm)
{
  std::string fullPath = GAMEDATA_PATH + "scripts/sh_manifest.cfg";
  std::ifstream file(fullPath);

  if (!file.is_open())
  {
    printf("CRITICAL: Could not open shared manifest at %s\n", fullPath.c_str());
    return;
  }

  std::string line;
  while (std::getline(file, line))
  {
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    if (!line.empty() && line[0] != '#')
    {
      SQInteger top = sq_gettop(vm);
      std::string full_path = GAMEDATA_PATH + "scripts/shared/" + line;

      if (SQ_SUCCEEDED(sqstd_loadfile(vm, full_path.c_str(), SQTrue)))
      {
        sq_pushroottable(vm);
        if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
          printf("SCRIPTERROR: Execution of shared/%s failed.\n", line.c_str());
      }
      else
        printf("SCRIPTERROR: Could not load shared/%s\n", line.c_str());

      sq_settop(vm, top);
    }
  }
};

/*
sqLoadManifest
Loads all scripts listed in a manifest file into the appropriate VM.
*/
void sqLoadManifest(const std::string &manifestName, VM_TYPE vm_type)
{
  std::string fullPath = GAMEDATA_PATH + "scripts/" + manifestName;
  std::ifstream file(fullPath);

  if (!file.is_open())
  {
    printf("CRITICAL: Could not open script manifest at %s\n", fullPath.c_str());
    return;
  }

  HSQUIRRELVM target = (vm_type == SERVER) ? server_vm : client_vm;

  std::string line;
  while (std::getline(file, line))
  {
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    if (!line.empty() && line[0] != '#')
      sqLoadNutFile(target, line, vm_type);
  }
};

// -----------------------------------------------------------------------
// VM Setup
// -----------------------------------------------------------------------

static void sq_register_func(HSQUIRRELVM v, SQFUNCTION func, const char *name)
{
  sq_pushstring(v, _SC(name), -1);
  sq_newclosure(v, func, 0);
  sq_newslot(v, -3, SQFalse);
};

/*
register_raylib_colors
Registers Raylib colors into a global table for Squirrel
*/
static void register_raylib_colors(HSQUIRRELVM v)
{
  auto add_color = [&](const char *name, Color c)
  {
    sq_pushstring(v, _SC(name), -1);
    sq_newtable(v);

    auto set_field = [&](const char *field, int val)
    {
      sq_pushstring(v, _SC(field), -1);
      sq_pushinteger(v, val);
      sq_newslot(v, -3, SQFalse);
    };

    set_field("r", c.r);
    set_field("g", c.g);
    set_field("b", c.b);
    set_field("a", c.a);

    sq_newslot(v, -3, SQFalse);
  };

  sq_pushstring(v, _SC("COLOR"), -1);
  sq_newtable(v);

  add_color("LIGHTGRAY", LIGHTGRAY);
  add_color("GRAY", GRAY);
  add_color("DARKGRAY", DARKGRAY);
  add_color("YELLOW", YELLOW);
  add_color("GOLD", GOLD);
  add_color("ORANGE", ORANGE);
  add_color("PINK", PINK);
  add_color("RED", RED);
  add_color("MAROON", MAROON);
  add_color("GREEN", GREEN);
  add_color("LIME", LIME);
  add_color("DARKGREEN", DARKGREEN);
  add_color("SKYBLUE", SKYBLUE);
  add_color("BLUE", BLUE);
  add_color("DARKBLUE", DARKBLUE);
  add_color("PURPLE", PURPLE);
  add_color("VIOLET", VIOLET);
  add_color("DARKPURPLE", DARKPURPLE);
  add_color("BEIGE", BEIGE);
  add_color("BROWN", BROWN);
  add_color("DARKBROWN", DARKBROWN);
  add_color("WHITE", WHITE);
  add_color("BLACK", BLACK);
  add_color("BLANK", BLANK);
  add_color("MAGENTA", MAGENTA);
  add_color("RAYWHITE", RAYWHITE);

  sq_newslot(v, -3, SQFalse);
};

static void register_raylib_mousebuttons(HSQUIRRELVM v)
{
  auto add_key = [&](const char *name, int val)
  {
    sq_pushstring(v, _SC(name), -1);
    sq_pushinteger(v, val);
    sq_newslot(v, -3, SQFalse);
  };
  // ----- MOUSE_BUTTON table -----
  sq_pushstring(v, _SC("MOUSE_BUTTON"), -1);
  sq_newtable(v);
  add_key("LEFT", 0);
  add_key("RIGHT", 1);
  add_key("MIDDLE", 2);
  add_key("SIDE", 3);
  add_key("EXTRA", 4);
  add_key("FORWARD", 5);
  add_key("BACK", 6);
  sq_newslot(v, -3, SQFalse);
};

static void register_keys(HSQUIRRELVM v)
{
  auto add_key = [&](const char *name, int val)
  {
    sq_pushstring(v, _SC(name), -1);
    sq_pushinteger(v, val);
    sq_newslot(v, -3, SQFalse);
  };
  // ----- KEY table -----
  sq_pushstring(v, _SC("KEY"), -1);
  sq_newtable(v);
  // Alphanumeric
  add_key("APOSTROPHE", 39);
  add_key("COMMA", 44);
  add_key("MINUS", 45);
  add_key("PERIOD", 46);
  add_key("SLASH", 47);
  add_key("ZERO", 48);
  add_key("ONE", 49);
  add_key("TWO", 50);
  add_key("THREE", 51);
  add_key("FOUR", 52);
  add_key("FIVE", 53);
  add_key("SIX", 54);
  add_key("SEVEN", 55);
  add_key("EIGHT", 56);
  add_key("NINE", 57);
  add_key("SEMICOLON", 59);
  add_key("EQUAL", 61);

  // Letters A-Z
  for (int i = 0; i < 26; i++)
  {
    char name[] = "A";
    name[0] += i;
    add_key(name, 65 + i);
  }

  // Functional
  add_key("SPACE", 32);
  add_key("ESCAPE", 256);
  add_key("ENTER", 257);
  add_key("TAB", 258);
  add_key("BACKSPACE", 259);
  add_key("INSERT", 260);
  add_key("DELETE", 261);
  add_key("RIGHT", 262);
  add_key("LEFT", 263);
  add_key("DOWN", 264);
  add_key("UP", 265);

  // F1-F12
  for (int i = 1; i <= 12; i++)
  {
    std::string fkey = "F" + std::to_string(i);
    add_key(fkey.c_str(), 289 + i);
  }

  // Modifiers
  add_key("LEFT_SHIFT", 340);
  add_key("LEFT_CONTROL", 341);
  add_key("LEFT_ALT", 342);
  add_key("RIGHT_SHIFT", 344);

  sq_newslot(v, -3, SQFalse); // push KEY table
};

static void register_packets(HSQUIRRELVM v)
{
  auto add_key = [&](const char *name, int val)
  {
    sq_pushstring(v, _SC(name), -1);
    sq_pushinteger(v, val);
    sq_newslot(v, -3, SQFalse);
  };
  // ----- PACKET table -----
  sq_pushstring(v, _SC("PACKET_TYPE"), -1);
  sq_newtable(v);
  add_key("NUMBER", 0);
  add_key("VECTOR3", 1);
  add_key("STRING", 2);
  add_key("BOOL", 3);
  sq_newslot(v, -3, SQFalse);
};

static void register_gamepad_enums(HSQUIRRELVM v)
{
  auto add_key = [&](const char *name, int val)
  {
    sq_pushstring(v, _SC(name), -1);
    sq_pushinteger(v, val);
    sq_newslot(v, -3, SQFalse);
  };
  // ----- GAMEPAD_BUTTON table -----
  sq_pushstring(v, _SC("GAMEPAD_BUTTON"), -1);
  sq_newtable(v);

  add_key("UNKNOWN", 0);
  // Left side (D-Pad)
  add_key("LEFT_FACE_UP", 1);
  add_key("LEFT_FACE_RIGHT", 2);
  add_key("LEFT_FACE_DOWN", 3);
  add_key("LEFT_FACE_LEFT", 4);
  // Right side (Action buttons)
  add_key("RIGHT_FACE_UP", 5);
  add_key("RIGHT_FACE_RIGHT", 6);
  add_key("RIGHT_FACE_DOWN", 7);
  add_key("RIGHT_FACE_LEFT", 8);
  // Shoulders/Triggers
  add_key("LEFT_TRIGGER_1", 9);
  add_key("LEFT_TRIGGER_2", 10);
  add_key("RIGHT_TRIGGER_1", 11);
  add_key("RIGHT_TRIGGER_2", 12);
  // Center buttons
  add_key("MIDDLE_LEFT", 13);
  add_key("MIDDLE", 14);
  add_key("MIDDLE_RIGHT", 15);

  sq_newslot(v, -3, SQFalse);

  // ----- GAMEPAD_AXIS table -----
  sq_pushstring(v, _SC("GAMEPAD_AXIS"), -1);
  sq_newtable(v);

  add_key("LEFT_X", 0);
  add_key("LEFT_Y", 1);
  add_key("RIGHT_X", 2);
  add_key("RIGHT_Y", 3);
  add_key("LEFT_TRIGGER", 4);
  add_key("RIGHT_TRIGGER", 5);

  sq_newslot(v, -3, SQFalse);
}

/*
sqSetupVM
Creates and configures a Squirrel VM for the given side ("SERVER" or "CLIENT").
Registers all C++ functions, key constants, and side identifier.
*/
static HSQUIRRELVM sqSetupVM(const char *side_name)
{
  HSQUIRRELVM v = sq_open(1024);
  sqstd_seterrorhandlers(v);

  if (std::string(side_name) == "SERVER")
    sq_setprintfunc(v, sqPrintFunc_Server, sqErrorFunc);
  else
    sq_setprintfunc(v, sqPrintFunc_Client, sqErrorFunc);

  sq_setcompilererrorhandler(v, sqCompilerErrorHandler);

  sq_pushroottable(v);
  sqstd_register_mathlib(v);
  sqstd_register_stringlib(v);
  sqstd_register_iolib(v);

  // create global enums
  register_raylib_colors(v);
  register_raylib_mousebuttons(v);
  register_packets(v);
  register_keys(v);
  register_gamepad_enums(v);

  // ----- Server-only table -----
  if (std::string(side_name) == "SERVER")
  {
    // network
    sq_register_func(v, sq_start_server, "start_server");
    sq_register_func(v, sq_sendflags_sync, "sendflags_sync");

    // instances
    sq_register_func(v, sq_get_player_count, "get_player_count");
    sq_register_func(v, sq_instance_create, "instance_create");
    sq_register_func(v, sq_instances_get, "instances_get_all");
    sq_register_func(v, sq_instances_get, "instances_get");
    sq_register_func(v, sq_instances_get, "get_instance_count");
    sq_register_func(v, sq_get_player_instance, "get_player_instance");
  }

  // ----- Client-only table -----
  if (std::string(side_name) == "CLIENT")
  {
    // network
    sq_register_func(v, sq_connect_to_server, "connect_to_server");

    // input mouse
    sq_register_func(v, sq_is_mouse_button_pressed, "is_mouse_button_pressed");
    sq_register_func(v, sq_is_mouse_button_down, "is_mouse_button_down");
    sq_register_func(v, sq_is_mouse_button_released, "is_mouse_button_released");
    sq_register_func(v, sq_is_mouse_button_up, "is_mouse_button_up");
    sq_register_func(v, sq_get_mouse_x, "get_mouse_x");
    sq_register_func(v, sq_get_mouse_y, "get_mouse_y");
    sq_register_func(v, sq_get_mouse_delta, "get_mouse_delta");
    sq_register_func(v, sq_get_mouse_wheel_move, "get_mouse_wheel_move");
    sq_register_func(v, sq_set_mouse_cursor, "set_mouse_cursor");

    // input keys
    sq_register_func(v, sq_is_key_pressed, "is_key_pressed");
    sq_register_func(v, sq_is_key_down, "is_key_down");
    sq_register_func(v, sq_is_key_released, "is_key_released");
    sq_register_func(v, sq_is_key_up, "is_key_up");

    // gamepad
    sq_register_func(v, sq_is_gamepad_available, "is_gamepad_available");
    sq_register_func(v, sq_get_gamepad_name, "get_gamepad_name");
    sq_register_func(v, sq_is_gamepad_button_pressed, "is_gamepad_button_pressed");
    sq_register_func(v, sq_is_gamepad_button_down, "is_gamepad_button_down");
    sq_register_func(v, sq_is_gamepad_button_released, "is_gamepad_button_released");
    sq_register_func(v, sq_is_gamepad_button_up, "is_gamepad_button_up");
    sq_register_func(v, sq_get_gamepad_button_pressed, "get_gamepad_button_pressed");
    sq_register_func(v, sq_get_gamepad_axis_count, "get_gamepad_axis_count");
    sq_register_func(v, sq_get_gamepad_axis_movement, "get_gamepad_axis_movement");
    sq_register_func(v, sq_set_gamepad_vibration, "set_gamepad_vibration");

    // cursor
    sq_register_func(v, sq_show_cursor, "show_cursor");
    sq_register_func(v, sq_hide_cursor, "hide_cursor");
    sq_register_func(v, sq_is_cursor_hidden, "is_cursor_hidden");
    sq_register_func(v, sq_enable_cursor, "enable_cursor");
    sq_register_func(v, sq_disable_cursor, "disable_cursor");
    sq_register_func(v, sq_is_cursor_on_screen, "is_cursor_on_screen");

    // system
    sq_register_func(v, sq_get_camera_position, "get_camera_position");

    // drawing
    sq_register_func(v, sq_draw_fps, "draw_fps");
    sq_register_func(v, sq_clear_background, "clear_background");
    sq_register_func(v, sq_begin_scissor_mode, "begin_scissor_mode");
    sq_register_func(v, sq_end_scissor_mode, "end_scissor_mode");

    // window
    sq_register_func(v, sq_toggle_fullscreen, "toggle_fullscreen");
    sq_register_func(v, sq_toggle_borderless_windowed, "toggle_borderless_windowed");
    sq_register_func(v, sq_maximize_window, "maximize_window");
    sq_register_func(v, sq_minimize_window, "minimize_window");
    sq_register_func(v, sq_restore_window, "restore_window");
    sq_register_func(v, sq_set_window_icon, "set_window_icon");
    sq_register_func(v, sq_set_window_title, "set_window_title");
    sq_register_func(v, sq_set_window_position, "set_window_position");
    sq_register_func(v, sq_get_screen_width, "get_screen_width");
    sq_register_func(v, sq_get_screen_height, "get_screen_height");
    sq_register_func(v, sq_get_render_width, "get_render_width");
    sq_register_func(v, sq_get_render_height, "get_render_height");

    // rtext
    sq_register_func(v, sq_load_font, "load_font");
    sq_register_func(v, sq_draw_text, "draw_text");

    // rshapes
    sq_register_func(v, sq_draw_rectangle, "draw_rectangle");

    // rmodels
    sq_register_func(v, sq_draw_cube, "draw_cube");
  };

  // ----- SIDE constant  | Returns CLIENT OR SERVER -----
  sq_pushstring(v, "SIDE", -1);
  sq_pushstring(v, side_name, -1);
  sq_newslot(v, -3, SQFalse);

  // ----- Global functions (both sides) -----
  // networking
  sq_register_func(v, sq_send_packet_number, "send_packet_number");
  sq_register_func(v, sq_send_packet_vector3, "send_packet_vector3");
  sq_register_func(v, sq_get_packet, "get_packet");

  // timing
  sq_register_func(v, sq_set_target_fps, "set_target_fps");
  sq_register_func(v, sq_get_frame_time, "get_frame_time");
  sq_register_func(v, sq_get_time, "get_time");
  sq_register_func(v, sq_get_fps, "get_fps");

  // rng
  sq_register_func(v, sq_set_random_seed, "set_random_seed");
  sq_register_func(v, sq_get_random_value, "get_random_value");

  sq_pop(v, 1);
  return v;
};

// -----------------------------------------------------------------------
// Public Entry Points
// -----------------------------------------------------------------------

/*
sqStartServer
Call once when the server starts.
Sets up the server VM, loads shared + server scripts, and calls init().
*/
void sqStartServer()
{
  if (server_vm)
    return;
  server_vm = sqSetupVM("SERVER");
  sqLoadShared(server_vm);
  sqLoadManifest("sv_manifest.cfg", SERVER);
  sqRunFunc(server_vm, "init");
};

/*
sqStartClient
Call once at program start.
Sets up the client VM and loads shared + client scripts.
init() is deferred until the server assigns an ID (see sqUpdate).
*/
void sqStartClient()
{
  client_vm = sqSetupVM("CLIENT");
  sqLoadShared(client_vm);
  sqLoadManifest("cs_manifest.cfg", CLIENT);
};

/*
sqUpdate
Called every frame. Drives ENet polling, deferred client init, and VM update().
*/
void sqUpdate(float dt)
{
  if (server_online && global_is_hosting)
    enetserver_update();

  enetclient_update();

  // Defer client init() until server has assigned our player ID
  static bool sq_client_init_done = false;
  bool has_id = (my_local_player_id >= 0);
  if (!sq_client_init_done && global_client_init_called)
  {
    sqRunFunc(client_vm, "init");
    sq_client_init_done = true;

    sqSpawnBSPEntities();
  }

  if (global_is_hosting)
    sqRunFunc(server_vm, "update", dt);

  sqRunFunc(client_vm, "update", dt);
};

/*
sqDraw
Called every frame inside BeginMode3D / BeginDrawing.
*/
void sqDraw(float dt)
{
  sqRunFunc(client_vm, "draw", dt);
};

/*
sqDrawGUI
Called every frame for 2D/HUD drawing.
*/
void sqDrawGUI(float dt)
{
  sqRunFunc(client_vm, "draw_gui", dt);
};