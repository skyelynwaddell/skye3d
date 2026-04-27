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

  // Identical letterbox calculation to draw.cpp — GUI stretches to fill
  // the same rect as the 3D scene so they always scale together.
  float scale = fminf((float)sw / RENDER_WIDTH, (float)sh / RENDER_HEIGHT);
  float destw = RENDER_WIDTH * scale;
  float desth = RENDER_HEIGHT * scale;
  float destx = (sw - destw) / 2.0f;
  float desty = (sh - desth) / 2.0f;

  if (global_guiscale <= 0.001f)
    global_guiscale = 1.0f;

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
    // Map window mouse coords → logical GUI space:
    //   1. Subtract destx/y so coords are relative to the game viewport.
    //   2. Multiply by (FBO size / dest size) to reach FBO pixel space.
    //   3. Divide by guiscale to reach pre-scale logical GUI coords.
    float scaleX = (float)pp_gui_fbo.texture.width / (destw * global_guiscale);
    float scaleY = (float)pp_gui_fbo.texture.height / (desth * global_guiscale);
    SetMouseOffset(-destx, -desty);
    SetMouseScale(scaleX, scaleY);
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

  // Stretch the GUI FBO to the game viewport — identical to how draw.cpp
  // blits the scene, so GUI and game always occupy exactly the same area.
  BeginBlendMode(BLEND_ALPHA);
  DrawTexturePro(pp_gui_fbo.texture,
                 {0, 0, (float)RENDER_WIDTH, (float)-RENDER_HEIGHT},
                 {destx, desty, destw, desth},
                 {0, 0}, 0.0f, WHITE);
  EndBlendMode();

  EndDrawing();
}