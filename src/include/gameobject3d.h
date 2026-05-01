#pragma once
#include "enet.h"
#include "global.h"
#include "engine.h"
#include "camera3d.h"
#include <raylib.h>
#include <type_traits>
#include <string>
#include <unordered_map>
#include <variant>
#include <memory>
#include <functional>
#include <vector>
#include "bsp.h"
#include "gamemodel.h"

void SendToClient(ENetPeer *peer, uint8_t type, const void *data, size_t data_len);

/*
UpdateInputMode
*/
enum SyncFlags
{
  SYNC_NONE = 0,
  SYNC_POS = 1 << 0,        // 1
  SYNC_ANGLE = 1 << 1,      // 2
  SYNC_MODEL = 1 << 2,      // 4 (for when an entity changes skins/models)
  SYNC_SIZE = 1 << 3,       // 5 updates collision size
  SYNC_CLASSNAME = 1 << 4,  // 5 updates collision size
  SYNC_VISIBLE = 1 << 5,    // 5 updates collision size
  SYNC_SCRIPTVARS = 1 << 5, // 5 updates collision size
  SYNC_ALL = 0xFFFF
};

// GameObject3D
using ScriptValue = std::variant<std::string, float, bool, int>; // value a .nut script variable can be
class GameObject3D
{
public:
  virtual ~GameObject3D() = default;
  int client_id = -1; // multiplayer client id only for player objects
  int sync_flags = SYNC_NONE;
  int spawn_flags = 0;
  bool is_me = false;
  bool destroy_me = false;
  bool visible = true;
  // Hitscan opt-out. Defaults to true for everything — set false on entities
  bool blocks_hitscan = true;
  // 0 = impenetrable (default). >0 = how much "penetration budget" this
  // surface costs to shoot through. Wallbang traces start with a budget
  // (e.g. cardboard=1, drywall=2, brick=99) and tunnel through any entity
  // whose strength is <= remaining budget.
  int wallbang_strength = 0;
  std::string classname = "";
  std::string target_name = "";
  std::string target = "";
  Vector3 velocity = {0.0f, 0.0f, 0.0f};
  Vector3 position = {0.0f, 0.0f, 0.0f};
  Vector3 spawn_origin = {0, 0, 0};
  std::unordered_map<std::string, std::string> tags;
  Vector3 last_position = {0.0f, 0.0f, 0.0f};
  Vector3 collision_box = {1.0f, 2.0f, 1.0f};
  Vector3 collision_offset = {0.0f, 0.0f, 0.0f};
  float speed = 200.0f;
  float acceleration = 20.0f;
  int sendflags = 0;
  int spawnflags = 0;
  float angle = 0;
  float last_angle = 0;
  int leaf_id = 0;
  GameModel game_model = {};
  std::function<void(GameObject3D *)> on_trigger_fn;

  std::unordered_map<std::string, ScriptValue> script_vars;

  Color debug_color = MAROON;

  bool has_server_think = false;
  bool has_client_think = false;

  void Destroy() { destroy_me = true; };
  bool IsMoving() { return Vector3Length(velocity) > 0.01f; };

  void UpdateInputMode()
  {
    if (IsMenuMode)
    {
      velocity = {0, velocity.y, 0};
    }
  };

  virtual Vector3 GetInteractCenter()
  {
    return position;
  }

  // Forward direction derived from angle (same convention as the player camera).
  // angle is stored as -cam_yaw * RAD2DEG, so we reverse it back to get yaw.
  Vector3 GetForward() const
  {
    float yaw = -angle * DEG2RAD;
    return {sinf(yaw), 0.0f, -cosf(yaw)};
  }

  // Eye / muzzle position — must match the camera offset in Player::UpdateCamera.
  inline static float eye_offset = 1.0f;
  Vector3 GetEyePos() const
  {
    return {position.x,
            position.y + eye_offset,
            position.z};
  }

