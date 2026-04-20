-- sv_update.lua

function update(dt)
  if not poll_packets() then return end
  player_update()
end
