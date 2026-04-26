#pragma once
#include <string>

// networking props
inline std::string global_window_title = "skye3d";
inline std::string global_server_ip = "127.0.0.1";
inline unsigned short global_server_port = 1234;
inline bool global_is_hosting = true;
inline bool global_client_init_called = false;
inline std::string global_map_to_load = "";

// camera props
inline float global_cam_sensitivity = 0.003f;
inline float global_cam_yaw = 0.0f;
inline float global_cam_pitch = 0.0f;
inline float global_cam_pitch_max = 1.5f;
inline float global_analog_sens = 1000.0f;

inline bool global_show_collisions = true;
inline bool global_draw_fps = false;
inline float global_fps_x = 0.0f;
inline float global_fps_y = 0.0f;
inline bool IsMenuMode = false;

inline int brush_sync_id = 100000;
inline int spawned_instance_id = 200000;

// RL_TEXTURE_FILTER_POINT = 0,        // No filter, just pixel approximation
// RL_TEXTURE_FILTER_BILINEAR = 1,         // Linear filtering
// RL_TEXTURE_FILTER_TRILINEAR = 2,        // Trilinear filtering (linear with mipmaps)
// RL_TEXTURE_FILTER_ANISOTROPIC_4X = 3,   // Anisotropic filtering 4x
// RL_TEXTURE_FILTER_ANISOTROPIC_8X = 4,   // Anisotropic filtering 8x
// RL_TEXTURE_FILTER_ANISOTROPIC_16X = 5,  // Anisotropic filtering 16x
inline int global_texture_filter = 2;
