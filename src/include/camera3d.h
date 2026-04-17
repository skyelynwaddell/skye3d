#pragma once
#include "engine.h"
#include <raylib.h>
#include <raymath.h>
#include <rlights.h>
#include <algorithm>
#include <memory>
#include "rlgl.h"

inline std::unique_ptr<Camera> camera;
inline long shader_mod_time;
inline Shader shader;
inline Light camera_light;
inline int light_power = 30;

// Post-processing
inline Shader pp_bloom_extract;
inline Shader pp_blur;
inline Shader pp_composite;
inline RenderTexture2D pp_scene_fbo;   // full-res scene render
inline RenderTexture2D pp_bloom_fbo_a; // half-res bloom ping
inline RenderTexture2D pp_bloom_fbo_b; // half-res bloom pong

// Post-process tunspables
inline float pp_bloom_threshold = 0.55f;
inline float pp_bloom_knee = 0.3f;
inline float pp_bloom_intensity = 0.8f;
inline float pp_exposure = 0.6f;
inline float pp_saturation = 1.2f;
inline float pp_warmth = 0.15f;
inline float pp_vignette_strength = 0.3f;

static bool HandleMouseCursorActive()
{
  static bool enable_cursor = false;
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
  {
    if (enable_cursor = !enable_cursor)
      EnableCursor();
    else
      DisableCursor();
  }
  return enable_cursor;
};

static void Camera3D_UpdateShaders()
{
  if (!camera)
    return;

  // Check if shader file has been modified
  long current_shader_mod_time = std::max(GetFileModTime(VS_PATH), GetFileModTime(FS_PATH));
  if (current_shader_mod_time != shader_mod_time)
  {
    // Try hot-reloading updated shader
    Shader updated_shader = LoadShader(VS_PATH, FS_PATH);
    if (updated_shader.id != rlGetShaderIdDefault()) // It was correctly loaded
    {
      UnloadShader(shader);
      shader = updated_shader;
      shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

      LIGHT_COUNT = 0;
      camera_light = CreateLight(LIGHT_POINT, camera->position, {}, WHITE, shader);
      SetShaderValue(shader, GetShaderLocation(shader, "lightPower"), &light_power, SHADER_UNIFORM_INT);
    }

    shader_mod_time = current_shader_mod_time;
  }
};

inline void Camera3D_Move(Camera &camera, bool enabled)
{
  if (!enabled)
    return;

  float speed = 10.0f * GetFrameTime();

  Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
  Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

  if (IsKeyDown(KEY_W))
    camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
  if (IsKeyDown(KEY_S))
    camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
  if (IsKeyDown(KEY_D))
    camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));
  if (IsKeyDown(KEY_A))
    camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
  if (IsKeyDown(KEY_SPACE))
    camera.position.y += speed;
  if (IsKeyDown(KEY_LEFT_CONTROL))
    camera.position.y -= speed;

  Vector2 mouse_delta = GetMouseDelta();
  float sensitivity = 0.003f;

  static float yaw = 0.0f;
  static float pitch = 0.0f;

  yaw += mouse_delta.x * sensitivity;
  pitch -= mouse_delta.y * sensitivity;

  pitch = Clamp(pitch, -1.5f, 1.5f);

  Vector3 direction = {
      cosf(pitch) * sinf(yaw),
      sinf(pitch),
      -cosf(pitch) * cosf(yaw)};

  camera.target = Vector3Add(camera.position, direction);
};

inline void PostProcess_CreateFBOs(int w, int h)
{
  pp_scene_fbo = LoadRenderTexture(w, h);
  pp_bloom_fbo_a = LoadRenderTexture(w / 2, h / 2);
  pp_bloom_fbo_b = LoadRenderTexture(w / 2, h / 2);
}

inline void PostProcess_DestroyFBOs()
{
  UnloadRenderTexture(pp_scene_fbo);
  UnloadRenderTexture(pp_bloom_fbo_a);
  UnloadRenderTexture(pp_bloom_fbo_b);
}

inline void PostProcess_SetUniforms()
{
  // bloom
  SetShaderValue(pp_bloom_extract, GetShaderLocation(pp_bloom_extract, "bloomThreshold"), &pp_bloom_threshold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pp_bloom_extract, GetShaderLocation(pp_bloom_extract, "bloomKnee"), &pp_bloom_knee, SHADER_UNIFORM_FLOAT);

  // composite
  SetShaderValue(pp_composite, GetShaderLocation(pp_composite, "bloomIntensity"), &pp_bloom_intensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pp_composite, GetShaderLocation(pp_composite, "exposure"), &pp_exposure, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pp_composite, GetShaderLocation(pp_composite, "saturation"), &pp_saturation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pp_composite, GetShaderLocation(pp_composite, "warmth"), &pp_warmth, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pp_composite, GetShaderLocation(pp_composite, "vignetteStrength"), &pp_vignette_strength, SHADER_UNIFORM_FLOAT);

  // bloom = texture slot 1
  int tex_1_loc = GetShaderLocation(pp_composite, "texture1");
  int tex_1_val = 1;
  SetShaderValue(pp_composite, tex_1_loc, &tex_1_val, SHADER_UNIFORM_INT);
}

inline void PostProcess_Init()
{
  pp_bloom_extract = LoadShader(PP_VS_PATH, BLOOM_EXTRACT_FS_PATH);
  pp_blur = LoadShader(PP_VS_PATH, BLUR_FS_PATH);
  pp_composite = LoadShader(PP_VS_PATH, COMPOSITE_FS_PATH);

  PostProcess_CreateFBOs(SCREEN_WIDTH, SCREEN_HEIGHT);
  PostProcess_SetUniforms();
}

inline void Camera3D_Init()
{
  shader_mod_time = std::max(GetFileModTime(VS_PATH), GetFileModTime(FS_PATH));
  shader = LoadShader(VS_PATH, FS_PATH);
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  camera = std::make_unique<Camera>(Camera{
      .position = {10.0f, 10.0f, 10.0f},
      .target = {0.0f, 0.0f, 0.0f},
      .up = {0.0f, 1.0f, 0.0f},
      .fovy = 90.0f,
      .projection = CAMERA_PERSPECTIVE});

  camera_light = CreateLight(LIGHT_POINT, camera->position, {}, WHITE, shader);
  SetShaderValue(shader, GetShaderLocation(shader, "lightPower"), &light_power, SHADER_UNIFORM_INT);

  PostProcess_Init();
};

inline void Camera3D_Update()
{
  if (!camera)
    return;

  Camera3D_UpdateShaders();
  camera_light.position = camera->position;
};