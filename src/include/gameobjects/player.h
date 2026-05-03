#pragma once
#include "gameobject3d.h"
#include "global.h"
#include <functional>
#include <server_ops.h>
#include <net_utils.h>
#include <bsp.h>
#include <camera3d.h>
#include <input_bindings.h>

class Player : public GameObject3D
{
public:
  std::string target_name = "player";
  Vector3 collision_box = {1.0f, 2.0f, 1.0f};
  Vector3 collision_offset = {0.0f, 1.0f, 0.0f};

  void ViewRoll(float smove)
  {
    float horizontalvel = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (horizontalvel > 1.0f)
    {
      float targetroll = (smove / speed) * global_view_roll_angle_limit * global_view_roll_scale;
      global_view_roll_angle = Lerp(global_view_roll_angle, -targetroll, GetFrameTime() * 20);
    }
    else
    {
      global_view_roll_angle = 0.0f;
    }
  };

  void ViewBob()
  {

    float horizontalvel = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (horizontalvel > 1.0f && velocity.y == 0.f)
    {
      global_view_bob_cycle += GetFrameTime() * global_view_bob_freq;
      global_view_bob_intensity = Lerp(global_view_bob_intensity, 1.0f, GetFrameTime() * 10.0f);
    }
    else
    {
      global_view_bob_intensity = Lerp(global_view_bob_intensity, 0.0f, GetFrameTime() * 10.0f);
      if (global_view_bob_intensity < 0.001f)
        global_view_bob_cycle = 0.0f;
    }
  };

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

    int key_fwd = EngineInputCode("up", KEY_W);
    int key_back = EngineInputCode("down", KEY_S);
    int key_right = EngineInputCode("right", KEY_D);
    int key_left = EngineInputCode("left", KEY_A);
    int key_fwd_alt = EngineInputCodeAlt("up");
    int key_back_alt = EngineInputCodeAlt("down");
    int key_right_alt = EngineInputCodeAlt("right");
    int key_left_alt = EngineInputCodeAlt("left");

    UpdateStack(key_fwd, vertical_stack);
    UpdateStack(key_back, vertical_stack);
    if (key_fwd_alt > 0)
      UpdateStack(key_fwd_alt, vertical_stack);
    if (key_back_alt > 0)
      UpdateStack(key_back_alt, vertical_stack);

    UpdateStack(key_right, horizontal_stack);
    UpdateStack(key_left, horizontal_stack);
    if (key_right_alt > 0)
      UpdateStack(key_right_alt, horizontal_stack);
    if (key_left_alt > 0)
      UpdateStack(key_left_alt, horizontal_stack);

    if (!horizontal_stack.empty())
    {
      int last_key = horizontal_stack.back();
      direction.x = (last_key == key_right || last_key == key_right_alt) ? 1.0f : -1.0f;
    }

    if (!vertical_stack.empty())
    {
      int last_key = vertical_stack.back();
      direction.y = (last_key == key_fwd || last_key == key_fwd_alt) ? 1.0f : -1.0f;
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
  void UpdateCamera(Camera3D *camera, float fmove, float smove)
  {
    if (!camera)
      return;

    ViewRoll(smove);
    ViewBob();

    Vector3 viewdir = {
        cosf(global_cam_pitch) * sinf(global_cam_yaw),
        sinf(global_cam_pitch),
        -cosf(global_cam_pitch) * cosf(global_cam_yaw)};

    float current_bob = sinf(global_view_bob_cycle) * global_view_bob_amount * global_view_bob_intensity;

    camera->position = (Vector3){
        position.x,
        Lerp(camera->position.y, position.y + 1.0f + current_bob, 0.5f),
        position.z};

    camera->target = Vector3Add(camera->position, viewdir);
    Vector3 world_up = {0, 1, 0};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(viewdir, world_up));
    float roll = global_view_roll_angle * DEG2RAD;

    camera->up = (Vector3){
        world_up.x * cosf(roll) + right.x * sinf(roll),
        world_up.y * cosf(roll) + right.y * sinf(roll),
        world_up.z * cosf(roll) + right.z * sinf(roll)};
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

    UpdateCamera(camera.get(), fmove, smove);
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
           // Pitch goes alongside angle so the server-side traceline used
           // by sv_network.lua's shoot/interact handlers matches the
           // client's visual aim. Without it the server traces yaw-only.
           Net_ToCPPClient(peer, "set_pitch", global_cam_pitch);
         }},

        // BroadcastPosition
        {i_am_host,
         "BroadcastPosition",
         [&]()
         {
           SetPosition(position, my_local_player_id);
           SetAngle(angle, my_local_player_id);
           // Host writes its own pitch directly — no network round trip.
           pitch = global_cam_pitch;
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
    {
      // Draw laser sight for local player only (no body drawn in first person)
      if (global_show_collisions)
      {
        const float range = 2048.0f;
        Vector3 eye = GetEyePos();
        Vector3 fwd = GetViewForward();
        TraceResult tr = TraceViewLine(range);
        Vector3 hit_pos = Vector3Add(eye, Vector3Scale(fwd, tr.fraction * range));

        Color beam_color = GREEN;
        Color dot_color = GREEN;
        if (tr.hit_type == TraceResult::HitType::World)
        {
          beam_color = RED;
          dot_color = RED;
        }
        else if (tr.hit_type == TraceResult::HitType::BrushEntity)
        {
          beam_color = ORANGE;
          dot_color = ORANGE;
        }
        else if (tr.hit_type == TraceResult::HitType::Object)
        {
          beam_color = YELLOW;
          dot_color = YELLOW;
        }

        // Offset the line start slightly forward + to the right so it's
        // visible in first person (a line going straight away from the camera
        // is invisible — you're looking along it).
        Vector3 right = {cosf(global_cam_yaw), 0.0f, sinf(global_cam_yaw)};
        Vector3 draw_start = Vector3Add(
            Vector3Add(eye, Vector3Scale(fwd, 0.3f)),
            Vector3Scale(right, 0.12f));
        DrawLine3D(draw_start, hit_pos, beam_color);

        // Small cross at the hit point (no sphere)
        if (tr.fraction < 1.0f)
        {
          float cs = 0.06f;
          DrawLine3D({hit_pos.x - cs, hit_pos.y, hit_pos.z}, {hit_pos.x + cs, hit_pos.y, hit_pos.z}, beam_color);
          DrawLine3D({hit_pos.x, hit_pos.y - cs, hit_pos.z}, {hit_pos.x, hit_pos.y + cs, hit_pos.z}, beam_color);
          DrawLine3D({hit_pos.x, hit_pos.y, hit_pos.z - cs}, {hit_pos.x, hit_pos.y, hit_pos.z + cs}, beam_color);
        }
      }
      return;
    }
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