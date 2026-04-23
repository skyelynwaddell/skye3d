-- sv_func_door.lua

-- sv_func_door.lua

function func_door_think(self, dt)
  local is_open = self:get("open")
  local pos = self:get_position()
  local aabb = self:get_aabb() -- This returns Raylib (Y-up) bounds

  local angle = self:get("angle")
  local lip = self:get("lip")

  local size_x = aabb.max.x - aabb.min.x
  local size_y = aabb.max.y - aabb.min.y
  local size_z = aabb.max.z - aabb.min.z

  local dir = { x = 0, y = 0, z = 0 }
  local travel_dist = 0

  -- 1. Determine direction based on your Raylib axis mapping
  if angle == -1 then -- Quake Up (Z+) -> Raylib Up (Y+)
    dir.y = 1
    travel_dist = size_y - lip
  elseif angle == -2 then -- Quake Down (Z-) -> Raylib Down (Y-)
    dir.y = -1
    travel_dist = size_y - lip
  else
    -- Horizontal movement (Quake X/Y -> Raylib Z/X)
    local rad = math.rad(angle)
    -- Quake X is Raylib Z, Quake Y is Raylib X
    dir.z = math.cos(rad)
    dir.x = math.sin(rad)

    -- Determine travel distance based on the dominant horizontal axis
    if math.abs(dir.x) > math.abs(dir.z) then
      travel_dist = size_x - lip
    else
      travel_dist = size_z - lip
    end
  end

  -- 2. Define target relative to original spawn position
  local og = { x = self:get("ogx"), y = self:get("ogy"), z = self:get("ogz") }
  local target = {
    x = og.x + (dir.x * travel_dist),
    y = og.y + (dir.y * travel_dist),
    z = og.z + (dir.z * travel_dist)
  }

  local speed = 5 -- Suggestion: use 100 * 0.03 if you're using scale
  local step = speed * dt

  -- 3. Interpolate position
  local function move(current, start, dest)
    if is_open then
      if start < dest then return math.min(current + step, dest) end
      if start > dest then return math.max(current - step, dest) end
    else
      if current > start then return math.max(current - step, start) end
      if current < start then return math.min(current + step, start) end
    end
    return current
  end

  pos.x = move(pos.x, og.x, target.x)
  pos.y = move(pos.y, og.y, target.y)
  pos.z = move(pos.z, og.z, target.z)

  self:set_position(pos.x, pos.y, pos.z)
end

function func_door_trigger(self)
  local is_open = self:get("open")
  is_open = not is_open
  self:set("open", is_open)
  print("func_door_trigger!!\n")
end

function func_door(self, origin, tags)
  self:set("open", false)

  self:set("ogx", origin.x)
  self:set("ogy", origin.y)
  self:set("ogz", origin.z)
  self:set("angle", tonumber(tags.angle) or 90)
  self:set("lip", (tonumber(tags.lip) or 4) * 0.03)

  self:set_think(func_door_think)
  self:set_trigger(func_door_trigger)
  -- print("func_door spawned\n")
end
