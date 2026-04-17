#include "engine.h"
#include <camera3d.h>
#include <bsp.h>
#include <rlImGui.h>

void CleanUp()
{
  UnloadShader(shader);
  UnloadShader(pp_bloom_extract);
  UnloadShader(pp_blur);
  UnloadShader(pp_composite);
  PostProcess_DestroyFBOs();
  BSP_CleanUp();
  rlImGuiShutdown();
};