#pragma once
#include <raylib.h>
#include <unordered_map>
#include <string>

// gamemodel.h

class GameModel
{
public:
  Model model = {0};
  Vector3 scale = {1.0f, 1.0f, 1.0f};
  std::string model_path = "";
};

inline std::unordered_map<std::string, Model> model_cache;

// Character Shader
inline Shader characterShader;
inline float char_ambientStrength = 0.35f;
inline float char_specularStrength = 0.35f;
inline float char_shininess = 64.0f;

inline void CharacterShader_Init()
{
  characterShader = LoadShader("gamedata/shaders/330/model.vs",
                               "gamedata/shaders/330/model.fs");

  SetShaderValue(characterShader, GetShaderLocation(characterShader, "ambientStrength"),
                 &char_ambientStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "specularStrength"),
                 &char_specularStrength, SHADER_UNIFORM_FLOAT);
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "shininess"),
                 &char_shininess, SHADER_UNIFORM_FLOAT);
}

inline void CharacterShader_Update()
{
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "viewPos"),
                 &camera->position, SHADER_UNIFORM_VEC3);
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "lightPos"),
                 &camera->position, SHADER_UNIFORM_VEC3);

  Vector4 lightColor = {1.0f, 0.98f, 0.92f, 1.0f};
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "lightColor"),
                 &lightColor, SHADER_UNIFORM_VEC4);

  Vector3 colDiffuse = {1.0f, 1.0f, 1.0f};
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "colDiffuse"),
                 &colDiffuse, SHADER_UNIFORM_VEC3);

  Vector3 colAmbient = {0.35f, 0.4f, 0.5f};
  SetShaderValue(characterShader, GetShaderLocation(characterShader, "colAmbient"),
                 &colAmbient, SHADER_UNIFORM_VEC3);
}

inline void CharacterShader_Cleanup()
{
  UnloadShader(characterShader);
}

inline Model GetModelCached(const std::string &path)
{
  if (model_cache.find(path) == model_cache.end())
  {
    model_cache[path] = LoadModel(path.c_str());

    for (int i = 0; i < model_cache[path].materialCount; i++)
    {
      model_cache[path].materials[i].shader = characterShader;

      int texLoc = GetShaderLocation(characterShader, "texture0");
      if (texLoc != -1)
      {
        int texUnit = 0;
        SetShaderValue(characterShader, texLoc, &texUnit, SHADER_UNIFORM_INT);
      }

      SetShaderValue(characterShader, GetShaderLocation(characterShader, "ambientStrength"),
                     &char_ambientStrength, SHADER_UNIFORM_FLOAT);
      SetShaderValue(characterShader, GetShaderLocation(characterShader, "specularStrength"),
                     &char_specularStrength, SHADER_UNIFORM_FLOAT);
      SetShaderValue(characterShader, GetShaderLocation(characterShader, "shininess"),
                     &char_shininess, SHADER_UNIFORM_FLOAT);
    }
  }
  return model_cache[path];
}