  // Full 3D view direction using global camera pitch+yaw.
  // GetForward() is yaw-only (for NPCs/objects); this version is for the local player's aim.
  Vector3 GetViewForward() const
  {
    float yaw = global_cam_yaw;
    float pitch = global_cam_pitch;
    return Vector3Normalize({cosf(pitch) * sinf(yaw),
                             sinf(pitch),
                             -cosf(pitch) * cosf(yaw)});
  }

  // Defined below, after gameobjects is declared.
  TraceResult TraceLine(float range = 2048.0f);
  TraceResult TraceLineObjects(float range = 2048.0f);
  TraceResult TraceViewLine(float range = 2048.0f); // pitched, uses camera yaw+pitch

  std::vector<GameObject3D *> FindAllObjectsInRange(float max_dist);
  GameObject3D *FindClosestObject(float max_dist);

  // overrides
  virtual void Update()
  {
    if (global_is_hosting)
    {
      if (Vector3Equals(position, last_position) == false)
      {
        leaf_id = bsp_renderer.FindLeaf(position);
        sync_flags |= SYNC_POS;
      }
      if (angle != last_angle)
        sync_flags |= SYNC_ANGLE;

      last_position = position;
      last_angle = angle;
    }
  };

  void set_size(Vector3 min, Vector3 max)
  {
    // collision_box is max - min
    collision_box = {
        max.x - min.x,
        max.y - min.y,
        max.z - min.z};

    // collision_offset is the center of that box relative to the origin (0,0,0)
    // origin is often at the feet, so the offset isn't always zero.
    collision_offset = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f};
  };

  virtual void Draw()
  {
    if (game_model.model.meshCount > 0 && visible)
      DrawModelEx(
          game_model.model,
          {
              position.x,
              position.y + collision_offset.y - (collision_box.y / 2),
              position.z,
          },
          {0, 1, 0}, // rotate around Y
          angle,
          game_model.scale,
          WHITE);

    if (global_show_collisions)
    {

      rlPushMatrix();
      rlTranslatef(position.x, position.y, position.z);
      rlRotatef(angle, 0, 1, 0);
      rlTranslatef(collision_offset.x, collision_offset.y, collision_offset.z);

      Color debug_color = MAGENTA;
      if (classname.starts_with("path"))
        debug_color = SKYBLUE;
      else if (classname.starts_with("info"))
        debug_color = GRAY;
      else if (classname.starts_with("monster"))
        debug_color = MAROON;
      else if (classname.starts_with("item"))
        debug_color = PURPLE;
      else if (classname.starts_with("weapon"))
        debug_color = BLUE;
      else if (classname.starts_with("player"))
        debug_color = YELLOW;

      DrawCubeWires(Vector3{0, 0, 0}, collision_box.x, collision_box.y, collision_box.z, debug_color);

      rlPopMatrix();
    }
  }

  virtual void OnTrigger()
  {
    if (this && this->on_trigger_fn)
    {
      this->on_trigger_fn(this);
    }
  };

  virtual void DrawDebug() {};
  virtual void DrawGUI() {};
  virtual void CleanUp() {};
};

// Definitions for the accessor shims forward-declared in bsp.h.
// Kept here so bsp.h can use them in non-template code without needing
// the full GameObject3D type at that point in the header chain.
inline bool GO_BlocksHitscan(const GameObject3D *o)        { return o ? o->blocks_hitscan : true; }
inline int  GO_WallbangStrength(const GameObject3D *o)     { return o ? o->wallbang_strength : 0; }
inline void GO_SetBlocksHitscan(GameObject3D *o, bool v)   { if (o) o->blocks_hitscan = v; }

// stores all gameobjects in game
inline std::vector<std::unique_ptr<GameObject3D>> gameobjects;

// TraceLine / TraceLineObjects / TraceViewLine — defined here so gameobjects is in scope.
// Cast this to const GameObject3D* so the template always deduces T = GameObject3D,
// matching the vector element type regardless of which subclass calls the method.
inline TraceResult GameObject3D::TraceLine(float range)
{
  Vector3 from = GetEyePos();
  Vector3 to = Vector3Add(from, Vector3Scale(GetForward(), range));
  return bsp_collider.TraceAll(from, to, gameobjects, static_cast<const GameObject3D *>(this));
}

