// sv_packets.nut
// table to map packet names to functions
packet_funcs <- {

	// request_join
	request_join = function (packet) {
		send_packet_number(-1, "request_join", -1);
	},

	// set_sendflags
	sendflags_sync = function (packet) {
		server.sendflags_sync(packet.client_id, packet.value);
	},

	// get_position
	get_position = function (packet) {
		local me = server.get_player_instance(packet.client_id);
		local pos = me.get_position();
		send_packet_vector3(packet.client_id, "get_position", pos);
	},

	// set_position
	set_position = function (packet) {
		local me = server.get_player_instance(packet.client_id);
		me.set_position(packet.value);
	},

	// get_player_count
	get_player_count = function (packet) {
		local total_players = server.get_player_count();
		send_packet_number(packet.client_id, "get_player_count", total_players);
	},

};

/*
poll_packets
Polls packets from the client
*/
function poll_packets() {
	local packet = get_packet();

	while (packet != null) {
		if (packet.name in packet_funcs) {
			packet_funcs[packet.name](packet); // run func inside of 'packet_funcs' table with same name as packet_name
		} else {
			print("Warning: No handler for packet '" + packet.name + "'\n");
		}

		packet = get_packet();
	}
	return true;
};
