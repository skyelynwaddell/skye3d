#pragma once
#include <gameobject3d.h>
#include <bsp.h>

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
    target_name = GetTag("targetname");
    target = GetTag("target");
    brush_model = data.model;
    has_model = data.has_model;
    clipnode_root = data.clipnode_root;

    position = data.origin;
    spawn_origin = data.origin;

    bsp_collider.entity_hulls.push_back({clipnode_root,
                                         &position,
                                         data.origin});

    if (classname.starts_with("trigger"))
    {
      visible = false;
    }
  }

  // Read a tag value, returns fallback if not present
  std::string GetTag(const std::string &key, const std::string &fallback = "") const
  {
    auto it = tags.find(key);
    return it != tags.end() ? it->second : fallback;
  }

  BoundingBox GetBoundingBox()
  {
    if (!has_model)
      return {position, position};

    BoundingBox box = GetModelBoundingBox(brush_model);

    Vector3 current_movement = Vector3Subtract(position, spawn_origin);

    box.min = Vector3Add(box.min, current_movement);
    box.max = Vector3Add(box.max, current_movement);

    return box;
  }

  Vector3 GetInteractCenter() override
  {
    if (has_model)
    {
      BoundingBox box = GetBoundingBox();

      if (isnan(box.min.x) || isinf(box.min.x))
        return position;

      return {
          (box.min.x + box.max.x) * 0.5f,
          (box.min.y + box.max.y) * 0.5f,
          (box.min.z + box.max.z) * 0.5f};
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
    if (!has_model)
      return;

    if (!visible)
      return;

    BoundingBox bb = GetModelBoundingBox(brush_model);

    Vector3 model_center = {
        (bb.min.x + bb.max.x) * 0.5f,
        (bb.min.y + bb.max.y) * 0.5f,
        (bb.min.z + bb.max.z) * 0.5f};
    Vector3 draw_offset = Vector3Subtract(position, model_center);
    DrawModel(brush_model, draw_offset, 1.001f, WHITE); // scale up 0.001 so we dont z-fight
  };

  void DrawDebug() override
  {
    if (!global_show_collisions)
      return;

    DrawBoundingBox(GetBoundingBox(), GREEN);
  }

  void CleanUp() override
  {
    if (!has_model)
      return;

    UnloadModel(brush_model);
    has_model = false;
  }
};

/*
SpawnBrushEntities
*/
inline void SpawnBrushEntities()
{
  for (auto &data : BSP_SpawnBrushEntities())
    gameobjects.push_back(std::make_unique<BrushEntity>(data));
}
