-- sv_network.lua
-- table to map packet names to functions

packet_funcs = {

  -- request_join
  request_join = function(packet)
    send_packet_number(-1, "request_join", -1)
  end,

  -- set_sendflags
  on_sendflags_sync = function(packet)
    -- math.tointeger ensures the internal Lua type is shifted to an integer
    local cid = math.tointeger(packet.client_id)
    local val = math.tointeger(packet.value)

    if not cid or not val then
      print("Warning: Received non-integer network flags!")
      return
    end

    sendflags_sync(cid, val)
    local player = get_player_instance(cid)
    if not player then
      print("error: no player instance for cid " .. cid .. "\n")
      return
    end

    -- Shoot
    if (val & SENDFLAG_SHOOT) ~= 0 then
      local tr = player:traceline(2048)
      if tr then
        if tr.hit_type == "world" then
          -- hit solid
          print("shot world\n")
        elseif tr.hit_type == "brush_entity" then
          -- hit brush ent
          print("shot brush ent\n")
        elseif tr.hit_type == "object" then
          -- hit gameobject
          print("shot gameobject\n")
        end
      end
    end

    -- Interact
    if (val & SENDFLAG_INTERACT) ~= 0 then
      local tr = player:traceline(3)
      if tr then
        if tr.hit_type == "world" then
          -- hit solid
          print("interacted w/ world\n")
        elseif tr.hit_type == "brush_entity" then
          -- hit brush ent

          if tr.hit_object:get_classname() == "func_door" then
            tr.hit_object:on_trigger()
          end

          print("interacted w/ brush ent\n")
        elseif tr.hit_type == "object" then
          -- hit gameobject
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
