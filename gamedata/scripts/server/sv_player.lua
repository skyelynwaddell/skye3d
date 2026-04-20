-- sv_player.lua
_G.showpos = true
function player_update()
  local me = get_player_instance(0)

  if me and _G.showpos then
    _G.showpos = false
    local pos = me:get_position()
    print("player spawned \n")
    print("x: " .. pos.x .. "\n")
    print("y: " .. pos.y .. "\n")
    print("z: " .. pos.z .. "\n")
  end
end
