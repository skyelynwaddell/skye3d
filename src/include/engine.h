#pragma once
inline int LIGHT_COUNT = 4;
inline int my_local_player_id = -1;

inline int SCREEN_WIDTH = 1920;
inline int SCREEN_HEIGHT = 1080;
inline int RENDER_WIDTH = 1920;
inline int RENDER_HEIGHT = 1080;
inline float GUI_WIDTH = 1920;
inline float GUI_HEIGHT = 1080;
inline float global_guiscale = 2.0f;

void Init();
void Update();
void Draw();
void DrawGUI();
void CleanUp();