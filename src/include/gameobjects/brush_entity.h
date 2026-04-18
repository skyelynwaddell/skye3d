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
  std::unordered_map<std::string, std::string> tags;
  Model brush_model = {};
  bool has_model = false;

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
    cube = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  }

  // Read a tag value, returns fallback if not present
  std::string GetTag(const std::string &key, const std::string &fallback = "") const
  {
    auto it = tags.find(key);
    return it != tags.end() ? it->second : fallback;
  }

  void Draw() override
  {
    if (has_model)
    {
      if (classname.starts_with("trigger") == false)
        DrawModel(brush_model, position, 1.0f, WHITE);
      DrawModelWires(brush_model, position, 1.0f, RED);
    }
  }

  void DrawDebug() override
  {
    if (has_model)
    {
      // DrawModelWires(brush_model, position, 1, RED);
      // for (int m = 0; m < brush_model.meshCount; m++)
      // {
      //   Mesh mesh = brush_model.meshes[m];

      //   for (int t = 0; t < mesh.triangleCount; t++)
      //   {
      //     int i0 = mesh.indices[t * 3 + 0];
      //     int i1 = mesh.indices[t * 3 + 1];
      //     int i2 = mesh.indices[t * 3 + 2];

      //     Vector3 v0 = {
      //         mesh.vertices[i0 * 3 + 0],
      //         mesh.vertices[i0 * 3 + 1],
      //         mesh.vertices[i0 * 3 + 2]};

      //     Vector3 v1 = {
      //         mesh.vertices[i1 * 3 + 0],
      //         mesh.vertices[i1 * 3 + 1],
      //         mesh.vertices[i1 * 3 + 2]};

      //     Vector3 v2 = {
      //         mesh.vertices[i2 * 3 + 0],
      //         mesh.vertices[i2 * 3 + 1],
      //         mesh.vertices[i2 * 3 + 2]};

      //     DrawLine3D(v0, v1, RED);
      //     DrawLine3D(v1, v2, RED);
      //     DrawLine3D(v2, v0, RED);
      //   }
      // }
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
