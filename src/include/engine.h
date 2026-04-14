#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlgl.h>

#include <imgui.h>
#include <rlImGui.h>

#include <filesystem>
#include <set>
#include <vector>
#include "bsp.h"

inline int LIGHT_COUNT = 4;
inline int my_local_player_id = -1;

void Init();
void Update();
void Draw();
void DrawGUI();
void CleanUp();