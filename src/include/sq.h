// sq.h
#pragma once
#include <squirrel.h>
#include <string>
#include <unordered_map>
#include "fonts.h"

inline HSQUIRRELVM client_vm = nullptr;
inline HSQUIRRELVM server_vm = nullptr;

enum VM_TYPE
{
  CLIENT,
  SERVER,
  SHARED
};

// functions callable via squirrel .nut files

// [server] table
// server side [start_server()]
SQInteger sq_start_server(HSQUIRRELVM vm);
SQInteger sq_get_player_count(HSQUIRRELVM vm);

// global instance
SQInteger sq_instance_create(HSQUIRRELVM vm);
SQInteger sq_instances_get_all(HSQUIRRELVM vm);
SQInteger sq_instances_get(HSQUIRRELVM vm);
SQInteger sq_get_player_instance(HSQUIRRELVM vm);
SQInteger sq_get_instance_count(HSQUIRRELVM vm);

// [gameobject] table
// instance sub functions [obj.get_position()]
SQInteger sq_instance_destroy(HSQUIRRELVM vm);
SQInteger sq_instance_is_valid(HSQUIRRELVM vm);
SQInteger sq_instance_get_position(HSQUIRRELVM vm);
SQInteger sq_instance_set_position(HSQUIRRELVM vm);
SQInteger sq_instance_get_velocity(HSQUIRRELVM vm);
SQInteger sq_instance_set_velocity(HSQUIRRELVM vm);
SQInteger sq_instance_get_collision_box(HSQUIRRELVM vm);
SQInteger sq_instance_set_collision_box(HSQUIRRELVM vm);
SQInteger sq_instance_get_size(HSQUIRRELVM vm);
SQInteger sq_instance_set_size(HSQUIRRELVM vm);
SQInteger sq_instance_get_speed(HSQUIRRELVM vm);
SQInteger sq_instance_set_speed(HSQUIRRELVM vm);
SQInteger sq_instance_get_acceleration(HSQUIRRELVM vm);
SQInteger sq_instance_set_acceleration(HSQUIRRELVM vm);
SQInteger sq_instance_get_classname(HSQUIRRELVM vm);
SQInteger sq_instance_set_classname(HSQUIRRELVM vm);
SQInteger sq_instance_get_target_name(HSQUIRRELVM vm);
SQInteger sq_instance_set_target_name(HSQUIRRELVM vm);
SQInteger sq_instance_get_target(HSQUIRRELVM vm);
SQInteger sq_instance_set_target(HSQUIRRELVM vm);
SQInteger sq_instance_set_var(HSQUIRRELVM vm);
SQInteger sq_instance_get_var(HSQUIRRELVM vm);
SQInteger sq_instance_print(HSQUIRRELVM vm);
SQInteger sq_instance_set_think(HSQUIRRELVM vm);

SQInteger sq_instance_get(HSQUIRRELVM vm); // special for funcs to run on objs

// global to all - client & server side (no prefix needed)

// networking
SQInteger sq_connect_to_server(HSQUIRRELVM vm);
SQInteger sq_sendflags_sync(HSQUIRRELVM vm);
SQInteger sq_send_packet_number(HSQUIRRELVM vm);
SQInteger sq_send_packet_vector3(HSQUIRRELVM vm);
SQInteger sq_get_packet(HSQUIRRELVM vm);

// input keys
SQInteger sq_is_key_pressed(HSQUIRRELVM vm);
SQInteger sq_is_key_down(HSQUIRRELVM vm);
SQInteger sq_is_key_released(HSQUIRRELVM vm);
SQInteger sq_is_key_up(HSQUIRRELVM vm);

// input mouse
SQInteger sq_is_mouse_button_pressed(HSQUIRRELVM vm);
SQInteger sq_is_mouse_button_down(HSQUIRRELVM vm);
SQInteger sq_is_mouse_button_released(HSQUIRRELVM vm);
SQInteger sq_is_mouse_button_up(HSQUIRRELVM vm);
SQInteger sq_get_mouse_x(HSQUIRRELVM vm);
SQInteger sq_get_mouse_y(HSQUIRRELVM vm);
SQInteger sq_get_mouse_delta(HSQUIRRELVM vm);
SQInteger sq_get_mouse_wheel_move(HSQUIRRELVM vm);
SQInteger sq_set_mouse_cursor(HSQUIRRELVM vm);

