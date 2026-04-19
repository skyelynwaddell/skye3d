// net_utils.h
#pragma once
#include "global.h"
#include "bsp.h"
#include "enet.h"
#include <vector>
#include <variant>
#include <cstdint>

typedef struct PlayerSpawnData
{
  Vector3 origin = {0, 0, 0};
} PlayerSpawnData;

inline PlayerSpawnData InfoPlayerStart()
{
  PlayerSpawnData data = {{0.0f, 0.0f, 0.0f}};

  if (!bsp_renderer.bsp_file)
    return data;

  auto entities = bsp_renderer.bsp_file->entities();
  for (auto &e : entities)
  {
    auto class_it = e.tags.find("classname");
    if (class_it == e.tags.end() || class_it->second != "info_player_start")
      continue;

    auto org_it = e.tags.find("origin");
    if (org_it != e.tags.end())
    {
      float qx = 0, qy = 0, qz = 0;
      if (sscanf(org_it->second.c_str(), "%f %f %f", &qx, &qy, &qz) == 3)
      {
        data.origin = FromQuake({qx, qy, qz});
        return data;
      }
    }
  }

  return data;
}

using PacketData = std::variant<int, float, std::string, bool, Vector3>;

/*
Packet
A named packet with a value that can be sent between C++ and Squirrel.
client_id = -1 means broadcast to all.
*/
struct Packet
{
  int client_id = -1;
  std::string name = "";
  PacketData value;
};

inline std::vector<Packet> server_to_client_packets; // Server → SQ Client
inline std::vector<Packet> client_to_server_packets; // Client → SQ Server

// ============================================================
// There are 4 destinations packets can go:
//
//  1. CPP Client   — goes over ENet, received in enetclient_update()
//  2. CPP Server   — goes over ENet, received in enetserver_update()
//  3. SQ Client    — pushed to server_to_client_packets, read by cs_packets.nut
//  4. SQ Server    — pushed to client_to_server_packets, read by sv_packets.nut
// ============================================================

// -----------------------------------------------------------------------
// Buffer Builders
// Construct a raw ENet-ready buffer for a given type + name
// Returns the total size written
// -----------------------------------------------------------------------

/*
BuildFloatPacket
[MESSAGE_TYPE (1b)] [float (4b)] [name (Nb)]
*/
inline size_t BuildFloatPacket(unsigned char *buffer, const std::string &name, float value)
{
  buffer[0] = MESSAGE_TYPE_PACKET_FLOAT;
  memcpy(buffer + 1, &value, sizeof(float));
  memcpy(buffer + 1 + sizeof(float), name.c_str(), name.length() + 1);
  return 1 + sizeof(float) + name.length() + 1;
};

/*
BuildVector3Packet
[MESSAGE_TYPE (1b)] [Vector3 (12b)] [name (Nb)]
*/
inline size_t BuildVector3Packet(unsigned char *buffer, const std::string &name, Vector3 value)
{
  buffer[0] = MESSAGE_TYPE_PACKET_VECTOR3;
  memcpy(buffer + 1, &value, sizeof(Vector3));
  memcpy(buffer + 1 + sizeof(Vector3), name.c_str(), name.length() + 1);
  return 1 + sizeof(Vector3) + name.length() + 1;
};

// -----------------------------------------------------------------------
// Core ENet Dispatcher
// All raw ENet sends go through here — nowhere else touches enet_packet_create
// -----------------------------------------------------------------------

