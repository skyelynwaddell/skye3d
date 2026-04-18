#pragma once
#include "enet.h"
#include "global.h"
#include "sq.h"
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

void SendToClient(ENetPeer *peer, uint8_t type, const void *data, size_t data_len);

/*
UpdateInputMode
*/
inline bool IsMenuMode = false;
inline void UpdateInputMode()
{
  if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    return;
  IsMenuMode = !IsMenuMode;
  IsMenuMode ? EnableCursor() : DisableCursor();
};

// GameObject3D
using ScriptValue = std::variant<std::string, float, bool, int>; // value a .nut script variable can be
class GameObject3D
{
public:
  virtual ~GameObject3D() = default;
  int client_id = -1;
  bool is_me = false;
  bool destroy_me = false;
  std::string target_name = "";
  std::string target = "";
  Vector3 velocity = {0.0f, 0.0f, 0.0f};
  Vector3 position = {0.0f, 0.0f, 0.0f};
  Vector3 collision_offset = {0.0f, 0.0f, 0.0f};
  Vector3 collision_box = {0.5f, 0.6f, 0.5f};
  Vector3 size = {0.5f, 0.6f, 0.5f};
  float speed = 200.0f;
  float acceleration = 20.0f;
  int sendflags = 0;
  std::unordered_map<std::string, ScriptValue> script_vars;

  void Destroy() { destroy_me = true; };
  bool IsMoving() { return Vector3Length(velocity) > 0.01f; };

  // overrides
  virtual void Update() {};
  virtual void Draw()
  {
    if (global_show_collisions)
    {
      Vector3 drawPos = Vector3Add(position, collision_offset);
      DrawCubeWires(drawPos, collision_box.x, collision_box.y, collision_box.z, MAROON);
    }
  };
  virtual void DrawDebug() {};
  virtual void DrawGUI() {};
  virtual void CleanUp() {};
};

// stores all gameobjects in game
inline std::vector<std::unique_ptr<GameObject3D>> gameobjects;

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
  printf("DEBUG: Instance created: Type=%s, Address=%p\n",
         typeid(*ptr).name(),
         (void *)ptr);

  gameobjects.push_back(std::move(obj));
  return ptr;
};