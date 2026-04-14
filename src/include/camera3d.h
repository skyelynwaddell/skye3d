#pragma once
#include "engine.h"
#include <raylib.h>
#include <raymath.h>
#include <rlights.h>
#include <algorithm>

inline std::unique_ptr<Camera> camera;
inline long shaderModTime;
inline Shader shader;
inline Light cameraLight;
inline int lightPower = 10;

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
  long currentShaderModTime = std::max(GetFileModTime(VS_PATH), GetFileModTime(FS_PATH));
  if (currentShaderModTime != shaderModTime)
  {
    // Try hot-reloading updated shader
    Shader updatedShader = LoadShader(VS_PATH, FS_PATH);
    if (updatedShader.id != rlGetShaderIdDefault()) // It was correctly loaded
    {
      UnloadShader(shader);
      shader = updatedShader;
      shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

      LIGHT_COUNT = 0;
      cameraLight = CreateLight(LIGHT_POINT, camera->position, {}, WHITE, shader);
      SetShaderValue(shader, GetShaderLocation(shader, "lightPower"), &lightPower, SHADER_UNIFORM_INT);
    }

    shaderModTime = currentShaderModTime;
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

  Vector2 mouseDelta = GetMouseDelta();
  float sensitivity = 0.003f;

  static float yaw = 0.0f;
  static float pitch = 0.0f;

  yaw += mouseDelta.x * sensitivity;
  pitch -= mouseDelta.y * sensitivity;

  pitch = Clamp(pitch, -1.5f, 1.5f);

  Vector3 direction = {
      cosf(pitch) * sinf(yaw),
      sinf(pitch),
      -cosf(pitch) * cosf(yaw)};

  camera.target = Vector3Add(camera.position, direction);
};

inline void Camera3D_Init()
{
  shaderModTime = std::max(GetFileModTime(VS_PATH), GetFileModTime(FS_PATH));
  shader = LoadShader(VS_PATH, FS_PATH);
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  camera = std::make_unique<Camera>(Camera{
      .position = {10.0f, 10.0f, 10.0f},
      .target = {0.0f, 0.0f, 0.0f},
      .up = {0.0f, 1.0f, 0.0f},
      .fovy = 90.0f,
      .projection = CAMERA_PERSPECTIVE});

  cameraLight = CreateLight(LIGHT_POINT, camera->position, {}, WHITE, shader);
  SetShaderValue(shader, GetShaderLocation(shader, "lightPower"), &lightPower, SHADER_UNIFORM_INT);
};

inline void Camera3D_Update()
{
  if (!camera)
    return;

  Camera3D_UpdateShaders();
  cameraLight.position = camera->position;
  // UpdateLightValues(shader, cameraLight);
};