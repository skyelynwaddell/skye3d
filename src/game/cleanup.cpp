#include "engine.h"
#include <camera3d.h>
#include <bsp.h>
#include <rlImGui.h>

void CleanUp()
{
  UnloadShader(shader);
  BSP_CleanUp();
  rlImGuiShutdown();
};