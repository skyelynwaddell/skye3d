#include "engine.h"
#include <camera3d.h>
#include <sq.h>
#include <gameobject3d.h>

void Update()
{
  sqUpdate(GetFrameTime());
  Camera3D_Update();
  GameObject3D_UpdateAll();

  // static bool enable_imgui = true;
  // if (IsKeyPressed(KEY_I))
  //   enable_imgui = !enable_imgui;
};