inline TraceResult GameObject3D::TraceLineObjects(float range)
{
  Vector3 from = GetEyePos();
  Vector3 to = Vector3Add(from, Vector3Scale(GetForward(), range));
  return bsp_collider.TraceObjects(from, to, gameobjects, static_cast<const GameObject3D *>(this));
}

inline TraceResult GameObject3D::TraceViewLine(float range)
{
  Vector3 from = GetEyePos();
  Vector3 to = Vector3Add(from, Vector3Scale(GetViewForward(), range));
  return bsp_collider.TraceAll(from, to, gameobjects, static_cast<const GameObject3D *>(this));
}

/*
DestroyIfNeeded
Checks if this object was marked for destruction
and destroys it
*/
inline bool DestroyIfNeeded(int i)
{
  if (gameobjects[i]->destroy_me)
  {
    gameobjects[i]->CleanUp();
    gameobjects.erase(gameobjects.begin() + i);
    return true;
  }
  return false;
};

inline void GameLoop(const std::string &event, const std::function<void(size_t)> &func)
{
  for (size_t i = 0; i < gameobjects.size(); ++i)
  {
    if (gameobjects[i])
    {
      if (DestroyIfNeeded(i))
        continue;
      try
      {
        func(i);
      }
      catch (const std::exception &e)
      {
        printf("Exception in %s at index %zu: %s\n", event.c_str(), i, e.what());
      }
      catch (...)
      {
        printf("Unknown exception in %s at index %zu\n", event.c_str(), i);
      }
    }
  }
};

/*
GameObject3D_DrawAll
Draw all objects
*/
inline void GameObject3D_DrawAll()
{
  GameLoop("draw", [&](size_t i)
           { gameobjects[i]->Draw(); });
};
/*
GameObject3D_DrawAllDebug
Draw all objects
*/
inline void GameObject3D_DrawAllDebug()
{
  GameLoop("drawdebug", [&](size_t i)
           { gameobjects[i]->DrawDebug(); });
};

/*
GameObject3D_UpdateAll
Update all objects
*/
inline void GameObject3D_UpdateAll()
{
  GameLoop("update", [&](size_t i)
           { gameobjects[i]->Update(); });
};

/*
GameObject3D_DrawGUIAll
Update all objects
*/
inline void GameObject3D_DrawGUIAll()
{
  GameLoop("draw_gui", [&](size_t i)
           { gameobjects[i]->DrawGUI(); });
};

// InstanceFind
// Returns the first ptr found of an instance if one exists
// probably only good for singletons tbh like the player, or camera be careful
// usage :  Player* player_ref = InstanceFind<Player>();
template <typename T>
  requires(std::is_base_of_v<GameObject3D, T>)
T *InstanceFind()
{
  for (const auto &obj : gameobjects)
  {
    if (auto *instance = dynamic_cast<T *>(obj.get()))
      return instance;
  }
  return nullptr;
};

// InstanceFindByTargetName
// usage :  Player* player_ref = InstanceFindByTargetName<Player>("torch");
template <typename T>
  requires(std::is_base_of_v<GameObject3D, T>)
T *InstanceFindByTargetName(std::string target_name)
{
  for (const auto &obj : gameobjects)
  {
    if (auto *instance = dynamic_cast<T *>(obj.get()))
    {
      if (instance->target_name != target_name)
        continue;
      return instance;
    }
  }
  return nullptr;
};

/*
InstanceCreate
usage: Player* player = InstanceCreate<Player>({ spawn_x, spawn_y });
*/
template <class T, class... Args>
  requires(std::is_base_of_v<GameObject3D, T>)
T *InstanceCreate(Vector3 spawn_pos, Args &&...args)
{
  auto obj = std::make_unique<T>(std::forward<Args>(args)...);
  T *ptr = obj.get();
  ptr->position = spawn_pos;
  gameobjects.push_back(std::move(obj));
  return ptr;
};