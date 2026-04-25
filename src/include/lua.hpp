#pragma once
#ifndef SOL_LUA_VERSION
#define SOL_LUA_VERSION 504
#endif

#include <sol/sol.hpp>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "fonts.h"
#include "gameobject3d.h"

// -----------------------------------------------------------------------
// Global VM states
// -----------------------------------------------------------------------
inline std::unique_ptr<sol::state> client_lua;
inline std::unique_ptr<sol::state> server_lua;

enum VM_TYPE
{
  CLIENT,
  SERVER,
  SHARED
};

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

// BSP entity spawning
void luaCallEntitySpawner(const std::string &classname,
                          Vector3 origin,
                          const std::unordered_map<std::string, std::string> &tags,
                          GameObject3D *obj);
void luaSpawnBrushEntities();
void luaSpawnEntities();

// Run a named global function in a VM. If dt >= 0, it is passed as the arg.
// If dt == -1.0f (default), the function is called with no arguments.
void luaRunFunc(sol::state &lua, const char *func_name, float dt = -1.0f);

// Script loading
void luaLoadLuaFile(sol::state &lua, std::string lua_filename, VM_TYPE vm_type);
void luaLoadShared(sol::state &lua);
void luaLoadManifest(const std::string &manifestName, VM_TYPE vm_type);

// Lifecycle
void luaStartServer();
void luaStartClient();
void luaUpdate(float dt);
void luaDraw(float dt);
void luaDrawGUI(float dt);

// register a "think" function (called per-frame while object lives)
// for a particular GameObject3D in a given VM. Exposed so engine code can
// clear them on object destruction.
class GameObject3D;
void luaClearThinks(GameObject3D *obj);
