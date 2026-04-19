-- cl_door_test.lua (or similar client-side script)
function func_door_client_think(self, dt)
  local pos = self:get_position()
  -- pos.y = pos.y - 1.0
  -- self:set_position(pos.x, pos.y, pos.z)
  -- print(pos.y)
end

-- In your spawner
function func_door(self, origin, tags)
  self:set_think(func_door_client_think) -- This hits g_server_thinks
  -- self:set_client_think(func_door_client_think) -- Does this exist?
end
