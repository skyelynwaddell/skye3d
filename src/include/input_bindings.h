#pragma once
#include <string>
#include <unordered_map>
#include <raylib.h>

// ============================================================
//  Engine Input Binding System
//
//  C++ side of the Lua rebinding system.  Each entry mirrors
//  an entry in the Lua BINDINGS table and is kept in sync by
//  set_engine_binding() / set_engine_binding_alt() Lua functions.
//
//  Usage in engine code:
//    if (EngineInputPressed("jump"))       { ... }
//    if (EngineInputDown("up"))            { ... }
//    int k    = EngineInputCode("right");  // primary keycode for stack comparisons
//    int kalt = EngineInputCodeAlt("right"); // alternate keycode
// ============================================================

enum EngineBindType { EBIND_KEY = 0, EBIND_MOUSE = 1 };

// Virtual mouse-wheel codes (match MOUSE_BUTTON.WHEEL_UP/DOWN in lua.cpp)
constexpr int EBIND_WHEEL_UP   = 10;
constexpr int EBIND_WHEEL_DOWN = 11;

struct EngineBinding
{
  EngineBindType type     = EBIND_KEY;
  int            code     = -1;  // -1 = unbound
  EngineBindType alt_type = EBIND_KEY;
  int            alt_code = -1;  // -1 = no alternate
};

// Global binding table.
// Defaults match sh_input.lua's BINDINGS_DEFAULT so the game works
// correctly before Lua has a chance to call set_engine_binding().
inline std::unordered_map<std::string, EngineBinding> g_engine_bindings =
{
  { "up",         { EBIND_KEY,   KEY_W,              EBIND_KEY, KEY_UP    } },
  { "down",       { EBIND_KEY,   KEY_S,              EBIND_KEY, KEY_DOWN  } },
  { "left",       { EBIND_KEY,   KEY_A,              EBIND_KEY, KEY_LEFT  } },
  { "right",      { EBIND_KEY,   KEY_D,              EBIND_KEY, KEY_RIGHT } },
  { "jump",       { EBIND_KEY,   KEY_SPACE,          EBIND_KEY, -1        } },
  { "reload",     { EBIND_KEY,   KEY_R,              EBIND_KEY, -1        } },
  { "interact",   { EBIND_KEY,   KEY_E,              EBIND_KEY, -1        } },
  { "shoot",      { EBIND_MOUSE, MOUSE_BUTTON_LEFT,  EBIND_KEY, -1        } },
  { "flashlight", { EBIND_KEY,   KEY_F,              EBIND_KEY, -1        } },
};

// ---- Internal single-binding check helpers -------------------

static inline bool _ebind_pressed(EngineBindType type, int code)
{
  if (code < 0) return false;
  if (type == EBIND_MOUSE)
  {
    if (code == EBIND_WHEEL_UP)   return GetMouseWheelMove() > 0.0f;
    if (code == EBIND_WHEEL_DOWN) return GetMouseWheelMove() < 0.0f;
    return IsMouseButtonPressed(code);
  }
  if (code == 0) return false;  // KEY_NULL
  return IsKeyPressed(code);
}

static inline bool _ebind_down(EngineBindType type, int code)
{
  if (code < 0) return false;
  if (type == EBIND_MOUSE)
  {
    if (code == EBIND_WHEEL_UP)   return GetMouseWheelMove() > 0.0f;
    if (code == EBIND_WHEEL_DOWN) return GetMouseWheelMove() < 0.0f;
    return IsMouseButtonDown(code);
  }
  if (code == 0) return false;
  return IsKeyDown(code);
}

static inline bool _ebind_released(EngineBindType type, int code)
{
  if (code < 0) return false;
  if (type == EBIND_MOUSE)
  {
    if (code == EBIND_WHEEL_UP)   return GetMouseWheelMove() == 0.0f;
    if (code == EBIND_WHEEL_DOWN) return GetMouseWheelMove() == 0.0f;
    return IsMouseButtonReleased(code);
  }
  if (code == 0) return false;
  return IsKeyReleased(code);
}

// ---- Public query helpers -------------------------------------

// Primary keycode for named binding (keyboard only).
// Returns fallback if the binding is a mouse button or is unset.
inline int EngineInputCode(const std::string &name, int fallback = 0)
{
  auto it = g_engine_bindings.find(name);
  if (it == g_engine_bindings.end()) return fallback;
  if (it->second.type == EBIND_MOUSE) return fallback;
  return (it->second.code > 0) ? it->second.code : fallback;
}

// Alternate keycode for named binding (keyboard only).
// Returns fallback if the alt binding is a mouse button or is unset.
inline int EngineInputCodeAlt(const std::string &name, int fallback = -1)
{
  auto it = g_engine_bindings.find(name);
  if (it == g_engine_bindings.end()) return fallback;
  if (it->second.alt_type == EBIND_MOUSE) return fallback;
  return (it->second.alt_code > 0) ? it->second.alt_code : fallback;
}

// Pressed — true if primary OR alternate is pressed this frame.
inline bool EngineInputPressed(const std::string &name)
{
  auto it = g_engine_bindings.find(name);
  if (it == g_engine_bindings.end()) return false;
  const auto &b = it->second;
  return _ebind_pressed(b.type, b.code) || _ebind_pressed(b.alt_type, b.alt_code);
}

// Down — true if primary OR alternate is held this frame.
inline bool EngineInputDown(const std::string &name)
{
  auto it = g_engine_bindings.find(name);
  if (it == g_engine_bindings.end()) return false;
  const auto &b = it->second;
  return _ebind_down(b.type, b.code) || _ebind_down(b.alt_type, b.alt_code);
}

// Released — true if primary OR alternate was released this frame.
inline bool EngineInputReleased(const std::string &name)
{
  auto it = g_engine_bindings.find(name);
  if (it == g_engine_bindings.end()) return false;
  const auto &b = it->second;
  return _ebind_released(b.type, b.code) || _ebind_released(b.alt_type, b.alt_code);
}