// gamepad
SQInteger sq_is_gamepad_available(HSQUIRRELVM vm);
SQInteger sq_get_gamepad_name(HSQUIRRELVM vm);
SQInteger sq_is_gamepad_button_pressed(HSQUIRRELVM vm);
SQInteger sq_is_gamepad_button_down(HSQUIRRELVM vm);
SQInteger sq_is_gamepad_button_released(HSQUIRRELVM vm);
SQInteger sq_is_gamepad_button_up(HSQUIRRELVM vm);
SQInteger sq_get_gamepad_button_pressed(HSQUIRRELVM vm);
SQInteger sq_get_gamepad_axis_count(HSQUIRRELVM vm);
SQInteger sq_get_gamepad_axis_movement(HSQUIRRELVM vm);
SQInteger sq_set_gamepad_vibration(HSQUIRRELVM vm);

// objects
SQInteger sq_get_camera_position(HSQUIRRELVM vm);

// window related funcs
SQInteger sq_toggle_fullscreen(HSQUIRRELVM vm);
SQInteger sq_toggle_borderless_windowed(HSQUIRRELVM vm);
SQInteger sq_maximize_window(HSQUIRRELVM vm);
SQInteger sq_minimize_window(HSQUIRRELVM vm);
SQInteger sq_restore_window(HSQUIRRELVM vm);
SQInteger sq_set_window_icon(HSQUIRRELVM vm);
SQInteger sq_set_window_title(HSQUIRRELVM vm);
SQInteger sq_set_window_position(HSQUIRRELVM vm);
SQInteger sq_get_screen_width(HSQUIRRELVM vm);
SQInteger sq_get_screen_height(HSQUIRRELVM vm);
SQInteger sq_get_render_width(HSQUIRRELVM vm);
SQInteger sq_get_render_height(HSQUIRRELVM vm);

// cursor funcs
SQInteger sq_show_cursor(HSQUIRRELVM vm);
SQInteger sq_hide_cursor(HSQUIRRELVM vm);
SQInteger sq_is_cursor_hidden(HSQUIRRELVM vm);
SQInteger sq_enable_cursor(HSQUIRRELVM vm);
SQInteger sq_disable_cursor(HSQUIRRELVM vm);
SQInteger sq_is_cursor_on_screen(HSQUIRRELVM vm);

// drawing
SQInteger sq_draw_fps(HSQUIRRELVM vm);
SQInteger sq_clear_background(HSQUIRRELVM vm);
SQInteger sq_begin_scissor_mode(HSQUIRRELVM vm);
SQInteger sq_end_scissor_mode(HSQUIRRELVM vm);

// timing
SQInteger sq_set_target_fps(HSQUIRRELVM vm);
SQInteger sq_get_frame_time(HSQUIRRELVM vm);
SQInteger sq_get_time(HSQUIRRELVM vm);
SQInteger sq_get_fps(HSQUIRRELVM vm);

// rng
SQInteger sq_set_random_seed(HSQUIRRELVM vm);
SQInteger sq_get_random_value(HSQUIRRELVM vm);

// rtext
SQInteger sq_load_font(HSQUIRRELVM vm);
SQInteger sq_draw_text(HSQUIRRELVM vm);

// rshapes
SQInteger sq_draw_rectangle(HSQUIRRELVM vm);

// rmodels
SQInteger sq_draw_cube(HSQUIRRELVM vm);

void sqCallEntitySpawner(const std::string &classname, Vector3 origin, const std::unordered_map<std::string, std::string> &tags);
void sqSpawnBSPEntities();

void sqRunFunc(HSQUIRRELVM vm, const char *func_name, float dt = -1.0f);
void sqPrintFunc_Server(HSQUIRRELVM /*vm*/, const SQChar *s, ...);
void sqPrintFunc_Client(HSQUIRRELVM /*vm*/, const SQChar *s, ...);
void sqErrorFunc(HSQUIRRELVM /*vm*/, const SQChar *s, ...);
void sqCompilerErrorHandler(HSQUIRRELVM /*v*/, const SQChar *desc, const SQChar *source, SQInteger line, SQInteger column);
void sqLoadNutFile(HSQUIRRELVM vm, std::string nut_filename, VM_TYPE vm_type);
void sqLoadShared(HSQUIRRELVM vm);
void sqLoadManifest(const std::string &manifestName, VM_TYPE vm_type);
void sqStartServer();
void sqStartClient();
void sqUpdate(float dt);
void sqDraw(float dt);
void sqDrawGUI(float dt);