#include "engine.h"
#include <camera3d.h>
#include <lua.hpp>
#include <gameobject3d.h>
#include <bsp.h>

// Track window size for FBO recreation.
// Start at -1 so the handler fires on the very first frame and initialises
// the GUI FBO at the correct viewport size before DrawGUI runs.
static int lastscreenw = -1;
static int lastscreenh = -1;

void Draw()
{
  if (!camera)
    return;

  luaDraw(GetFrameTime());

  BeginDrawing();
  ClearBackground(BLACK);

  static bool enable_wireframe = false;
  int curw = GetScreenWidth();
  int curh = GetScreenHeight();
  if (curw != lastscreenw || curh != lastscreenh)
  {
    PostProcess_DestroyFBOs();
    PostProcess_CreateFBOs(RENDER_WIDTH, RENDER_HEIGHT);

    // GUI FBO tracks the game viewport (letterbox) size so it can be blitted
    // at 1:1 — no scaling, no blur, can't reach the black bars.
    float s = fminf((float)curw / RENDER_WIDTH, (float)curh / RENDER_HEIGHT);
    int vpw = (int)(RENDER_WIDTH  * s);
    int vph = (int)(RENDER_HEIGHT * s);
    if (pp_gui_fbo.id != 0)
      UnloadRenderTexture(pp_gui_fbo);
    pp_gui_fbo = LoadRenderTexture(vpw, vph);
    SetTextureFilter(pp_gui_fbo.texture, TEXTURE_FILTER_POINT);

    // Keep GUI_WIDTH/HEIGHT in sync with the viewport so SkyeUI's drag
    // clamping (LogW/LogH) uses the real available space, not the config value.
    GUI_WIDTH  = (float)vpw;
    GUI_HEIGHT = (float)vph;

    lastscreenw = curw;
    lastscreenh = curh;
  }

  BSP_UpdateFlashlight(camera.get(), default_shader, characterShader, liquid_shader);

  // ── Pass 1 Render scene ──────────────────────────
  BeginTextureMode(pp_scene_fbo);
  ClearBackground(GRAY);
  BeginMode3D(*camera);
  BSP_Draw(shader, enable_wireframe, camera->position);

  CharacterShader_Update();
  GameObject3D_DrawAll();

  GameObject3D_DrawAllDebug();

  EndMode3D();
  EndTextureMode();

  // ── Pass 2 bloom a ──
  BeginTextureMode(pp_bloom_fbo_a);
  ClearBackground(BLACK);
  BeginShaderMode(pp_bloom_extract);
  DrawTextureRec(pp_scene_fbo.texture,
                 {0, 0, (float)pp_scene_fbo.texture.width, (float)-pp_scene_fbo.texture.height},
                 {0, 0}, WHITE);
  EndShaderMode();
  EndTextureMode();

  // ── Pass 3 bloom horizontal blur ────────────────────────────────
  BeginTextureMode(pp_bloom_fbo_b);
  ClearBackground(BLACK);
  BeginShaderMode(pp_blur);
  float dirH[2] = {1.0f / pp_bloom_fbo_a.texture.width, 0.0f};
  SetShaderValue(pp_blur, GetShaderLocation(pp_blur, "direction"), dirH, SHADER_UNIFORM_VEC2);
  DrawTextureRec(pp_bloom_fbo_a.texture,
                 {0, 0, (float)pp_bloom_fbo_a.texture.width, (float)-pp_bloom_fbo_a.texture.height},
                 {0, 0}, WHITE);
  EndShaderMode();
  EndTextureMode();

  // ── Pass 4 bloom vertical blur ─────────────────────────────────
  BeginTextureMode(pp_bloom_fbo_a);
  ClearBackground(BLACK);
  BeginShaderMode(pp_blur);
  float dirV[2] = {0.0f, 1.0f / pp_bloom_fbo_b.texture.height};
  SetShaderValue(pp_blur, GetShaderLocation(pp_blur, "direction"), dirV, SHADER_UNIFORM_VEC2);
  DrawTextureRec(pp_bloom_fbo_b.texture,
                 {0, 0, (float)pp_bloom_fbo_b.texture.width, (float)-pp_bloom_fbo_b.texture.height},
                 {0, 0}, WHITE);
  EndShaderMode();
  EndTextureMode();

  // ── Pass 5+6 moaar blur ─────
  BeginTextureMode(pp_bloom_fbo_b);
  ClearBackground(BLACK);
  BeginShaderMode(pp_blur);
  float dirH2[2] = {2.0f / pp_bloom_fbo_a.texture.width, 0.0f};
  SetShaderValue(pp_blur, GetShaderLocation(pp_blur, "direction"), dirH2, SHADER_UNIFORM_VEC2);
  DrawTextureRec(pp_bloom_fbo_a.texture,
                 {0, 0, (float)pp_bloom_fbo_a.texture.width, (float)-pp_bloom_fbo_a.texture.height},
                 {0, 0}, WHITE);
  EndShaderMode();
  EndTextureMode();

  BeginTextureMode(pp_bloom_fbo_a);
  ClearBackground(BLACK);
  BeginShaderMode(pp_blur);
  float dirV2[2] = {0.0f, 2.0f / pp_bloom_fbo_b.texture.height};
  SetShaderValue(pp_blur, GetShaderLocation(pp_blur, "direction"), dirV2, SHADER_UNIFORM_VEC2);
  DrawTextureRec(pp_bloom_fbo_b.texture,
                 {0, 0, (float)pp_bloom_fbo_b.texture.width, (float)-pp_bloom_fbo_b.texture.height},
                 {0, 0}, WHITE);
  EndShaderMode();
  EndTextureMode();

  // Pass 7 - Combined Render Texture Aspect Ratio Scaled
  ClearBackground(BLACK);

  BeginShaderMode(pp_composite);
  rlActiveTextureSlot(1);
  rlEnableTexture(pp_bloom_fbo_a.texture.id);

  // Calculate scaling to fit the FBO into the current window
  float scale = fminf((float)GetScreenWidth() / RENDER_WIDTH,
                      (float)GetScreenHeight() / RENDER_HEIGHT);

  float destw = RENDER_WIDTH * scale;
  float desth = RENDER_HEIGHT * scale;
  float destx = (GetScreenWidth() - destw) / 2.0f;
  float desty = (GetScreenHeight() - desth) / 2.0f;

  // Draw the scene FBO scaled and centered
  DrawTexturePro(pp_scene_fbo.texture,
                 {0, 0, (float)RENDER_WIDTH, (float)-RENDER_HEIGHT},
                 {destx, desty, destw, desth},
                 {0, 0}, 0.0f, WHITE);

  rlActiveTextureSlot(1);
  rlDisableTexture();
  rlActiveTextureSlot(0);
  EndShaderMode();
};
