-- sv_func_door.lua

-- func_door_think - called every frame from entities_update
-- 'self' = the GameObject3D for this door
function func_door_think(self, dt)
  local pos = self:get_position()
  pos.y = pos.y - 0.01
  self:set_position(pos.x, pos.y, pos.z)
  -- print("posx: ", pos.x);
  -- print("posy: ", pos.y);
  -- print("posz: ", pos.z);
end

-- func_door spawner - called once at BSP map load
-- 'self'   = the newly created GameObject3D for this entity
-- 'origin' = {x, y, z} table with the BSP entity position
-- 'tags'   = table of all key/value pairs from the BSP entity
function func_door(self, origin, tags)
  -- self:set("open", false)
  self:set_think(func_door_think)
  print("func_door spawned\n")
end
