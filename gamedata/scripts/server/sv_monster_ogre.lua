function monster_ogre_think(self, dt)
  local pos = self:get_position()
  pos.y = pos.y - 0.001
  self:set_position(pos.x, pos.y, pos.z)
end

function monster_ogre(self, origin, tags)
  self:set_think(monster_ogre_think)
  print("monster_ogre spawned\n")
end
