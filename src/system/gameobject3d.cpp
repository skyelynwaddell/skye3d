#include "gameobject3d.h"

/*
  FindClosestObject
  */
GameObject3D *GameObject3D::FindClosestObject(float max_dist)
{
  GameObject3D *best = nullptr;
  float best_dist = max_dist;

  for (auto &obj : gameobjects)
  {
    if (!obj || obj.get() == this)
      continue;

    // Skip non-interactables
    if (!obj->classname.starts_with("func"))
      continue;
    // if (obj->classname.starts_with("info") ||
    //     obj->classname.starts_with("path"))
    //   continue;

    // Ask the object for its true world-space center
    Vector3 world_pos = obj->GetInteractCenter();

    float dist = Vector3Distance(world_pos, this->position);

    if (dist < best_dist)
    {
      best_dist = dist;
      best = obj.get();
    }
  }
  return best;
}