#include "engine.h"
#include <camera3d.h>
#include <cfg_parser.h>
#include <bsp.h>
#include <rlImGui.h>
#include <gameobjects/brush_entity.h>
#include <lua.hpp>
#include <skyeui.h>

void Init()
{
  CFG_LoadSettings();

  SetTargetFPS(60);
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1200, 800, global_window_title.c_str());
  SetWindowState(FLAG_WINDOW_MAXIMIZED);
  rlEnableBackfaceCulling();
  rlImGuiSetup(false);
  CharacterShader_Init();
  SkyeUI_Init();

  SetExitKey(KEY_NULL);

  if (global_is_hosting)
    luaStartServer();

  luaStartClient();
  try
  {
    bsp_renderer.texture_filter = global_texture_filter;
    models = LoadModelsFromBSPFile(global_map_to_load);
    SpawnBrushEntities();
  }
  catch (...)
  {
    global_map_to_load = "";
  }

  DisableCursor();
  Camera3D_Init();
};