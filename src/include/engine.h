#pragma once
#include "raylib.h"
#include "global.h"
inline int LIGHT_COUNT = 4;
inline int my_local_player_id = -1;

inline int SCREEN_WIDTH = 1920;
inline int SCREEN_HEIGHT = 1080;
inline int RENDER_WIDTH = 1920;
inline int RENDER_HEIGHT = 1080;
inline float GUI_WIDTH = 1920;
inline float GUI_HEIGHT = 1080;

void Init();
void Update();
void Draw();
void DrawGUI();
void CleanUp();

inline void HandleWindowMode()
{
  switch (global_window_mode)
  {
  case 1:
    if (!IsWindowState(FLAG_FULLSCREEN_MODE))
      ToggleFullscreen();
    break;
  case 2:
    if (IsWindowState(FLAG_FULLSCREEN_MODE))
      ToggleFullscreen();
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize(GetMonitorWidth(GetCurrentMonitor()),
                  GetMonitorHeight(GetCurrentMonitor()));
    SetWindowPosition(0, 0);
    break;
  default: // 0 = windowed
    if (IsWindowState(FLAG_FULLSCREEN_MODE))
      ToggleFullscreen();
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    break;
  }
};