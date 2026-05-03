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

    collision_box    = {0, 0, 0};
    collision_offset = {0, 0, 0};

    {
      EntityHull h;
      h.root         = clipnode_root;       // hull 1: pre-expanded, horizontal walls
      h.root_h0      = data.bsp_node_root;  // hull 0: exact geometry, step up/down
      h.entity_pos   = &position;
      h.spawn_origin = data.origin;
      h.owner        = this;
      if (has_model)
      {
        BoundingBox bb = GetModelBoundingBox(brush_model);
        h.bbox_min  = bb.min;
        h.bbox_max  = bb.max;
        h.has_bbox  = true;
      }
      bsp_collider.entity_hulls.push_back(h);
    }

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

Assign deterministic client_ids (1000 + entity_index) at construction time.
Both host and client iterate the same BSP entity list in the same order, so
the resulting ids match without any cross-process coordination. Doing this
here (in C++ during Init) — instead of later in luaSpawnBrushEntities() during
the first luaUpdate — ensures every BrushEntity has a real id BEFORE any
network sync packet can possibly arrive. Otherwise pos/size sync packets sent
during the connect window find no matching local object and the unmatched-id
branch creates a phantom duplicate GameObject3D that owns the id forever.
*/
inline void SpawnBrushEntities()
{
  int idx = 0;
  for (auto &data : BSP_SpawnBrushEntities())
  {
    auto obj = std::make_unique<BrushEntity>(data);
    obj->client_id = 1000 + idx;
    printf("[%s] BrushEntity #%d cls=%s origin=(%.2f,%.2f,%.2f) root=%d root_h0=%d\n",
           global_is_hosting ? "HOST" : "CLIENT",
           obj->client_id, data.classname.c_str(),
           data.origin.x, data.origin.y, data.origin.z,
           data.clipnode_root, data.bsp_node_root);
    ++idx;
    gameobjects.push_back(std::move(obj));
  }
  printf("[%s] SpawnBrushEntities done: %d entities, entity_hulls.size()=%zu\n",
         global_is_hosting ? "HOST" : "CLIENT",
         idx, bsp_collider.entity_hulls.size());
}
