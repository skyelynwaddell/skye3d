// lua.cpp — sol2 + Lua 5.4 port of sq.cpp
//
// Design notes:
//   * One sol::state per VM (client_lua / server_lua). Constructed in
//     luaStartServer() / luaStartClient().
//   * GameObject3D is registered as a sol2 usertype — scripts call methods
//     naturally: obj:set_position(x,y,z), obj:get("key"), obj:destroy(), ...
//     No manual stack management.
//   * Enum-like tables (COLOR, KEY, MOUSE_BUTTON, GAMEPAD_BUTTON, GAMEPAD_AXIS,
//     PACKET_TYPE) are created as Lua tables at VM setup.
//   * `print()` is overridden per-VM so server prints [SERVER]: and client
//     prints [CLIENT]:.
//   * `set_think(func)` stores a sol::protected_function in an engine-owned
//     per-object map. Calling luaClearThinks(obj) on destruction clears it.
//   * Script files end in .lua instead of .nut. Paths are otherwise identical
//     (gamedata/scripts/{server,client,shared}/...).
//
// All global function names match the Squirrel versions, so porting .nut files
// to .lua is a largely syntax-level rewrite:
//   this.set("k", v)          -> self:set("k", v)
//   foo <- bar                -> foo = bar  (Lua globals)
//   function f(x) {...}       -> function f(x) ... end
//   print("x\n")              -> print("x")         (Lua print adds newline)

#define SOL_ALL_SAFETIES_ON 1
#define SOL_USE_STD_OPTIONAL 1
#define SOL_LUA_VERSION 504

#include <sol/sol.hpp>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <algorithm>

#include "global.h"
#include "enet.h"
#include "net_utils.h"
#include "camera3d.h"
#include <brush_entity.h>
#include "bsp.h"

#ifndef RAYLIB_H
#include "raylib.h"
#endif

#include "raymath.h"

// -----------------------------------------------------------------------
// Globals (defined here; declared in lua.h)
// -----------------------------------------------------------------------

static inline std::string GAMEDATA_PATH = "gamedata/";

// Per-object think function tables. sol::protected_function holds a Lua
// reference and will not be garbage-collected while it lives here.
// Keyed by GameObject3D* so engine code can call luaClearThinks(obj).
static std::unordered_map<GameObject3D *, sol::protected_function> g_server_thinks;
static std::unordered_map<GameObject3D *, sol::protected_function> g_client_thinks;

void luaClearThinks(GameObject3D *obj)
{
  g_server_thinks.erase(obj);
  g_client_thinks.erase(obj);
}

// -----------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------

// Read a raylib Color from a Lua table {r=,g=,b=,a=}. Missing alpha => 255.
static Color LuaGetColor(const sol::table &t)
{
  Color c = {0, 0, 0, 255};
  c.r = (unsigned char)t.get_or("r", 0);
  c.g = (unsigned char)t.get_or("g", 0);
  c.b = (unsigned char)t.get_or("b", 0);
  c.a = (unsigned char)t.get_or("a", 255);
  return c;
}

// Read a Vector3 from a Lua table {x=,y=,z=}.
static Vector3 LuaGetVector3(const sol::table &t)
{
  return Vector3{
      (float)t.get_or("x", 0.0),
      (float)t.get_or("y", 0.0),
      (float)t.get_or("z", 0.0),
  };
}

// Build a Lua table {x=,y=,z=} from a Vector3 using the given state.
static sol::table Vec3ToTable(sol::state_view lua, Vector3 v)
{
  sol::table t = lua.create_table();
  t["x"] = v.x;
  t["y"] = v.y;
  t["z"] = v.z;
  return t;
}

// Script_vars <-> Lua value bridging -----------------------------------

static sol::object ScriptValueToLua(sol::state_view lua, const ScriptValue &sv)
{
  return std::visit([&lua](auto &&arg) -> sol::object
                    {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, std::string>)
      return sol::make_object(lua, arg);
    else if constexpr (std::is_same_v<T, float>)
      return sol::make_object(lua, (double)arg);
    else if constexpr (std::is_same_v<T, int>)
      return sol::make_object(lua, arg);
    else if constexpr (std::is_same_v<T, bool>)
      return sol::make_object(lua, arg);
    else
      return sol::lua_nil; }, sv);
}

static std::optional<ScriptValue> LuaToScriptValue(const sol::object &o)
{
  if (o.is<bool>())
    return ScriptValue{(bool)o.as<bool>()};
  if (o.is<int>())
    return ScriptValue{(int)o.as<int>()};
  if (o.is<double>())
    return ScriptValue{(float)o.as<double>()};
  if (o.is<std::string>())
    return ScriptValue{o.as<std::string>()};
  return std::nullopt;
}

// -----------------------------------------------------------------------
// Forward decls for setup chunks
// -----------------------------------------------------------------------
static void register_enums(sol::state &lua);
static void register_gameobject3d_usertype(sol::state &lua, bool is_server);
static void bind_globals(sol::state &lua, bool is_server);
static void install_print_override(sol::state &lua, const char *prefix);

