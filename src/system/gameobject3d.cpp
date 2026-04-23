#include "gameobject3d.h"

std::vector<GameObject3D *> GameObject3D::FindAllObjectsInRange(float max_dist)
{
  std::vector<GameObject3D *> results;

  for (auto &obj : gameobjects)
  {
    if (!obj || obj.get() == this)
      continue;

    if (obj->classname.empty())
      continue;

    if (obj->classname.starts_with("info") ||
        obj->classname.starts_with("path") ||
        obj->classname.starts_with("worldspawn"))
      continue;

    Vector3 world_pos = obj->GetInteractCenter();
    float dist = Vector3Distance(world_pos, this->position);

    if (dist <= max_dist)
    {
      results.push_back(obj.get());
    }
  }

  return results;
}

/*
  FindClosestObject
  */
GameObject3D *GameObject3D::FindClosestObject(float max_dist)
{
  // printf("\n=== FindClosestObject DEBUG ===\n");
  // printf("  Player position: (%.2f, %.2f, %.2f)\n", position.x, position.y, position.z);
  // printf("  Search radius: %.2f\n", max_dist);
  // printf("  Total gameobjects: %zu\n", gameobjects.size());

  GameObject3D *best = nullptr;
  float best_dist = max_dist;
  int skipped_count = 0;
  int considered_count = 0;

  for (auto &obj : gameobjects)
  {
    if (!obj)
    {
      // printf("  [SKIP] null object\n");
      continue;
    }

    if (obj.get() == this)
    {
      // printf("  [SKIP] self (%s)\n", obj->classname.c_str());
      continue;
    }

    if (obj->classname.empty())
    {
      // printf("  [SKIP] empty classname\n");
      continue;
    }

    if (obj->classname.starts_with("info") ||
        obj->classname.starts_with("path") ||
        obj->classname.starts_with("worldspawn"))
    {
      // printf("  [SKIP] info/path entity: %s\n", obj->classname.c_str());
      skipped_count++;
      continue;
    }

    Vector3 world_pos = obj->GetInteractCenter();
    float dist = Vector3Distance(world_pos, this->position);

    // printf("  [CHECK] %s | pos:(%.2f,%.2f,%.2f) | dist:%.2f\n",
    //        obj->classname.c_str(), world_pos.x, world_pos.y, world_pos.z, dist);
    considered_count++;

    if (dist < best_dist)
    {
      best_dist = dist;
      best = obj.get();
      // printf("    -> NEW BEST!\n");
    }
  }

  // printf("  Skipped: %d | Considered: %d\n", skipped_count, considered_count);
  // printf("  RESULT: %s @ %.2f units\n\n",
  //        best ? best->classname.c_str() : "NONE", best_dist);

  return best;
};