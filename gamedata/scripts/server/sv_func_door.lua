-- sv_func_door.lua

function func_door_think(self, dt)
  local is_open = self:get("open")

  if is_open then
    -- opening
    local pos = self:get_position()
    pos.y = pos.y - 0.01
    self:set_position(pos.x, pos.y, pos.z)
  else
    -- closing
  end
end

function func_door_trigger(self)
  self:set("open", true)
  print("func_door_trigger!!\n")
end

function func_door(self, origin, tags)
  self:set("open", false)
  self:set_think(func_door_think)
  self:set_trigger(func_door_trigger)
  print("func_door spawned\n")
end