// -----------------------------------------------------------------------
// Enum / constant registration (COLOR, KEY, MOUSE_BUTTON, GAMEPAD_*, PACKET_TYPE)
// -----------------------------------------------------------------------
static sol::table color_to_table(sol::state &lua, Color c)
{
  sol::table t = lua.create_table();
  t["r"] = (int)c.r;
  t["g"] = (int)c.g;
  t["b"] = (int)c.b;
  t["a"] = (int)c.a;
  return t;
}

static void register_enums(sol::state &lua)
{
  // ---- COLOR ----
  sol::table color = lua.create_named_table("COLOR");
  color["LIGHTGRAY"] = color_to_table(lua, LIGHTGRAY);
  color["GRAY"] = color_to_table(lua, GRAY);
  color["DARKGRAY"] = color_to_table(lua, DARKGRAY);
  color["YELLOW"] = color_to_table(lua, YELLOW);
  color["GOLD"] = color_to_table(lua, GOLD);
  color["ORANGE"] = color_to_table(lua, ORANGE);
  color["PINK"] = color_to_table(lua, PINK);
  color["RED"] = color_to_table(lua, RED);
  color["MAROON"] = color_to_table(lua, MAROON);
  color["GREEN"] = color_to_table(lua, GREEN);
  color["LIME"] = color_to_table(lua, LIME);
  color["DARKGREEN"] = color_to_table(lua, DARKGREEN);
  color["SKYBLUE"] = color_to_table(lua, SKYBLUE);
  color["BLUE"] = color_to_table(lua, BLUE);
  color["DARKBLUE"] = color_to_table(lua, DARKBLUE);
  color["PURPLE"] = color_to_table(lua, PURPLE);
  color["VIOLET"] = color_to_table(lua, VIOLET);
  color["DARKPURPLE"] = color_to_table(lua, DARKPURPLE);
  color["BEIGE"] = color_to_table(lua, BEIGE);
  color["BROWN"] = color_to_table(lua, BROWN);
  color["DARKBROWN"] = color_to_table(lua, DARKBROWN);
  color["WHITE"] = color_to_table(lua, WHITE);
  color["BLACK"] = color_to_table(lua, BLACK);
  color["BLANK"] = color_to_table(lua, BLANK);
  color["MAGENTA"] = color_to_table(lua, MAGENTA);
  color["RAYWHITE"] = color_to_table(lua, RAYWHITE);

  // ---- MOUSE_BUTTON ----
  sol::table mb = lua.create_named_table("MOUSE_BUTTON");
  mb["LEFT"] = 0;
  mb["RIGHT"] = 1;
  mb["MIDDLE"] = 2;
  mb["SIDE"] = 3;
  mb["EXTRA"] = 4;
  mb["FORWARD"] = 5;
  mb["BACK"] = 6;

  // ---- PACKET_TYPE ----
  sol::table pt = lua.create_named_table("PACKET_TYPE");
  pt["NUMBER"] = 0;
  pt["VECTOR3"] = 1;
  pt["STRING"] = 2;
  pt["BOOL"] = 3;

  // ---- KEY (raylib key codes) ----
  sol::table key = lua.create_named_table("KEY");
  key["APOSTROPHE"] = 39;
  key["COMMA"] = 44;
  key["MINUS"] = 45;
  key["PERIOD"] = 46;
  key["SLASH"] = 47;
  key["ZERO"] = 48;
  key["ONE"] = 49;
  key["TWO"] = 50;
  key["THREE"] = 51;
  key["FOUR"] = 52;
  key["FIVE"] = 53;
  key["SIX"] = 54;
  key["SEVEN"] = 55;
  key["EIGHT"] = 56;
  key["NINE"] = 57;
  key["SEMICOLON"] = 59;
  key["EQUAL"] = 61;
  // Letters A-Z
  for (int i = 0; i < 26; i++)
  {
    char name[2] = {(char)('A' + i), 0};
    key[name] = 65 + i;
  }
  // Functional
  key["SPACE"] = 32;
  key["ESCAPE"] = 256;
  key["ENTER"] = 257;
  key["TAB"] = 258;
  key["BACKSPACE"] = 259;
  key["INSERT"] = 260;
  key["DELETE"] = 261;
  key["RIGHT"] = 262;
  key["LEFT"] = 263;
  key["DOWN"] = 264;
  key["UP"] = 265;
  // F1..F12
  for (int i = 1; i <= 12; i++)
  {
    std::string fkey = "F" + std::to_string(i);
    key[fkey] = 289 + i;
  }
  // Modifiers
  key["LEFT_SHIFT"] = 340;
  key["LEFT_CONTROL"] = 341;
  key["LEFT_ALT"] = 342;
  key["RIGHT_SHIFT"] = 344;

  // ---- GAMEPAD_BUTTON ----
  sol::table gb = lua.create_named_table("GAMEPAD_BUTTON");
  gb["UNKNOWN"] = 0;
  gb["LEFT_FACE_UP"] = 1;
  gb["LEFT_FACE_RIGHT"] = 2;
  gb["LEFT_FACE_DOWN"] = 3;
  gb["LEFT_FACE_LEFT"] = 4;
  gb["RIGHT_FACE_UP"] = 5;
  gb["RIGHT_FACE_RIGHT"] = 6;
  gb["RIGHT_FACE_DOWN"] = 7;
  gb["RIGHT_FACE_LEFT"] = 8;
  gb["LEFT_TRIGGER_1"] = 9;
  gb["LEFT_TRIGGER_2"] = 10;
  gb["RIGHT_TRIGGER_1"] = 11;
  gb["RIGHT_TRIGGER_2"] = 12;
  gb["MIDDLE_LEFT"] = 13;
  gb["MIDDLE"] = 14;
  gb["MIDDLE_RIGHT"] = 15;

  // ---- GAMEPAD_AXIS ----
  sol::table ga = lua.create_named_table("GAMEPAD_AXIS");
  ga["LEFT_X"] = 0;
  ga["LEFT_Y"] = 1;
  ga["RIGHT_X"] = 2;
  ga["RIGHT_Y"] = 3;
  ga["LEFT_TRIGGER"] = 4;
  ga["RIGHT_TRIGGER"] = 5;
}

