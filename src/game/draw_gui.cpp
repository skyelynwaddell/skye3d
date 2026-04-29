#include "engine.h"
#include <camera3d.h>
#include <gameobject3d.h>
#include <bsp.h>
#include <lua.hpp>
#include <skyeui.h>

void DrawGUI()
{
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  if (global_guiscale <= 0.001f)
    global_guiscale = 1.0f;

  // Game viewport rect — same letterbox formula as draw.cpp.
  float scale = fminf((float)sw / RENDER_WIDTH, (float)sh / RENDER_HEIGHT);
  int vpw = (int)(RENDER_WIDTH  * scale);
  int vph = (int)(RENDER_HEIGHT * scale);
  float destx = (sw - vpw) / 2.0f;
  float desty = (sh - vph) / 2.0f;

  // pp_gui_fbo is recreated by Draw() whenever the window changes size,
  // so it always matches the current game viewport exactly.
  BeginTextureMode(pp_gui_fbo);
  ClearBackground(BLANK);
  rlPushMatrix();
  rlScalef(global_guiscale, global_guiscale, 1.0f);

  if (IsCursorHidden())
  {
    SetMouseOffset(-99999, -99999);
  }
  else
  {
    // Map window mouse → FBO space:
    //   subtract viewport origin so coords are relative to the game area,
    //   then divide by guiscale to reach the pre-scale logical GUI coords.
    SetMouseOffset(-destx, -desty);
    SetMouseScale(1.0f / global_guiscale, 1.0f / global_guiscale);
  }

  /// DRAW ---------------------------

  SkyeUI_BeginFrame();
  luaDrawGUI(GetFrameTime());

  if (global_draw_fps)
    DrawFPS(global_fps_x, global_fps_y);

  /// END DRAW ---------------------------

  SetMouseOffset(0.0f, 0.0f);
  SetMouseScale(1.0f, 1.0f);
  rlPopMatrix();
  EndTextureMode();

  // Blit GUI at 1:1, positioned at the game viewport origin — no scaling, no blur.
  BeginBlendMode(BLEND_ALPHA);
  DrawTextureRec(pp_gui_fbo.texture,
                 {0, 0, (float)vpw, (float)-vph},
                 {destx, desty}, WHITE);
  EndBlendMode();

  EndDrawing();
}