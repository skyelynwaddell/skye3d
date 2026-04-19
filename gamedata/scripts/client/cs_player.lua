-- cs_player.lua

-- state machine
STATE = {
  IDLE     = 0,
  MOVING   = 1,
  JUMP     = 2,
  FALL     = 3,
  SWIMMING = 4,
}
state = STATE.IDLE

function change_state(new_state)
  state = new_state
end


-- RELOAD
function reload()
  print("reloading")
end
function can_reload()
  sendflag(SENDFLAG_RELOAD, button_reload_pressed(), button_reload_released(), reload)
end


-- SHOOT
function shoot()
  print("shoot")
end
function can_shoot()
  sendflag(SENDFLAG_SHOOT, button_shoot_pressed(), button_shoot_released(), shoot)
end


-- INTERACT
function interact()
  print("i have interacted with something")
end
function can_interact()
  sendflag(SENDFLAG_INTERACT, button_interact_pressed(), button_interact_released(), interact)
end


--[[
player_update
Main player update function.
]]
function player_update()
  can_shoot()
  can_reload()
  can_interact()
end