// -----------------------------------------------------------------------
// GameObject3D usertype
// -----------------------------------------------------------------------
// Must be registered per-VM because sol2 stores the usertype in the Lua state's
// registry. We register the same methods on both, but the think function the
// script passes to set_think is bound to the VM that registered it.
static void register_gameobject3d_usertype(sol::state &lua, bool is_server)
{
  lua.new_usertype<GameObject3D>(
      "GameObject3D",
      sol::no_constructor,

      // ----- data members (direct property access) -----
      "client_id", &GameObject3D::client_id,
      "is_me", &GameObject3D::is_me,
      "destroy_me", &GameObject3D::destroy_me,
      "classname", &GameObject3D::classname,
      "target_name", &GameObject3D::target_name,
      "target", &GameObject3D::target,
      "sendflags", &GameObject3D::sendflags,

      // ----- generic script_vars interface (maps to set()/get()) -----
      "set", [](GameObject3D &self, const std::string &key, sol::object value)
      {
        if (auto sv = LuaToScriptValue(value))
          self.script_vars[key] = *sv; },
      "get", [](GameObject3D &self, const std::string &key, sol::this_state ts) -> sol::object
      {
        sol::state_view lv(ts);
        auto it = self.script_vars.find(key);
        if (it == self.script_vars.end())
          return sol::lua_nil;
        return ScriptValueToLua(lv, it->second); },

      // ----- lifecycle -----
      "destroy", [](GameObject3D &self)
      {
        self.Destroy();
        luaClearThinks(&self); },
      "is_valid", [](GameObject3D & /*self*/)
      { return true; }, // sol2 nil-checks for you

      // ----- target name / target -----
      "get_target_name", [](GameObject3D &self)
      { return self.target_name; },
      "set_target_name", [](GameObject3D &self, const std::string &v)
      { self.target_name = v; },
      "get_target", [](GameObject3D &self)
      { return self.target; },
      "set_target", [](GameObject3D &self, const std::string &v)
      { self.target = v; },

      // ----- position -----
      "set_position", [](GameObject3D &self, double x, double y, double z)
      { self.position = Vector3{(float)x, (float)y, (float)z}; },
      "get_position", [](GameObject3D &self, sol::this_state ts)
      { return Vec3ToTable(sol::state_view(ts), self.position); },

      // ----- velocity -----
      "set_velocity", [](GameObject3D &self, double x, double y, double z)
      { self.velocity = Vector3{(float)x, (float)y, (float)z}; },
      "get_velocity", [](GameObject3D &self, sol::this_state ts)
      { return Vec3ToTable(sol::state_view(ts), self.velocity); },

      // ----- size -----
      "set_size", [](GameObject3D &self, double x, double y, double z)
      { self.size = Vector3{(float)x, (float)y, (float)z}; },
      "get_size", [](GameObject3D &self, sol::this_state ts)
      { return Vec3ToTable(sol::state_view(ts), self.size); },

      // ----- acceleration / speed -----
      "set_acceleration", [](GameObject3D &self, double a)
      { self.acceleration = (float)a; },
      "get_acceleration", [](GameObject3D &self)
      { return (double)self.acceleration; },
      "set_speed", [](GameObject3D &self, double s)
      { self.speed = (float)s; },
      "get_speed", [](GameObject3D &self)
      { return (double)self.speed; },

      // ----- collision box -----
      "set_collision_box", [](GameObject3D &self, double x, double y, double z)
      { self.collision_box = Vector3{(float)x, (float)y, (float)z}; },
      "get_collision_box", [](GameObject3D &self, sol::this_state ts)
      {
        sol::state_view lv(ts);
        sol::table t = lv.create_table();
        t["width"]  = self.collision_box.x;
        t["height"] = self.collision_box.y;
        t["length"] = self.collision_box.z;
        return t; },

      // ----- classname -----
      "get_classname", [](GameObject3D &self)
      { return self.classname; },
      "set_classname", [](GameObject3D &self, const std::string &v)
      { self.classname = v; },

      // ----- think function -----
      // Scripts call: obj:set_think(function(self, dt) ... end)
      // The closure is stored in the engine-side map and will be called every
      // frame by luaRunThinks() (wired into sqUpdate/luaUpdate).
      "set_think", [is_server](GameObject3D &self, sol::protected_function fn)
      {
        if (is_server)
        {
          g_server_thinks[&self] = std::move(fn);
          self.has_server_think = true;
        }
        else
        {
          g_client_thinks[&self] = std::move(fn);
          self.has_client_think = true;
        } },

      // ----- legacy print helper (some scripts call this) -----
      "print", [](GameObject3D &self)
      { printf("GameObject3D classname='%s' client_id=%d client_id=%d pos=(%.2f,%.2f,%.2f)\n",
               self.classname.c_str(), self.client_id,
               self.position.x, self.position.y, self.position.z); });
}

