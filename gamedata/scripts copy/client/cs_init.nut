// cs_init.nut
function init() {
	print("\n\n ### CS INIT CALLED ###\n");
	cs_get_player_count();
};

function info_player_start(origin, tags)
{
	print("INFO_PLAYER_SPAWN : spawned at " + origin.x + " " + origin.y + " " + origin.z + "\n")
    // tags.targetname, tags.angle, etc. all available
};