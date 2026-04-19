// sv_update.nut
function update(dt) {
	if (!poll_packets()) return;
	player_update();
};