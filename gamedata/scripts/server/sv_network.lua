-- sv_network.lua
-- table to map packet names to functions

packet_funcs = {

  -- request_join
  request_join = function(packet)
    send_packet_number(-1, "request_join", -1)
  end,

  -- set_sendflags
  --
  -- IMPORTANT: SHOOT / INTERACT must fire on the RISING EDGE of their flag,
  -- not whenever the bit happens to be set in `val`. Any unrelated flag
  -- change (movement, reload, etc.) re-sends sendflags with INTERACT still
  -- set if the player is still holding E, and a state-check fired the
  -- traceline every time — so a single press could toggle the door 2-3x
  -- and leave it in the wrong state. We keep per-player previous flags
  -- and only act when a flag goes 0 → 1.
  on_sendflags_sync = function(packet)
    local cid = math.tointeger(packet.client_id)
    local val = math.tointeger(packet.value)

    if not cid or not val then
      print("Warning: Received non-integer network flags!")
      return
    end

    -- per-player flag history; nil on first packet → previous = 0
    prev_sendflags = prev_sendflags or {}
    local prev = prev_sendflags[cid] or 0
    local pressed  = val & ~prev   -- bits that just turned on
    -- (released bits are val_changed & prev, unused for now)
    prev_sendflags[cid] = val

    sendflags_sync(cid, val)
    local player = get_player_instance(cid)
    if not player then
      print("error: no player instance for cid " .. cid .. "\n")
      return
    end

    -- Shoot — rising edge only
    if (pressed & SENDFLAG_SHOOT) ~= 0 then
      local shoot_dist = 2048
      local tr = player:traceline(shoot_dist)
      if tr then
        if tr.hit_type == "world" then
          print("shot world\n")
        elseif tr.hit_type == "brush_entity" then
          print("shot brush ent\n")
        elseif tr.hit_type == "object" then
          print("shot gameobject\n")
        end
      end
    end

    -- Interact — rising edge only
    if (pressed & SENDFLAG_INTERACT) ~= 0 then
      local interact_dist = 3
      local p_pos = player:get_position()
      local p_ang = player:get_angle()
      local p_pit = player:get_pitch()
      print(string.format("[SV] cid=%d interact: pos=(%.2f,%.2f,%.2f) yaw=%.2f pitch=%.3f",
        cid, p_pos.x, p_pos.y, p_pos.z, p_ang, p_pit))
      local tr = player:traceline(interact_dist)
      if tr then
        if tr.hit_type == "world" then
          print("interacted w/ world\n")
        elseif tr.hit_type == "brush_entity" then
          if tr.hit_object:get_classname() == "func_door" then
            tr.hit_object:on_trigger()
          end
          print("interacted w/ brush ent\n")
        elseif tr.hit_type == "object" then
          print("interacted w/ gameobject\n")
        end
      end
    end
  end,

  -- get_position
  get_position = function(packet)
    local me = get_player_instance(packet.client_id)
    local pos = me:get_position()
    send_packet_vector3(packet.client_id, "get_position", pos)
  end,

  -- set_position
  set_position = function(packet)
    local me = get_player_instance(packet.client_id)
    me:set_position(packet.value.x, packet.value.y, packet.value.z)
  end,

  -- get_player_count
  cs_get_player_count = function(packet)
    local total_players = get_player_count()
    print("total players: " .. total_players .. "\n")
    send_packet_number(packet.client_id, "cs_get_player_count", total_players)
  end,
}

function poll_packets()
  local packet = get_packet()
  while packet ~= nil do
    if packet_funcs[packet.name] ~= nil then
      packet_funcs[packet.name](packet)
    end
    packet = get_packet()
  end
  return true
end
