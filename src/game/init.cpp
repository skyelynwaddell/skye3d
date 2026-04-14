#include "engine.h"
#include <camera3d.h>
#include <sq.h>
#include <cfg_parser.h>

void Init()
{
  CFG_LoadSettings();

  SetTargetFPS(60);
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1200, 800, global_window_title.c_str());
  SetWindowState(FLAG_WINDOW_MAXIMIZED);
  rlEnableBackfaceCulling();
  rlImGuiSetup(false);

  if (global_is_hosting)
    sqStartServer();

  sqStartClient();
  try
  {
    models = LoadModelsFromBSPFile(global_map_to_load);
  }
  catch (...)
  {
    global_map_to_load = "";
  }

  DisableCursor(); // Limit cursor to relative movement inside the window

  Camera3D_Init();
};