-- cs_init.lua
function init()
  print("\n\n ### CS INIT CALLED ###")
  cs_get_player_count()
end

-- Called by the engine entity spawner for each info_player_start entity in
-- the loaded BSP. Signature: (self, origin, tags) where self is the fresh
-- GameObject3D, origin = {x=, y=, z=}, and tags is a table of key/value
-- pairs from the BSP entity.
function info_player_start(self, origin, tags)
  print("INFO_PLAYER_SPAWN : spawned at "
        .. origin.x .. " " .. origin.y .. " " .. origin.z)
  -- tags.targetname, tags.angle, etc. all available
end