// -----------------------------------------------------------------------
// Think dispatch
// -----------------------------------------------------------------------
// Called at the end of luaUpdate for each VM. Iterates the think map for that
// side and invokes each function with (self, dt). If the object is marked
// destroy_me, the entry is removed.
static void run_thinks(sol::state &lua,
                       std::unordered_map<GameObject3D *, sol::protected_function> &thinks,
                       float dt)
{
  for (auto it = thinks.begin(); it != thinks.end();)
  {
    GameObject3D *obj = it->first;
    if (!obj || obj->destroy_me)
    {
      it = thinks.erase(it);
      continue;
    }
    sol::protected_function_result r = it->second(obj, dt);
    if (!r.valid())
    {
      sol::error err = r;
      fprintf(stderr, "LUA THINK ERROR: %s\n", err.what());
    }
    ++it;
  }
  (void)lua;
}

// -----------------------------------------------------------------------
// Per-VM global bindings (formerly sq_register_func calls)
// -----------------------------------------------------------------------
static void bind_globals(sol::state &lua, bool is_server)
{
  // ====================================================================
  // SIDE constant
  // ====================================================================
  lua["SIDE"] = is_server ? "SERVER" : "CLIENT";

  // ====================================================================
  // SERVER-ONLY bindings
  // ====================================================================
  if (is_server)
  {
    // --- network ---
    lua.set_function("start_server", []()
                     { enetserver_start(); });

    lua.set_function("sendflags_sync", [](int player_id, int flags_val)
                     {
      if (player_id < 0 || player_id >= MAX_PLAYERS || users[player_id].object_ref == nullptr)
        return;
      GameObject3D *obj = users[player_id].object_ref;
      obj->sendflags = flags_val;
      printf("Server Set Flags for Player %d: %d\n", player_id, obj->sendflags); });

    // --- instances ---
    lua.set_function("get_player_count", []()
                     { return client_count; });

    lua.set_function("instance_create", [](double x, double y, double z) -> GameObject3D *
                     { return InstanceCreate<GameObject3D>(Vector3{(float)x, (float)y, (float)z}); });

    lua.set_function("get_instance_count", []()
                     { return (int)gameobjects.size(); });

    lua.set_function("instances_get_all", [](sol::this_state ts)
                     {
      sol::state_view lv(ts);
      sol::table arr = lv.create_table();
      int i = 1;
      for (const auto &obj : gameobjects)
        arr[i++] = obj.get();
      return arr; });

    lua.set_function("instances_get", [](const std::string &target_name, sol::this_state ts)
                     {
      sol::state_view lv(ts);
      sol::table arr = lv.create_table();
      int i = 1;
      for (const auto &obj : gameobjects)
        if (obj->target_name == target_name)
          arr[i++] = obj.get();
      return arr; });

    lua.set_function("get_player_instance", [](int player_id) -> GameObject3D *
                     {
      if (player_id < 0 || player_id >= MAX_PLAYERS) return nullptr;
      return users[player_id].object_ref; });
  }

  // ====================================================================
  // CLIENT-ONLY bindings
  // ====================================================================
  if (!is_server)
  {
    // --- network ---
    lua.set_function("connect_to_server", []()
                     { return enetclient_init(); });

    // --- input: mouse buttons ---
    lua.set_function("is_mouse_button_pressed", [](int b)
                     { return IsMouseButtonPressed(b); });
    lua.set_function("is_mouse_button_down", [](int b)
                     { return IsMouseButtonDown(b); });
    lua.set_function("is_mouse_button_released", [](int b)
                     { return IsMouseButtonReleased(b); });
    lua.set_function("is_mouse_button_up", [](int b)
                     { return IsMouseButtonUp(b); });

    // --- input: mouse position / wheel / cursor ---
    lua.set_function("get_mouse_x", []()
                     { return GetMouseX(); });
    lua.set_function("get_mouse_y", []()
                     { return GetMouseY(); });
    lua.set_function("get_mouse_delta", [](sol::this_state ts)
                     {
      sol::state_view lv(ts);
      Vector2 d = GetMouseDelta();
      sol::table t = lv.create_table();
      t["x"] = d.x;
      t["y"] = d.y;
      return t; });
    lua.set_function("get_mouse_wheel_move", []()
                     { return GetMouseWheelMove(); });
    lua.set_function("set_mouse_cursor", [](int id)
                     { SetMouseCursor(id); });

    // --- input: keys ---
    lua.set_function("is_key_pressed", [](int k)
                     { return IsKeyPressed(k); });
    lua.set_function("is_key_down", [](int k)
                     { return IsKeyDown(k); });
    lua.set_function("is_key_released", [](int k)
                     { return IsKeyReleased(k); });
    lua.set_function("is_key_up", [](int k)
                     { return IsKeyUp(k); });

    // --- input: gamepad ---
    lua.set_function("is_gamepad_available", [](int i)
                     { return IsGamepadAvailable(i); });
    lua.set_function("get_gamepad_name", [](int i)
                     { return std::string(GetGamepadName(i) ? GetGamepadName(i) : ""); });
    lua.set_function("is_gamepad_button_pressed", [](int i, int b)
                     { return IsGamepadButtonPressed(i, b); });
    lua.set_function("is_gamepad_button_down", [](int i, int b)
                     { return IsGamepadButtonDown(i, b); });
    lua.set_function("is_gamepad_button_released", [](int i, int b)
                     { return IsGamepadButtonReleased(i, b); });
    lua.set_function("is_gamepad_button_up", [](int i, int b)
                     { return IsGamepadButtonUp(i, b); });
    lua.set_function("get_gamepad_button_pressed", []()
                     { return GetGamepadButtonPressed(); });
    lua.set_function("get_gamepad_axis_count", [](int i)
                     { return GetGamepadAxisCount(i); });
    lua.set_function("get_gamepad_axis_movement", [](int i, int a)
                     { return GetGamepadAxisMovement(i, a); });
    lua.set_function("set_gamepad_vibration", [](int /*i*/, double /*left*/, double /*right*/)
                     {
                       // raylib SetGamepadVibration needs a duration arg in newer versions;
                       // original code commented it out, so we keep it a no-op for parity.
                     });

    // --- cursor ---
    lua.set_function("show_cursor", []()
                     { ShowCursor(); });
    lua.set_function("hide_cursor", []()
                     { HideCursor(); });
    lua.set_function("is_cursor_hidden", []()
                     { return IsCursorHidden(); });
    lua.set_function("enable_cursor", []()
                     { EnableCursor(); });
    lua.set_function("disable_cursor", []()
                     { DisableCursor(); });
    lua.set_function("is_cursor_on_screen", []()
                     { return IsCursorOnScreen(); });

    // --- system / camera ---
    lua.set_function("get_camera_position", [](sol::this_state ts) -> sol::object
                     {
      sol::state_view lv(ts);
      if (!camera) return sol::lua_nil;
      return sol::make_object(lv, Vec3ToTable(lv, camera->position)); });

    // --- drawing ---
    lua.set_function("draw_fps", [](int x, int y)
                     {
                       global_draw_fps = true;
                       global_fps_x = x;
                       global_fps_x = y; // original code: yes, both assigned to x. Preserved.
                     });
    lua.set_function("clear_background", [](sol::table color)
                     { ClearBackground(LuaGetColor(color)); });
    lua.set_function("begin_scissor_mode", [](int x, int y, int w, int h)
                     { BeginScissorMode(x, y, w, h); });
    lua.set_function("end_scissor_mode", []()
                     { EndScissorMode(); });

    // --- window ---
    lua.set_function("toggle_fullscreen", []()
                     { ToggleFullscreen(); });
    lua.set_function("toggle_borderless_windowed", []()
                     { ToggleBorderlessWindowed(); });
    lua.set_function("maximize_window", []()
                     { MaximizeWindow(); });
    lua.set_function("minimize_window", []()
                     { MinimizeWindow(); });
    lua.set_function("restore_window", []()
                     { RestoreWindow(); });
    lua.set_function("set_window_icon", [](const std::string &path)
                     {
      Image icon = LoadImage(path.c_str());
      if (icon.data)
      {
        SetWindowIcon(icon);
        UnloadImage(icon);
      } });
    lua.set_function("set_window_title", [](const std::string &t)
                     { SetWindowTitle(t.c_str()); });
    lua.set_function("set_window_position", [](int x, int y)
                     { SetWindowPosition(x, y); });
    lua.set_function("get_screen_width", []()
                     { return GetScreenWidth(); });
    lua.set_function("get_screen_height", []()
                     { return GetScreenHeight(); });
    lua.set_function("get_render_width", []()
                     { return GetRenderWidth(); });
    lua.set_function("get_render_height", []()
                     { return GetRenderHeight(); });

    // --- rtext ---
    lua.set_function("load_font", [](const std::string &path, int size)
                     {
      if (font_primary.texture.id > 0)
        UnloadFont(font_primary);
      font_primary = LoadFontEx(path.c_str(), size, NULL, 0); });
    lua.set_function("draw_text",
                     [](const std::string &text, int x, int y, int size, sol::table color)
                     {
                       Color c = LuaGetColor(color);
                       if (font_primary.texture.id == 0)
                         DrawText(text.c_str(), x, y, size, c);
                       else
                         DrawTextEx(font_primary, text.c_str(), Vector2{(float)x, (float)y}, (float)size, 2.0f, c);
                     });

    // --- rshapes ---
    lua.set_function("draw_rectangle",
                     [](int x, int y, int w, int h, sol::table color)
                     {
                       DrawRectangle(x, y, w, h, LuaGetColor(color));
                     });

    // --- rmodels ---
    lua.set_function("draw_cube",
                     [](double x, double y, double z, double w, double h, double l, sol::table color)
                     {
                       DrawCube(Vector3{(float)x, (float)y, (float)z},
                                (float)w, (float)h, (float)l,
                                LuaGetColor(color));
                     });
  }

  // ====================================================================
  // BOTH SIDES (network, timing, rng)
  // ====================================================================

  // --- networking: send_packet_number ---
  // Client  → CPP server (over ENet)
  // Server  → target client (local or remote), or broadcast if target_id == -1
  lua.set_function("send_packet_number",
                   [&lua](int target_id, const std::string &name, double val)
                   {
                     bool from_client = (&lua == client_lua.get());
                     float fv = (float)val;
                     if (from_client)
                     {
                       Net_ToCPPServer(name, fv);
                     }
                     else
                     {
                       if (target_id == my_local_player_id)
                         Net_ToSQClient(target_id, name, fv);
                       else if (target_id >= 0 && target_id < MAX_PLAYERS && users[target_id].peer)
                         Net_ToCPPClient(users[target_id].peer, name, fv);
                       else if (target_id == -1)
                       {
                         if (my_local_player_id >= 0)
                           Net_ToSQClient(my_local_player_id, name, fv);
                         for (int i = 0; i < client_count; i++)
                           if (users[i].peer && users[i].player_id != my_local_player_id)
                             Net_ToCPPClient(users[i].peer, name, fv);
                       }
                     }
                   });

  // --- networking: send_packet_vector3 ---
  lua.set_function("send_packet_vector3",
                   [&lua](int target_id, const std::string &name, sol::table vec)
                   {
                     bool from_client = (&lua == client_lua.get());
                     Vector3 v = LuaGetVector3(vec);
                     if (from_client)
                     {
                       Net_ToCPPServer(name, v);
                     }
                     else
                     {
                       if (target_id == my_local_player_id)
                         Net_ToSQClient(target_id, name, v);
                       else if (target_id >= 0 && target_id < MAX_PLAYERS && users[target_id].peer)
                         Net_ToCPPClient(users[target_id].peer, name, v);
                       else if (target_id == -1)
                       {
                         if (my_local_player_id >= 0)
                           Net_ToSQClient(my_local_player_id, name, v);
                         for (int i = 0; i < client_count; i++)
                           if (users[i].peer && users[i].player_id != my_local_player_id)
                             Net_ToCPPClient(users[i].peer, name, v);
                       }
                     }
                   });

  // --- networking: get_packet ---
  // Pops next packet from the appropriate queue and returns
  //   { client_id=..., name=..., value=... }
  // or nil if queue empty.
  lua.set_function("get_packet", [&lua](sol::this_state ts) -> sol::object
                   {
    sol::state_view lv(ts);
    bool is_server = (&lua == server_lua.get());
    auto &queue = is_server ? client_to_server_packets : server_to_client_packets;
    int target_id = my_local_player_id;

    if (queue.empty()) return sol::lua_nil;

    for (auto it = queue.begin(); it != queue.end(); ++it)
    {
      if (is_server || it->client_id == target_id || it->client_id == -1)
      {
        Packet p = *it;
        queue.erase(it);

        sol::table packet_tbl = lv.create_table();
        packet_tbl["client_id"] = p.client_id;
        packet_tbl["name"]      = p.name;

        std::visit([&](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, int>)
            packet_tbl["value"] = arg;
          else if constexpr (std::is_same_v<T, float>)
            packet_tbl["value"] = (double)arg;
          else if constexpr (std::is_same_v<T, std::string>)
            packet_tbl["value"] = arg;
          else if constexpr (std::is_same_v<T, bool>)
            packet_tbl["value"] = arg;
          else if constexpr (std::is_same_v<T, Vector3>)
            packet_tbl["value"] = Vec3ToTable(lv, arg);
        }, p.value);

        return sol::make_object(lv, packet_tbl);
      }
    }
    return sol::lua_nil; });

  // --- timing ---
  lua.set_function("set_target_fps", [](int fps)
                   { SetTargetFPS(fps); });
  lua.set_function("get_frame_time", []()
                   { return (double)GetFrameTime(); });
  lua.set_function("get_time", []()
                   { return (double)GetTime(); });
  lua.set_function("get_fps", []()
                   { return GetFPS(); });

  // --- rng ---
  lua.set_function("set_random_seed", [](unsigned int seed)
                   { SetRandomSeed(seed); });
  lua.set_function("get_random_value", [](int min, int max)
                   { return GetRandomValue(min, max); });
}

