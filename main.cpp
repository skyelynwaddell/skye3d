

#include "raylib.h"
// @REF https://github.com/raysan5/raylib/blob/master/examples/shaders/rlights.h
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#include "engine.h"

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