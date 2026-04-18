function func_door_think(){
  // Init
  if (!this.get("spawned")){
    this.set("open", false);
    this.set("spawned", true);

  }

  // Update
  if (true)
  {
    this.set("open", true);
  }
  
  if (this.get("open")){
    local pos = this.get_position();
    pos.y -= 1;
    this.set_position(pos.x,pos.y,pos.z);
  }
};
