#include "engine.h"
#include <camera3d.h>
#include <lua.hpp>
#include <gameobject3d.h>

void Update()
{
  luaUpdate(GetFrameTime());
  Camera3D_Update();
  GameObject3D_UpdateAll();

  // static bool enable_imgui = true;
  // if (IsKeyPressed(KEY_I))
  //   enable_imgui = !enable_imgui;
};