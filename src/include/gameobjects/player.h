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
HandleRotation
handles mouse yaw and pitch
*/
  void HandleRotation(float dt)
  {
    Vector2 mouse_delta = GetMouseDelta();
    global_cam_yaw += mouse_delta.x * global_cam_sensitivity;
    global_cam_pitch -= mouse_delta.y * global_cam_sensitivity;

    if (IsGamepadAvailable(0))
    {
      const float deadzone = 0.15f;
      float stick_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
      float stick_y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

      if (fabsf(stick_x) > deadzone)
        global_cam_yaw += stick_x * global_cam_sensitivity * global_analog_sens * 100.0f * dt;
      if (fabsf(stick_y) > deadzone)
        global_cam_pitch -= stick_y * global_cam_sensitivity * global_analog_sens * 100.0f * dt;
    }

    global_cam_pitch = Clamp(global_cam_pitch, -global_cam_pitch_max, global_cam_pitch_max);
  };

  /*
    GetInput
    handles directional input of player
    */
  // Static stacks to track the order of key presses
  static inline std::vector<int> horizontal_stack;
  static inline std::vector<int> vertical_stack;

  void GetInput(float &fmove, float &smove)
  {
    Vector2 direction = {0.0f, 0.0f};

    auto UpdateStack = [](int key, std::vector<int> &stack)
    {
      if (IsKeyPressed(key))
      {
        stack.push_back(key);
      }
      if (IsKeyReleased(key))
      {
        stack.erase(std::remove(stack.begin(), stack.end(), key), stack.end());
      }
    };

    UpdateStack(KEY_W, vertical_stack);
    UpdateStack(KEY_S, vertical_stack);
    UpdateStack(KEY_D, horizontal_stack);
    UpdateStack(KEY_A, horizontal_stack);

    if (!horizontal_stack.empty())
    {
      int last_key = horizontal_stack.back();
      direction.x = (last_key == KEY_D) ? 1.0f : -1.0f;
    }

    if (!vertical_stack.empty())
    {
      int last_key = vertical_stack.back();
      direction.y = (last_key == KEY_W) ? 1.0f : -1.0f;
    }

    if (IsGamepadAvailable(0))
    {
      const float deadzone = 0.15f;
      float move_x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
      float move_y = -GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

      if (fabsf(move_x) > deadzone)
        direction.x = move_x;
      if (fabsf(move_y) > deadzone)
        direction.y = move_y;
    }

    if (Vector2Length(direction) > 1.0f)
      direction = Vector2Normalize(direction);

    smove = direction.x * speed;
    fmove = direction.y * speed;
  }

  /*
Update Camera
updates camera position, yaw, and pitch
*/
  void UpdateCamera(Camera3D *camera)
  {
    if (!camera)
      return;

    Vector3 viewdir = {
        sinf(global_cam_yaw),
        tanf(global_cam_pitch),
        -cosf(global_cam_yaw)};
    camera->position = (Vector3){position.x, position.y + 0.5f, position.z};
    camera->target = Vector3Add(camera->position, viewdir);
  };

  /*
  PlayerMovement
  Function must be called every frame to enable movement
  */
  void PlayerMovement()
  {
    float dt = GetFrameTime();

    if (!IsMenuMode)
      HandleRotation(dt);

    angle = -global_cam_yaw * RAD2DEG;
    Vector3 forward = {sinf(global_cam_yaw), 0.f, -cosf(global_cam_yaw)};
    Vector3 right = {cosf(global_cam_yaw), 0.f, sinf(global_cam_yaw)};

    float fmove, smove = 0.0f;

    if (!IsMenuMode)
      GetInput(fmove, smove);
    else
    {
      vertical_stack.clear();
      horizontal_stack.clear();
      velocity = {0, 0, 0};
      fmove = 0.0f;
      smove = 0.0f;
    }

    position = bsp_collider.MoveAndSlide(position, velocity, forward, right, fmove, smove);

    UpdateCamera(camera.get());
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
         {
           Net_ToCPPClient(peer, "set_position", position);
           Net_ToCPPClient(peer, "set_angle", angle);
         }},

        // BroadcastPosition
        {i_am_host,
         "BroadcastPosition",
         [&]()
         {
           SetPosition(position, my_local_player_id);
           SetAngle(angle, my_local_player_id);
         }},

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
      SyncClientServer();
    }
    GameObject3D::Update();
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