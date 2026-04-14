// state machine
enum STATE {
	IDLE,
	MOVING,
	JUMP,
	FALL,
	SWIMMING,
}
state <- STATE.IDLE;
function change_state(new_state){ state = new_state; };

// JUMP
function jump() {
	print("jump\n");
	change_state(STATE.JUMP);
};
function can_jump() {
	sendflag(SENDFLAG_JUMP, button_jump_pressed(), button_jump_released(), jump);
};


// RELOAD
function reload(){
	print("reloading\n");
};
function can_reload(){
	sendflag(SENDFLAG_RELOAD, button_reload_pressed(), button_reload_released(), reload);
};


// SHOOT
function shoot(){
	print("shoot\n");
};
function can_shoot(){
	sendflag(SENDFLAG_SHOOT, button_shoot_pressed(), button_shoot_released(), shoot);
};


// INTERACT
function interact(){
	print("i have interacted with something\n");
};
function can_interact(){
	sendflag(SENDFLAG_INTERACT, button_interact_pressed(), button_interact_released(), interact);
};


/*
player_update
main player update function
*/
function player_update() {
	can_shoot();
	can_reload();
	can_jump();
	can_interact();
};
