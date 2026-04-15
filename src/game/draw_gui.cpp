#include "engine.h"
#include <camera3d.h>
#include <sq.h>
#include <gameobject3d.h>
#include <bsp.h>

void DrawGUI()
{
  global_draw_fps = false;
  sqDrawGUI(GetFrameTime());
  GameObject3D_DrawAll();

  // if (enable_imgui)
  // {
  // 	rlImGuiBegin();
  // 	ImGuiWindowFlags overlayFlags = ImGui::SetNextWindowOverlay();
  // 	if (ImGui::Begin("Controls", nullptr, overlayFlags))
  // 	{
  // 		ImGui::Text("Drag and Drop a .BSP file onto the window to view it.");
  // 		ImGui::Text("Current File: %s", currentFile.c_str());
  // 		ImGui::Separator();

  // 		ImGui::BulletText("WASD:        Move");
  // 		ImGui::BulletText("SPACE/LCTRL: Up/Down");
  // 		ImGui::BulletText("Q/E:         Roll");
  // 		ImGui::BulletText("R:           Reset Camera Roll");
  // 		ImGui::BulletText("Mouse:       Pan");
  // 		ImGui::BulletText("I:           Toggle UI");
  // 		ImGui::BulletText("RMB:         Toggle Cursor");

  // 		if (ImGui::SliderInt("Light Power", &lightPower, 1, 50))
  // 			SetShaderValue(shader, GetShaderLocation(shader, "lightPower"), &lightPower, SHADER_UNIFORM_INT);

  // 		ImGui::Checkbox("Wireframe", &enable_wireframe);

  // 		static float line_width = rlGetLineWidth();
  // 		if (ImGui::SliderFloat("Line Width", &line_width, 0.1f, 10))
  // 			rlSetLineWidth(line_width);
  // 	}
  // 	ImGui::End();
  // 	rlImGuiEnd();
  // }

  if (global_draw_fps)
    DrawFPS(global_fps_x, global_fps_y);

  BSP_DrawDebug(camera->position);

  EndDrawing();
};