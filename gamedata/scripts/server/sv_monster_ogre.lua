function monster_ogre_think(self, dt)
end

function monster_ogre(self, origin, tags)
  self:set_think(monster_ogre_think)

  self:set_model("gamedata/models/monster_shotgunner/monster_shotgunner_nowep.glb")

  self:set_size(
    { 1.9, 2.6, 1.9 },
    { 0.0, 0.0, 0.0 }
  )

  -- print("monster_ogre spawned\n")
end
