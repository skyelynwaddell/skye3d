// server_handlers.h
#pragma once
#include "enet.h"
#include <functional>
#include <string>
#include <raylib.h>
#include "server_ops.h"

using ServerPacketHandler = std::function<void(int sender_id, const void *payload, size_t len)>;

// request_join
inline std::unordered_map<std::string, ServerPacketHandler> float_handlers = {
    {"request_join", [](int sender_id, const void *payload, size_t len)
     {
       RequestJoin(sender_id);
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