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

    -- Interact
    if (val & SENDFLAG_INTERACT) ~= 0 then
      local player = get_player_instance(cid)
      if not player then
        print("error: no player instance for cid " .. cid .. "\n")
        return
      end

      local interact_dist = 100
      local obj = player:find_closest_object(interact_dist)

      if obj then
        local name = obj:get_classname()
        local obj_pos = obj:get_position()
        print("found object " .. tostring(obj) .. " " .. name .. " \n")
        print("  Player position: " .. tostring(player:get_position()) .. "\n")
        print("  Object position: " .. tostring(obj_pos) .. "\n")
        if obj:get_classname() == "func_door" then
          obj:on_trigger()
        end
      else
        print("interact pressed, no obj found within dist " .. interact_dist .. "\n")
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

--[[
poll_packets
Polls packets from the client
]]
function poll_packets()
  local packet = get_packet()

  while packet ~= nil do
    if packet_funcs[packet.name] ~= nil then
      -- run func inside of 'packet_funcs' table with same name as packet_name
      packet_funcs[packet.name](packet)
    else
      print("Warning: No handler for packet '" .. packet.name .. "'\n")
    end

    packet = get_packet()
  end
  return true
end