// -----------------------------------------------------------------------
// print() override
// -----------------------------------------------------------------------
// Lua's default print writes to stdout with tabs between args and a trailing
// newline. We override per-VM to prefix "[SERVER]: " or "[CLIENT]: ".
static void install_print_override(sol::state &lua, const char *prefix)
{
  std::string pfx = prefix;
  lua.set_function("print",
                   [pfx](sol::variadic_args va, sol::this_state ts)
                   {
                     (void)ts;
                     std::string line = pfx;
                     bool first = true;
                     for (auto v : va)
                     {
                       if (!first)
                         line += "\t";
                       first = false;
                       // sol2 tostring — go through Lua's tostring metamethod for fidelity
                       sol::state_view lv(v.lua_state());
                       sol::protected_function tostring = lv["tostring"];
                       auto r = tostring(v);
                       if (r.valid())
                       {
                         std::string s = r;
                         line += s;
                       }
                       else
                       {
                         line += "?";
                       }
                     }
                     fprintf(stdout, "%s\n", line.c_str());
                     fflush(stdout);
                   });
}

// -----------------------------------------------------------------------
// VM Setup
// -----------------------------------------------------------------------
static std::unique_ptr<sol::state> luaSetupVM(bool is_server)
{
  printf("luaSetupVM(%s)\n", is_server ? "SERVER" : "CLIENT");

  auto lua = std::make_unique<sol::state>();
  // Open the standard libraries we want available in scripts.
  lua->open_libraries(sol::lib::base,
                      sol::lib::string,
                      sol::lib::math,
                      sol::lib::table,
                      sol::lib::io,
                      sol::lib::os,
                      sol::lib::package);

  install_print_override(*lua, is_server ? "[SERVER]: " : "[CLIENT]: ");

  register_enums(*lua);
  register_gameobject3d_usertype(*lua, is_server);
  bind_globals(*lua, is_server);

  return lua;
}

