-- sv_func_door.lua

function func_door_think(self, dt)
  local pos = self:get_position()
  pos.y = pos.y - 0.001
  self:set_position(pos.x, pos.y, pos.z)
end

function func_door(self, origin, tags)
  self:set("open", false)
  self:set_think(func_door_think)
  print("func_door spawned\n")
end
