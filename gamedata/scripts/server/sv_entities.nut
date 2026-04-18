function entities_update()
{
  local objs = instances_get_all();

  foreach (obj in objs) {
    if (!obj.is_valid()) continue;
    
    local classname = obj.get_classname();
    if (classname == "func_door") obj.func_door_think();
  }
};