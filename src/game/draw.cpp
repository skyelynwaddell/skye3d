#include "engine.h"
#include <camera3d.h>
#include <lua.hpp>
#include <gameobject3d.h>
#include <bsp.h>

// Track window size for FBO recreation
static int lastscreenw = 0;
static int lastscreenh = 0;

void Draw()
{
  if (!camera)
    return;

  luaDraw(GetFrameTime());

  BeginDrawing();
  ClearBackground(BLACK);

  static bool enable_wireframe = false;
  if (SCREEN_WIDTH != lastscreenw || SCREEN_HEIGHT != lastscreenh)
  {
    PostProcess_DestroyFBOs();
    PostProcess_CreateFBOs(RENDER_WIDTH, RENDER_HEIGHT);
    lastscreenw = SCREEN_WIDTH;
    lastscreenh = SCREEN_HEIGHT;
  }

  BSP_UpdateFlashlight(camera.get(), default_shader, characterShader, liquid_shader);

  // ── Pass 1 Render scene ──────────────────────────
  BeginTextureMode(pp_scene_fbo);
  ClearBackground(GRAY);
  BeginMode3D(*camera);
  BSP_Draw(shader, enable_wireframe, camera->position);

  CharacterShader_Update();
  GameObject3D_DrawAll();

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

  rlDisableBackfaceCulling();
  rlEnableDepthTest();
  rlSetBlendMode(BLEND_ALPHA);

  rlSetTexture(0);
  rlActiveTextureSlot(0);

  BeginMode3D(*camera);
  rlSetLineWidth(10.0f);
  BeginShaderMode(debug_shader);
  GameObject3D_DrawAllDebug();
  EndShaderMode();
  EndMode3D();
};
