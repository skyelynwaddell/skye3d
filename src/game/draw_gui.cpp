#include "engine.h"
#include <camera3d.h>
#include <gameobject3d.h>
#include <bsp.h>
#include <lua.hpp>
#include <hud.h>

void DrawGUI()
{
  float screenscale = fminf((float)GetScreenWidth() / GUI_WIDTH,
                            (float)GetScreenHeight() / GUI_HEIGHT);
  float destw = floorf(GUI_WIDTH * screenscale);
  float desth = floorf(GUI_HEIGHT * screenscale);
  float destx = floorf((GetScreenWidth() - destw) / 2.0f);
  float desty = floorf((GetScreenHeight() - desth) / 2.0f);

  BeginTextureMode(pp_gui_fbo);
  ClearBackground(BLANK);
  rlPushMatrix();
  rlScalef(global_guiscale, global_guiscale, 1.0f);

  if (IsCursorHidden())
  {
    SetMouseOffset((float)-GetScreenWidth() * 10, (float)-GetScreenHeight() * 10);
    SetMouseScale(1.0f, 1.0f);
  }
  else
  {
    float scaleX = (float)pp_gui_fbo.texture.width / (destw * global_guiscale);
    float scaleY = (float)pp_gui_fbo.texture.height / (desth * global_guiscale);
    SetMouseOffset(-destx, -desty);
    SetMouseScale(scaleX, scaleY);
  }

  /// DRAW ---------------------------

  SkyeUI_BeginFrame();
  luaDrawGUI(GetFrameTime());
  HUD_DrawGUI();

  if (global_draw_fps)
    DrawFPS(global_fps_x, global_fps_y);

  // BSP_DrawDebug(camera->position);

  /// END DRAW ---------------------------

  SetMouseOffset(0.0f, 0.0f);
  SetMouseScale(1.0f, 1.0f);
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