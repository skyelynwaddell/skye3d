// #pragma once
// #include "skyeui.h"
// #include "global.h"

// static inline int MENUS_OPEN = 0;
// static inline int MENU_HIDDEN = 0;
// static inline int MENU_PAUSE = 1 << 0;
// static inline int MENU_OPTIONS = 1 << 1;

// inline void CloseOpenPauseMenu()
// {
//   IsMenuMode = !IsMenuMode;
//   IsMenuMode ? EnableCursor() : DisableCursor();
//   if (IsMenuMode)
//   {
//     SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
//     MENUS_OPEN = MENU_PAUSE;
//     return;
//   }
//   MENUS_OPEN = 0;
// };

// // ============================================================
// //  Pause menu  — pure button list
// // ============================================================
// inline std::vector<MenuItem> pause_menu_items =
//     {
//         {.type = MENUITEMTYPE_BUTTON, .text = "Resume Game", .onpress = []
//                                                              {
//                                                                printf("resume\n");
//                                                                CloseOpenPauseMenu();
//                                                              }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "Save Game", .onpress = []
//                                                            { printf("save\n"); }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "Load Game", .onpress = []
//                                                            { printf("load\n"); }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "New Game", .onpress = []
//                                                           { printf("new game\n"); }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "Achievements", .onpress = []
//                                                               { printf("achievements\n"); }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "Options", .onpress = []
//                                                          {
//                                                            printf("options\n");
//                                                            MENUS_OPEN ^= MENU_OPTIONS;
//                                                          }},
//         {.type = MENUITEMTYPE_BUTTON, .text = "Quit", .onpress = []
//                                                       { printf("quit\n"); }},
// };

// inline void UpdateInputMode()
// {
//   if (IsKeyPressed(KEY_ESCAPE))
//     CloseOpenPauseMenu();
// };

// // ============================================================
// //  HUD_DrawGUI  — called from SkyeUI_DrawGUI() every frame
// // ============================================================
// inline void HUD_DrawGUI()
// {
//   // ----------------------------------------------------------
//   // Options state — static so values survive across frames.
//   // Wire MenuItem.value_* at these to connect controls.
//   // ----------------------------------------------------------
//   static float music_vol = 0.75f;
//   static float sfx_vol = 0.80f;
//   static float brightness = 0.50f;
//   static float load_pct = 0.60f; // read-only progress example
//   static bool fullscreen = false;
//   static bool vsync = true;
//   static bool subtitles = false;
//   static int quality = 1;   // 0=Low 1=Medium 2=High
//   static int res_index = 0; // 0=1280x720 1=1920x1080 2=2560x1440
//   static int fov = 90;
//   static char player_name[64] = "Player";

//   // ----------------------------------------------------------
//   // Options items — one of each control type as a reference.
//   // Initialised once (static), so _edit state persists.
//   // ----------------------------------------------------------
//   static std::vector<MenuItem> options_items =
//       {
//           // ---- Audio ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Audio"},
//           {.type = MENUITEMTYPE_SLIDERBAR, .text = "Music", .value_f = &music_vol, .min_f = 0.f, .max_f = 1.f},
//           {.type = MENUITEMTYPE_SLIDERBAR, .text = "SFX", .value_f = &sfx_vol, .min_f = 0.f, .max_f = 1.f},

//           // ---- Video ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Video"},
//           {.type = MENUITEMTYPE_SLIDER, .text = "Brightness", .value_f = &brightness, .min_f = 0.f, .max_f = 1.f},
//           {.type = MENUITEMTYPE_CHECKBOX, .text = "Fullscreen", .value_b = &fullscreen},
//           {.type = MENUITEMTYPE_TOGGLE, .text = "VSync", .value_b = &vsync},
//           {.type = MENUITEMTYPE_DROPDOWN, .text = "Low;Medium;High", .value_i = &quality},
//           {.type = MENUITEMTYPE_DROPDOWN,
//            .text = "1280x720;1920x1080;2560x1440",
//            .value_i = &res_index},
//           {.type = MENUITEMTYPE_SPINNER, .text = "FOV", .value_i = &fov, .min_i = 60, .max_i = 120},

//           // ---- Accessibility ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Accessibility"},
//           {.type = MENUITEMTYPE_CHECKBOX, .text = "Subtitles", .value_b = &subtitles},
//           // ---- Accessibility ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Accessibility"},
//           {.type = MENUITEMTYPE_CHECKBOX, .text = "Subtitles", .value_b = &subtitles},
//           // ---- Accessibility ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Accessibility"},
//           {.type = MENUITEMTYPE_CHECKBOX, .text = "Subtitles", .value_b = &subtitles},
//           // ---- Accessibility ----
//           {.type = MENUITEMTYPE_SEPARATOR, .text = "Accessibility"},
//           {.type = MENUITEMTYPE_CHECKBOX, .text = "Subtitles", .value_b = &subtitles},

//           // ---- Player ----
//           {.type = MENUITEMTYPE_SEPARATOR},
//           {.type = MENUITEMTYPE_TEXTBOX, .text = "Name", .value_s = player_name, .buf_size = 64},
//           // {.type = MENUITEMTYPE_PROGRESSBAR, .text = "Loading", .value_f = &load_pct, .min_f = 0.f, .max_f = 1.f},
//       };

//   // ----------------------------------------------------------
//   // Draw  (x, y, panel_width, item_row_height)
//   // ----------------------------------------------------------

//   static PanelState pause_panel = {.x = 24, .y = 24, .w = 180};
//   static PanelState options_panel = {.x = 220, .y = 24, .w = 240, .h = 300};

//   // UpdateInputMode();

//   if (MENUS_OPEN & MENU_PAUSE)
//     if (!SkyeUI_Panel("Pause", pause_menu_items, pause_panel, 28))
//       CloseOpenPauseMenu(); // X button = same as Resume / Escape

//   if (MENUS_OPEN & MENU_OPTIONS)
//     if (!SkyeUI_Panel("Options", options_items, options_panel, 26))
//       MENUS_OPEN &= ~MENU_OPTIONS;
// }
