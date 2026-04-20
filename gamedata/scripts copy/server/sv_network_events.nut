// sv_network_events.nut
// Custom network events go here

// Example:
// packet_funcs.get_position <- function (packet) {
//     local me = server.get_player_instance(packet.client_id);
//
//     if (me) {
//         local pos = me.get_position();
//
//         // send response to client
//         send_packet_vector3(packet.client_id, "get_position", pos);
//     }
// };