// -----------------------------------------------------------------------
// Script loading
// -----------------------------------------------------------------------
void luaLoadLuaFile(sol::state &lua, std::string lua_filename, VM_TYPE vm_type)
{
  printf("luaLoadLuaFile\n");

  std::string folder;
  if (vm_type == SERVER)
    folder = "scripts/server/";
  else if (vm_type == CLIENT)
    folder = "scripts/client/";
  else
    folder = "scripts/shared/";

  std::string full_path = GAMEDATA_PATH + folder + lua_filename;
  printf("DEBUG: Loading: %s\n", std::filesystem::absolute(full_path).string().c_str());

  sol::protected_function_result r = lua.safe_script_file(
      full_path, sol::script_pass_on_error);
  if (!r.valid())
  {
    sol::error err = r;
    printf("SCRIPTERROR: Execution of %s failed: %s\n",
           lua_filename.c_str(), err.what());
  }
}

void luaLoadShared(sol::state &lua)
{
  printf("luaLoadShared\n");
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
    auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos)
      continue;
    line.erase(0, first);
    auto last = line.find_last_not_of(" \t");
    line.erase(last + 1);

    if (line.empty() || line[0] == '#')
      continue;

    std::string full_path = GAMEDATA_PATH + "scripts/shared/" + line;
    sol::protected_function_result r = lua.safe_script_file(
        full_path, sol::script_pass_on_error);
    if (!r.valid())
    {
      sol::error err = r;
      printf("SCRIPTERROR: Execution of shared/%s failed: %s\n",
             line.c_str(), err.what());
    }
  }
}

