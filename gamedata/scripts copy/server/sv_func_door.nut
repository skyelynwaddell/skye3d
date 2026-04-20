// func_door_think - called every frame from entities_update
// 'this' = the GameObject3D for this door
// function func_door_think() {
//     local pos = this.get_position();
//     pos.y = pos.y - 1.0;
//     this.set_position(pos.x, pos.y, pos.z);
//     print("hellooo");
// };


// func_door spawner - called once at BSP map load
// 'this' = the newly created GameObject3D for this entity
// 'origin' = {x, y, z} table with the BSP entity position
// 'tags' = table of all key/value pairs from the BSP entity
function func_door(origin, tags) {
  // this.set("open", false);
  // this.set_think(func_door_think);
  print("func_door spawned\n");
};

