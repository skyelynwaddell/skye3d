

#include "raylib.h"
// @REF https://github.com/raysan5/raylib/blob/master/examples/shaders/rlights.h
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#include "engine.h"

// namespace ImGui
// {
// 	ImGuiWindowFlags
// 	SetNextWindowOverlay()
// 	{
// 		const ImGuiViewport* viewport = GetMainViewport();
// 		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
// 		ImVec2 work_size = viewport->WorkSize;
// 		const float PAD = 10.0f;
// 		ImVec2 window_pos = {work_pos.x + PAD, work_pos.y + PAD};
// 		SetNextWindowPos(window_pos, ImGuiCond_Always);

// 		SetNextWindowBgAlpha(0.8f); // Transparent background
// 		return ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
// 	}
// };

int main()
{
	Init();

	while (!WindowShouldClose())
	{
		Update();
		Draw();
		DrawGUI();
	}

	CleanUp();
	CloseWindow();
	return 0;
};