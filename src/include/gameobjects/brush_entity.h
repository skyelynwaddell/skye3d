#pragma once
#include <gameobject3d.h>
#include <bsp.h>

/*
BrushEntity
A GameObject3D that owns a piece of BSP brush geometry and its entity data.
Subclass this for specific entity types (func_door, trigger_once, etc.).

Basic usage — just renders the brush at its origin and provides tag access:

  class MyDoor : public BrushEntity
  {
  public:
    void Update() override
    {
      // GetTag("speed"), GetTag("targetname"), etc.
    }
  };
*/
class BrushEntity : public GameObject3D
{
public:
  Model brush_model = {};
  bool has_model = false;
  int clipnode_root = -1;

  Model cube;

  // Build from data returned by BSP_SpawnBrushEntities()
  explicit BrushEntity(const BSP_BrushEntityData &data)
  {
    classname = data.classname;
    tags = data.tags;
    position = data.origin;
    target_name = GetTag("targetname");
    target = GetTag("target");
    brush_model = data.model;
    has_model = data.has_model;
    clipnode_root = data.clipnode_root;
    spawn_origin = data.origin;

    bsp_collider.entity_hulls.push_back({clipnode_root,
                                         &position,
                                         data.origin});

    printf("BRUSH ENTITY CREATED: %p | Class: %s\n", (void *)this, classname.c_str());
    printf("  Spawn Origin: (%.2f, %.2f, %.2f)\n", spawn_origin.x, spawn_origin.y, spawn_origin.z);
    printf("  Position: (%.2f, %.2f, %.2f)\n", position.x, position.y, position.z);
  }

  // Read a tag value, returns fallback if not present
  std::string GetTag(const std::string &key, const std::string &fallback = "") const
  {
    auto it = tags.find(key);
    return it != tags.end() ? it->second : fallback;
  }

  Vector3 GetInteractCenter() override
  {
    if (has_model)
    {
      BoundingBox box = GetModelBoundingBox(brush_model);
      Vector3 center = {
          (box.min.x + box.max.x) * 0.5f,
          (box.min.y + box.max.y) * 0.5f,
          (box.min.z + box.max.z) * 0.5f};

      // Add the entity's current position (not delta)
      return Vector3Add(center, position);
    }
    return position;
  }

  void OnTrigger() override
  {
    GameObject3D::OnTrigger();
  };

  void Update() override
  {
    GameObject3D::Update();
  };

  void Draw() override
  {
    // !classname.starts_with("trigger")
    if (has_model)
    {
      Vector3 offset = Vector3Subtract(position, spawn_origin);

      // Draw the model using the offset to move the baked vertices
      DrawModel(brush_model, offset, 1.0f, WHITE);
      DrawModelWires(brush_model, offset, 1.0f, RED);
    }
  }

  void DrawDebug() override
  {
    if (has_model)
    {
    }
  }

  void CleanUp() override
  {
    if (has_model)
    {
      UnloadModel(brush_model);
      has_model = false;
    }
  }
};

/*
SpawnBrushEntities
Convenience wrapper: calls BSP_SpawnBrushEntities(), creates a BrushEntity for
each result, and pushes it into the global gameobjects array.

If you need custom logic per classname, replace this with your own loop:

  for (auto &data : BSP_SpawnBrushEntities())
  {
    if      (data.classname == "func_door")    gameobjects.push_back(std::make_unique<Door>(data));
    else if (data.classname == "trigger_once") gameobjects.push_back(std::make_unique<TriggerOnce>(data));
    else                                       gameobjects.push_back(std::make_unique<BrushEntity>(data));
  }
*/
inline void SpawnBrushEntities()
{
  for (auto &data : BSP_SpawnBrushEntities())
    gameobjects.push_back(std::make_unique<BrushEntity>(data));
}
