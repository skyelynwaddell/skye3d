// cs_packets.nut
player_id <- 0;     // multiplayer player id in server
total_players <- 0;   // total players in current lobby
connected <- false;   // if is connected to a server
is_connecting <- false; // if is currently connecting to a server
pending_callbacks <- {}; // contains all callbacks awaiting to be called

// table to map packet names to functions
packet_funcs <- {

	// error Message
	error_msg = function (packet) {
		print("error: " + packet.value + "\n");
	}
};

/*
send_request
Sends basically a post request to the server by name, and value type,
the callback and packet is the response from the server
*/
function send_request(client_id, packet_name, value, type, callback) {
	pending_callbacks[packet_name] <- callback;

	if (type == PACKET_TYPE.NUMBER) {
		send_packet_number(client_id, packet_name, value);
	} else if (type == PACKET_TYPE.VECTOR3) {
		send_packet_vector3(client_id, packet_name, value);
	}

};

// ### SENDFLAGS ###
sendflags <- SENDFLAG_NONE;

/*
sendflags_has
*/
function sendflags_has(flag) {
	return (sendflags & flag) != 0;
};

/*
sendflags_set
*/
function sendflags_set(flag) {
    if (sendflags_has(flag)) return;

    sendflags = sendflags | flag;
    sendflags_sync();
};

/*
sendflags_unset
*/
function sendflags_unset(flag) {
    if (sendflags_has(flag) == false) return;

    sendflags = sendflags & ~flag;
    sendflags_sync();
};

/*
sendflag
call every update to have sendflag bound to conditions
flag - the flag to enable/disable
enable_condition - what condition will set the flag
disable_condition - what condition will unset the flag
callback - function that happens ONCE when the flag is set
*/
function sendflag(flag, enable_condition, disable_condition, callback)
{
if (enable_condition){
		sendflags_set(flag);
		callback();
	} else if (disable_condition){
		sendflags_unset(flag);
	}
};

/*
sendflags_sync
Syncs the clients sendflags with server
*/
function sendflags_sync() {
	send_request(player_id, "sendflags_sync", sendflags, PACKET_TYPE.NUMBER, function (packet) {});
};


/*
join_game
Joins a server
The server & ip are defined in 'settings.cfg'
*/
function join_game() {
	// If we are already connected or currently trying, don't do anything
	if (connected || is_connecting) {
		return;
	}

	print("attempting to join...\n");
	is_connecting = true;

	if (connect_to_server()) {
		connected = true;
		is_connecting = false;

		send_request(-1, "request_join", -1, PACKET_TYPE.NUMBER, function (packet) {
			player_id = packet.value;
		});
	} else {
		// Failed this frame, reset so we can try again next frame
		is_connecting = false;
		print("connection failed, retrying...\n");
	}
};

/*
is_connected
returns true if we are succesfully currently connected to a server
*/
function is_connected() {
	return (connected && !is_connecting);
};


/*
poll_packets
Polls packets from the server
*/
function poll_packets() {
	join_game(); // if we arent connected -> connect first before polling packets

	local packet = get_packet();
	while (packet != null) {

		// check if this is a response to a pending request
		if (packet.name in pending_callbacks) {
			pending_callbacks[packet.name](packet);
			delete pending_callbacks[packet.name]; // Clear it after use
		}
		// else check standard global handlers
		else if (packet.name in packet_funcs) {
			packet_funcs[packet.name](packet);
		}
		packet = get_packet();
	}

		if (!is_connected()) 
		return false;
		else return true;
};

// Mandatory Network Events

/*
get_position
Get the position of YOUR player character
*/
function get_position() {
	local pos;
	send_request(player_id, "get_position", -1, PACKET_TYPE.VECTOR3, function (packet) {
		pos = packet.value;
		print("position x: " + pos.x + " \n");
		print("position y: " + pos.y + " \n");
		print("position z: " + pos.z + " \n");
	});
	return pos
};

/*
set_position
Sets the position of YOUR player character
*/
function set_position() {
	send_request(player_id, "set_position", player_position, PACKET_TYPE.VECTOR3, function (packet) {});
};

/*
get_player_count
Gets the player count of the current connected server
*/
function get_player_count() {
	send_request(-1, "get_player_count", -1, PACKET_TYPE.NUMBER, function (packet) {
		total_players = packet.value;
		print("total player count: " + total_players + "\n");
	});
	return total_players
};