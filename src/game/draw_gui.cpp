#include "engine.h"
#include <camera3d.h>
#include <gameobject3d.h>
#include <bsp.h>
#include <lua.hpp>

void DrawGUI()
{
  float GUI_WIDTH = 1280;
  float GUI_HEIGHT = 720;
  float guiscale = 2.0f;

  float screenscale = fminf((float)GetScreenWidth() / GUI_WIDTH,
                            (float)GetScreenHeight() / GUI_HEIGHT);
  float destw = GUI_WIDTH * screenscale;
  float desth = GUI_HEIGHT * screenscale;
  float destx = (GetScreenWidth() - destw) / 2.0f;
  float desty = (GetScreenHeight() - desth) / 2.0f;

  BeginTextureMode(pp_gui_fbo);
  ClearBackground(BLANK);

  rlPushMatrix();
  rlScalef(guiscale, guiscale, 1.0f);

  luaDrawGUI(GetFrameTime());

  if (global_draw_fps)
    DrawFPS(global_fps_x, global_fps_y);

  BSP_DrawDebug(camera->position);
  rlPopMatrix();
  EndTextureMode();

  BeginBlendMode(BLEND_ALPHA);
  DrawTexturePro(pp_gui_fbo.texture,
                 {0, 0, (float)pp_gui_fbo.texture.width, (float)-pp_gui_fbo.texture.height},
                 {destx, desty, destw, desth}, // Standard size
                 {0, 0}, 0.0f, WHITE);
  EndBlendMode();

  EndDrawing();
}