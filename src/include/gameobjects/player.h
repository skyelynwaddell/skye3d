#pragma once
#include "gameobject3d.h"
#include "global.h"
#include <functional>
#include <server_ops.h>
#include <net_utils.h>
#include <bsp.h>
#include <camera3d.h>

class Player : public GameObject3D
{
public:
  std::string target_name = "player";
  /*
  PlayerMovement
  Function must be called every frame to enable movement
  */
  void PlayerMovement()
  {
    if (!IsMenuMode)
    {
      Vector2 mouseDelta = GetMouseDelta();
      global_cam_yaw += mouseDelta.x * global_cam_sensitivity;
      global_cam_pitch -= mouseDelta.y * global_cam_sensitivity;

      if (IsGamepadAvailable(0))
      {
        const float deadzone = 0.15f;
        float stickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float stickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        if (fabsf(stickX) > deadzone)
          global_cam_yaw += stickX * global_cam_sensitivity * global_analog_sens * GetFrameTime();
        if (fabsf(stickY) > deadzone)
          global_cam_pitch -= stickY * global_cam_sensitivity * global_analog_sens * GetFrameTime();
      }
      global_cam_pitch = Clamp(global_cam_pitch, -global_cam_pitch_max, global_cam_pitch_max);
    }

    float dt = GetFrameTime();

    // Calculate basis vectors for movement (horizontal only)
    Vector3 forward = {sinf(global_cam_yaw), 0.f, -cosf(global_cam_yaw)};
    Vector3 right = {cosf(global_cam_yaw), 0.f, sinf(global_cam_yaw)};

    // Gather raw input magnitudes (Quake uses +/- 320 for moves)
    float fmove = 0.0f;
    float smove = 0.0f;
    const float quake_speed = 320.0f;

    if (IsKeyDown(KEY_W))
      fmove += quake_speed;
    if (IsKeyDown(KEY_S))
      fmove -= quake_speed;
    if (IsKeyDown(KEY_D))
      smove += quake_speed;
    if (IsKeyDown(KEY_A))
      smove -= quake_speed;

    if (IsGamepadAvailable(0))
    {
      const float deadzone = 0.15f;
      float moveX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
      float moveY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
      if (fabsf(moveX) > deadzone)
        smove = moveX * quake_speed;
      if (fabsf(moveY) > deadzone)
        fmove = -moveY * quake_speed;
    }

    // Handle Jump: In Quake, jumping is an instantaneous velocity change
    // We check grounding before we move for the frame
    bool grounded = bsp_collider.IsGrounded();
    const float jump_vel = 8.0f; // Scale this as needed for your world

    if (grounded && (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))
    {
      velocity.y = jump_vel;
    }

    position = bsp_collider.MoveAndSlide(position, velocity, forward, right, fmove, smove, dt);

    Vector3 viewDir = {
        sinf(global_cam_yaw) * cosf(global_cam_pitch),
        sinf(global_cam_pitch),
        -cosf(global_cam_yaw) * cosf(global_cam_pitch)};

    camera->position = (Vector3){position.x, position.y + 0.5f, position.z};
    camera->target = Vector3Add(camera->position, viewDir);
  };

  /*
  CameraFollowPlayer
  Makes camera follow... player?
  */
  void CameraFollowPlayer()
  {
    if (!camera)
      return;

    Vector3 direction = {
        cosf(global_cam_pitch) * sinf(global_cam_yaw),
        sinf(global_cam_pitch),
        -cosf(global_cam_pitch) * cosf(global_cam_yaw)};
    camera->target = Vector3Add(camera->position, direction);
  };

  /*
  SyncClientServer
  Call at the end of update, and sync any Data that must be sent here.
  */
  void SyncClientServer()
  {
    bool valid_id = (my_local_player_id >= 0);
    bool i_am_client = (peer != nullptr && !global_is_hosting);
    bool i_am_host = (server_online && global_is_hosting && valid_id);

    struct SyncJob
    {
      bool condition;
      const char *name;
      std::function<void()> fn;
    };

    SyncJob jobs[] = {

        // SyncPosition
        {i_am_client,
         "SyncPosition",
         [&]()
         { Net_ToCPPClient(peer, "set_position", position); }},

        // BroadcastPosition
        {i_am_host,
         "BroadcastPosition",
         [&]()
         { SetPosition(position, my_local_player_id); }},

        // Add new sync tasks here:
        // { some_condition, "MyNewSync", [&]() { ... } },
    };

    // run all jobs
    for (auto &job : jobs)
      if (job.condition)
        job.fn();
  };

  /*
  Update
  Called every frame to update the object
  */
  void Update() override
  {
    bool isLocal = (client_id == my_local_player_id);
    if (isLocal && is_me)
    {
      UpdateInputMode();
      PlayerMovement();
      if (camera)
      {
        // Horizontal snappy
        camera->position.x = position.x;
        camera->position.z = position.z;

        // Vertical Lerpy for stairs
        float camlerp = 20.0f * GetFrameTime();
        camera->position.y = Lerp(camera->position.y, position.y + 1.5f, camlerp);
      }
      CameraFollowPlayer();
      SyncClientServer();
    }
  };

  /*
  Draw
  The draw event for the object
  */
  void Draw() override
  {
    if (client_id == my_local_player_id)
      return;
    GameObject3D::Draw();
  };

  /*
  DrawGUI
  Draw to the GUI in this event here
  */
  void DrawGUI() override
  {
  }

  /*
  CleanUp
  Called right before the object is destroyed
  */
  void CleanUp() override
  {
  }
};