/*
NetDispatch
Sends a buffer to:
  - a specific peer  (peer != nullptr)
  - all clients except sender_id (peer == nullptr, sender_id >= 0)
  - all clients including sender  (peer == nullptr, sender_id == -1)
*/
inline void NetDispatch(const unsigned char *buffer, size_t size, ENetPeer *peer = nullptr, int sender_id = -1)
{
  if (peer)
  {
    ENetPacket *packet = enet_packet_create(buffer, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
    return;
  }

  for (int i = 0; i < client_count; i++)
  {
    if (!users[i].peer)
      continue;
    if (sender_id >= 0 && users[i].player_id == sender_id)
      continue;

    ENetPacket *packet = enet_packet_create(buffer, size, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(users[i].peer, 0, packet);
  }
};

// -----------------------------------------------------------------------
// Destination 1: CPP Client  (goes over ENet wire)
// Use when you need the C++ client to receive it in enetclient_update()
// -----------------------------------------------------------------------

/*
Net_ToCPPClient
Sends a typed packet directly to one client's C++ layer
*/
inline void Net_ToCPPClient(ENetPeer *peer, const std::string &name, float value)
{
  unsigned char buffer[256];
  size_t size = BuildFloatPacket(buffer, name, value);
  NetDispatch(buffer, size, peer);
};

inline void Net_ToCPPClient(ENetPeer *peer, const std::string &name, Vector3 value)
{
  unsigned char buffer[256];
  size_t size = BuildVector3Packet(buffer, name, value);
  NetDispatch(buffer, size, peer);
};

inline void Net_ToCPPClient(ENetPeer *peer, const std::string &name, int value)
{
  Net_ToCPPClient(peer, name, static_cast<float>(value));
};

/*
Net_ToCPPAllClients
Broadcasts a typed packet to all client C++ layers except the sender
*/
inline void Net_ToCPPAllClients(int sender_id, const std::string &name, float value)
{
  unsigned char buffer[256];
  size_t size = BuildFloatPacket(buffer, name, value);
  NetDispatch(buffer, size, nullptr, sender_id);
};

inline void Net_ToCPPAllClients(int sender_id, const std::string &name, Vector3 value)
{
  unsigned char buffer[256];
  size_t size = BuildVector3Packet(buffer, name, value);
  NetDispatch(buffer, size, nullptr, sender_id);
};

/*
Net_ToCPPServer
Broadcasts a typed packet from CPP client to the CPP server
*/
inline void Net_ToCPPServer(const std::string &name_str, float value)
{
  // Client → Server CPP (over ENet)
  unsigned char buffer[256];
  size_t size = BuildFloatPacket(buffer, name_str, (float)value);
  enetclient_send(buffer, size);
};

inline void Net_ToCPPServer(const std::string &name_str, Vector3 value)
{
  // Client → Server CPP (over ENet)
  unsigned char buffer[256];
  size_t size = BuildVector3Packet(buffer, name_str, value);
  enetclient_send(buffer, size);
};

// -----------------------------------------------------------------------
// Destination 2: SQ Client  (goes into server_to_client_packets queue)
// Use when you want cs_packets.nut to receive it via get_packet()
// -----------------------------------------------------------------------

/*
Net_ToSQClient
Pushes a packet into the Squirrel client queue.
client_id = -1 means all clients
*/
inline void Net_ToSQClient(int client_id, const std::string &name, float value)
{
  server_to_client_packets.push_back({client_id, name, value});
};

inline void Net_ToSQClient(int client_id, const std::string &name, Vector3 value)
{
  server_to_client_packets.push_back({client_id, name, value});
};

inline void Net_ToSQClient(int client_id, const std::string &name, int value)
{
  server_to_client_packets.push_back({client_id, name, value});
};

inline void Net_ToSQClient(int client_id, const std::string &name, bool value)
{
  server_to_client_packets.push_back({client_id, name, value});
};

inline void Net_ToSQClient(int client_id, const std::string &name, const std::string &value)
{
  server_to_client_packets.push_back({client_id, name, value});
};

// -----------------------------------------------------------------------
// Destination 3: SQ Server  (goes into client_to_server_packets queue)
// Use when a C++ system wants to trigger a handler in sv_packets.nut
// -----------------------------------------------------------------------

/*
Net_ToSQServer
Pushes a packet into the Squirrel server queue.
*/
inline void Net_ToSQServer(int client_id, const std::string &name, float value)
{
  client_to_server_packets.push_back({client_id, name, value});
};

inline void Net_ToSQServer(int client_id, const std::string &name, Vector3 value)
{
  client_to_server_packets.push_back({client_id, name, value});
};

inline void Net_ToSQServer(int client_id, const std::string &name, int value)
{
  client_to_server_packets.push_back({client_id, name, value});
};

inline void Net_ToSQServer(int client_id, const std::string &name, bool value)
{
  client_to_server_packets.push_back({client_id, name, value});
};

inline void Net_ToSQServer(int client_id, const std::string &name, const std::string &value)
{
  client_to_server_packets.push_back({client_id, name, value});
};