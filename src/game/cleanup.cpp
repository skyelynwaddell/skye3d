#include "engine.h"
#include <camera3d.h>

void CleanUp()
{
  UnloadShader(shader);
  BSP_CleanUp();
  rlImGuiShutdown();
};