void luaLoadManifest(const std::string &manifestName, VM_TYPE vm_type)
{
  printf("luaLoadManifest %s\n", manifestName.c_str());
  std::string fullPath = GAMEDATA_PATH + "scripts/" + manifestName;
  std::ifstream file(fullPath);
  if (!file.is_open())
  {
    printf("CRITICAL: Could not open script manifest at %s\n", fullPath.c_str());
    return;
  }

  sol::state *target = (vm_type == SERVER) ? server_lua.get() : client_lua.get();
  if (!target)
  {
    printf("CRITICAL: target VM not initialized for manifest %s\n", manifestName.c_str());
    return;
  }

  std::string line;
  while (std::getline(file, line))
  {
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos)
      continue;
    line.erase(0, first);
    auto last = line.find_last_not_of(" \t");
    line.erase(last + 1);

    if (line.empty() || line[0] == '#')
      continue;
    luaLoadLuaFile(*target, line, vm_type);
  }
}

// -----------------------------------------------------------------------
// RunFunc
// -----------------------------------------------------------------------
void luaRunFunc(sol::state &lua, const char *func_name, float dt)
{
  sol::object obj = lua[func_name];
  if (!obj.valid() || obj.get_type() != sol::type::function)
    return;

  sol::protected_function fn = obj;
  sol::protected_function_result r;
  if (dt < 0.0f)
    r = fn();
  else
    r = fn(dt);

  if (!r.valid())
  {
    sol::error err = r;
    fprintf(stderr, "LUA ERROR in %s: %s\n", func_name, err.what());
  }
}

