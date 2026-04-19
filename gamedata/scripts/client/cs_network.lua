-- cs_network.lua

player_id         = 0     -- multiplayer player id in server
total_players     = 0     -- total players in current lobby
connected         = false -- if is connected to a server
is_connecting     = false -- if is currently connecting to a server

-- Tables containing pending callbacks and packet-name → handler functions.
pending_callbacks = {}

-- Map packet names → handler functions
packet_funcs      = {
  -- error message
  error_msg = function(packet)
    print("error: " .. tostring(packet.value))
  end,
}

--[[
send_request
Sends a request to the server by packet name + value type, and registers a
callback to run when the matching response packet comes back.
]]
function send_request(client_id, packet_name, value, type, callback)
  pending_callbacks[packet_name] = callback

  if type == PACKET_TYPE.NUMBER then
    send_packet_number(client_id, packet_name, value)
  elseif type == PACKET_TYPE.VECTOR3 then
    send_packet_vector3(client_id, packet_name, value)
  end
end

-- ### SENDFLAGS ###
sendflags = SENDFLAG_NONE

-- sendflags_has
function sendflags_has(flag)
  return (sendflags & flag) ~= 0
end

-- sendflags_set
function sendflags_set(flag)
  if sendflags_has(flag) then return end
  sendflags = sendflags | flag
  l_sendflags_sync()
end

-- sendflags_unset
function sendflags_unset(flag)
  if not sendflags_has(flag) then return end
  sendflags = sendflags & (~flag)
  l_sendflags_sync()
end

--[[
sendflag
Call every update to have a sendflag bound to conditions.
  flag              - the flag to enable/disable
  enable_condition  - when true, sets the flag and calls callback
  disable_condition - when true, unsets the flag
  callback          - function that fires ONCE when the flag is set
]]
function sendflag(flag, enable_condition, disable_condition, callback)
  if enable_condition then
    sendflags_set(flag)
    callback()
  elseif disable_condition then
    sendflags_unset(flag)
  end
end

--[[
sendflags_sync
Syncs the client's sendflags with the server.
]]
function l_sendflags_sync()
  send_request(player_id, "on_sendflags_sync", sendflags, PACKET_TYPE.NUMBER,
    function(packet) end)
end

--[[
join_game
Joins a server. The server/ip are defined in 'settings.cfg'.
]]
function join_game()
  -- If we are already connected or currently trying, don't do anything
  if connected or is_connecting then
    return
  end

  print("attempting to join...")
  is_connecting = true

  if connect_to_server() then
    connected = true
    is_connecting = false

    send_request(-1, "request_join", -1, PACKET_TYPE.NUMBER, function(packet)
      player_id = packet.value
    end)
  else
    -- Failed this frame, reset so we can try again next frame
    is_connecting = false
    print("connection failed, retrying...")
  end
end

--[[
is_connected
Returns true if we are successfully currently connected to a server.
]]
function is_connected()
  return connected and not is_connecting
end

--[[
poll_packets
Polls packets from the server and dispatches them to the appropriate handler.
]]
function poll_packets()
  join_game() -- if we aren't connected → connect first before polling packets

  local packet = get_packet()
  while packet ~= nil do
    -- check if this is a response to a pending request
    if pending_callbacks[packet.name] ~= nil then
      pending_callbacks[packet.name](packet)
      pending_callbacks[packet.name] = nil -- clear it after use
      -- else check standard global handlers
    elseif packet_funcs[packet.name] ~= nil then
      packet_funcs[packet.name](packet)
    end
    packet = get_packet()
  end
end

-- Mandatory Network Events

--[[
get_position
Get the position of YOUR player character.
NOTE: like the Squirrel version, this returns before the callback fires, so
the returned value is typically nil. The callback prints the actual position.
]]
function get_position()
  local pos
  send_request(player_id, "get_position", -1, PACKET_TYPE.VECTOR3,
    function(packet)
      pos = packet.value
      print("position x: " .. pos.x)
      print("position y: " .. pos.y)
      print("position z: " .. pos.z)
    end)
  return pos
end

--[[
set_position
Sets the position of YOUR player character.
]]
function set_position()
  send_request(player_id, "set_position", player_position, PACKET_TYPE.VECTOR3,
    function(packet) end)
end

--[[
cs_get_player_count
Gets the player count of the currently connected server.
]]
function cs_get_player_count()
  print("calling cs_get_player_count")
  send_request(player_id, "cs_get_player_count", -1, PACKET_TYPE.NUMBER,
    function(packet)
      total_players = packet.value
      print("total player count: " .. tostring(total_players))
    end)
end
