-- sv_init.lua
function init()
  print("\n\n### SV INIT CALLED ###\n")
  start_server()

  -- -- create a dummy "torch" object
  local obj = instance_create(0, 0, 0)
  obj:set_target_name("torch")
  obj:set_position(83, 20, 74)

  -- create some test vars
  obj:set("name", "skye")
  obj:set("health", 100)
  obj:set("speed", 5.3)
  obj:set("eliminated", false)

  -- print pos for proof
  local pos = obj:get_position()
  print("Object Position: x=" .. pos.x .. " y=" .. pos.y .. " z=" .. pos.z .. "\n")

  -- find the dummy torch object later
  local torches = instances_get("torch")
  for i, torch in ipairs(torches) do
    -- print the found torch pos
    local pos = torch:get_position()
    print("found torch at: " .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. "\n\n")

    -- get the vars we set above
    local name       = torch:get("name")
    local health     = torch:get("health")
    local speed      = torch:get("speed")
    local eliminated = torch:get("eliminated")

    -- print them to console
    print("name: " .. tostring(name) .. "\n")
    print("health: " .. tostring(health) .. "\n")
    print("speed: " .. tostring(speed) .. "\n")
    print("eliminated: " .. tostring(eliminated) .. "\n")
  end
end