// -----------------------------------------------------------------------
// BSP Entity Spawning
// -----------------------------------------------------------------------
// Calls the Lua function named `classname` (a global) with (self, origin, tags)
// where self is the fresh GameObject3D created by InstanceCreate.
//
// NOTE: see the earlier audit in this codebase — the fact that this creates
// a plain GameObject3D while SpawnBrushEntities() already creates a BrushEntity
// for the same BSP entity means the same entity ends up represented twice in
// `gameobjects`. Parity with sq.cpp is preserved here; the caller decides.
static void luaCallEntitySpawnerInVM(sol::state *lua,
                                     const std::string &classname,
                                     Vector3 origin,
                                     const std::unordered_map<std::string,
                                                              std::string> &tags,
                                     GameObject3D *obj)
{
  if (!lua)
    return;
  // printf("luaCallEntitySpawnerInVM (%s) classname=%s\n",
  //        (lua == server_lua.get()) ? "SERVER" : "CLIENT",
  //        classname.c_str());

  sol::object sp = (*lua)[classname];
  if (!sp.valid() || sp.get_type() != sol::type::function)
    return;

  // Create the engine-side object and push it into the global vector.
  obj->classname = classname;

  // Build the tags table.
  sol::table tags_tbl = lua->create_table();
  for (auto &[k, v] : tags)
    tags_tbl[k] = v;

  // Build the origin table (matches the old SQ signature).
  sol::table origin_tbl = lua->create_table();
  origin_tbl["x"] = origin.x;
  origin_tbl["y"] = origin.y;
  origin_tbl["z"] = origin.z;

  sol::protected_function fn = sp;
  sol::protected_function_result r = fn(obj, origin_tbl, tags_tbl);
  if (!r.valid())
  {
    sol::error err = r;
    fprintf(stderr, "LUA SPAWNER ERROR (%s): %s\n", classname.c_str(), err.what());
  }
}

void luaCallEntitySpawner(const std::string &classname,
                          Vector3 origin,
                          const std::unordered_map<std::string, std::string> &tags,
                          GameObject3D *obj)
{
  luaCallEntitySpawnerInVM(server_lua.get(), classname, origin, tags, obj);
  luaCallEntitySpawnerInVM(client_lua.get(), classname, origin, tags, obj);
}

inline int brush_sync_id = 1000;
void luaSpawnBSPEntities()
{
  brush_sync_id = 1000;
  for (auto &obj_ptr : gameobjects)
  {
    // Try to cast the base pointer to a BrushEntity pointer
    BrushEntity *brush_obj = dynamic_cast<BrushEntity *>(obj_ptr.get());

    // If it's not a BrushEntity (or has no classname), skip it
    if (!brush_obj || brush_obj->classname.empty())
      continue;

    // Now we can access brush_obj->tags because we've casted it
    brush_obj->client_id = brush_sync_id++;
    luaCallEntitySpawner(brush_obj->classname, brush_obj->position, brush_obj->tags, brush_obj);
  }
}

// -----------------------------------------------------------------------
// Public entry points
// -----------------------------------------------------------------------
void luaStartServer()
{
  if (server_lua)
    return;
  printf("luaStartServer\n");
  server_lua = luaSetupVM(true);
  luaLoadShared(*server_lua);
  luaLoadManifest("sv_manifest.cfg", SERVER);
  luaRunFunc(*server_lua, "init");
}

void luaStartClient()
{
  if (client_lua)
    return;
  printf("luaStartClient\n");
  client_lua = luaSetupVM(false);
  luaLoadShared(*client_lua);
  luaLoadManifest("cs_manifest.cfg", CLIENT);
  // init() is deferred until the server assigns our player ID (see luaUpdate).
}

void luaUpdate(float dt)
{
  static bool spawned = false;
  if (!spawned)
  {
    spawned = true;
    luaSpawnBSPEntities();
    printf("Spawned all ents!\n");
  }

  if (server_online && global_is_hosting)
    enetserver_update();

  enetclient_update();

  // Defer client init() until server has assigned our player ID.
  static bool lua_client_init_done = false;
  if (!lua_client_init_done && global_client_init_called && client_lua)
  {
    luaRunFunc(*client_lua, "init");
    lua_client_init_done = true;
  }

  if (global_is_hosting && server_lua)
  {
    luaRunFunc(*server_lua, "update", dt);
    run_thinks(*server_lua, g_server_thinks, dt);
  }

  if (client_lua)
  {
    luaRunFunc(*client_lua, "update", dt);
    run_thinks(*client_lua, g_client_thinks, dt);

    for (auto &obj : gameobjects)
    {
      // make sure this isnt a player object
      if (obj->client_id < 0)
      {
      }
    }
  }
}

void luaDraw(float dt)
{
  if (client_lua)
    luaRunFunc(*client_lua, "draw", dt);
}

void luaDrawGUI(float dt)
{
  if (client_lua)
    luaRunFunc(*client_lua, "draw_gui", dt);
}
