#pragma once
inline int LIGHT_COUNT = 4;
inline int my_local_player_id = -1;

inline int SCREEN_WIDTH = 1280;
inline int SCREEN_HEIGHT = 720;


void Init();
void Update();
void Draw();
void DrawGUI();
void CleanUp();