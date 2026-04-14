// sv_init.nut
function helloworld()
{
	print("From " + this.get("name") + ", my health: " + this.get("health") + "\n\n");
};

function init() {
	print("\n\n### SV INIT CALLED ###\n");
	server.start_server();

	// create a dummy "torch" object
	local obj = server.instance_create(0,0,0);
	obj.set_target_name("torch");
	obj.set_position(10.5, 20.0, -5.2);

	// create some test vars
	obj.set("name", "skye");
	obj.set("health", 100);
	obj.set("speed", 5.3);
	obj.set("eliminated", false);

	// print pos for proof
	local pos = obj.get_position();
	print("Object Position: x=" + pos.x + " y=" + pos.y + " z=" + pos.z + "\n");

	// find the dummy torch object later
	local torches = server.instances_get("torch");
	foreach (i, torch in torches){

		// print the found torch pos
		local pos = torch.get_position();
		print("found torch at: " + pos.x + ", " + pos.y + ", " + pos.z + "\n\n");

		// get the vars we set above
		local name = torch.get("name");
		local health = torch.get("health");
		local speed = torch.get("speed");
		local eliminated = torch.get("eliminated");

		// print them to console
		print("name: " + name + "\n");
		print("health: " + health + "\n");
		print("speed: " + speed + "\n");
		print("eliminated: " + eliminated + "\n");

		// call helloworld function on this object
		torch.helloworld();
	}

};
