// server_handlers.h
#pragma once
#include "enet.h"
#include <functional>
#include <raymath.h>
#include <string>
#include "server_ops.h"
#include "bsp.h"   // for GO_SetPitch and other GameObject3D shims

using ServerPacketHandler = std::function<void(int sender_id, const void *payload, size_t len)>;

// request_join
inline std::unordered_map<std::string, ServerPacketHandler> float_handlers = {
    {"request_join", [](int sender_id, const void *payload, size_t len)
     {
       RequestJoin(sender_id);
     }},
    {"set_angle", [](int sender_id, const void *payload, size_t len)
     {
       float angle;
       memcpy(&angle, payload, sizeof(float));
       SetAngle(angle, sender_id);
     }},
    {"set_pitch", [](int sender_id, const void *payload, size_t len)
     {
       // Pitch in radians. Server uses it for traceline aim so interact /
       // shoot rays match what the client visually pointed at. Stored on
       // the user's object; not rebroadcast to other clients (no third
       // party currently needs another player's pitch). Goes through the
       // GO_SetPitch shim because GameObject3D is only forward-declared
       // here (full definition lives in gameobject3d.h).
       float pitch;
       memcpy(&pitch, payload, sizeof(float));
       static int dbg = 0;
       if ((dbg++ & 63) == 0)  // log every 64th packet to avoid spam
         printf("[SERVER] set_pitch from player %d -> %.3f rad (%.1f deg)\n",
                sender_id, pitch, pitch * 57.2957795f);
       if (sender_id >= 0)
         GO_SetPitch(users[sender_id].object_ref, pitch);
     }},

    // Add new float packet handlers here:
    // { "my_packet", [](int sender_id, const void* payload, size_t len) { ... } },
};

// set_position
inline std::unordered_map<std::string, ServerPacketHandler> vec3_handlers = {
    {"set_position", [](int sender_id, const void *payload, size_t len)
     {
       Vector3 vec;
       memcpy(&vec, payload, sizeof(Vector3));
       SetPosition(vec, sender_id);
     }},
    // Add new vec3 packet handlers here:
};