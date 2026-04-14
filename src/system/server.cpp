// server.cpp
#include "enet.h"
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#include <global.h>
#include <engine.h>
#include <cstdint>
#include <string>
#include <sq.h>
#include "server_handlers.h"
#include "net_utils.h"
#include <format>

ENetHost *server;
ENetAddress address;
ENetEvent event;
MultiplayerUser users[MAX_PLAYERS];
int users_count = 0;

/*
enetserver_init
Initializes the ENet server and starts listening on the specified port.
*/
int enetserver_init()
{
  // Initialize ENet
  if (enet_initialize() != 0)
  {
    fprintf(stderr, "[SERVER]: ENet initialization failed.\n");
    return false;
  }
  atexit(enet_deinitialize);

  enet_address_set_host(&address, global_server_ip.c_str());
  address.host = ENET_HOST_ANY;
  address.port = global_server_port;
  server = enet_host_create(&address, 32, 2, 0, 0);

  my_local_player_id = 0;

  if (!server)
  {
    fprintf(stderr, "[SERVER]: Failed to create ENet server host.\n");
    return false;
  }

  printf(std::format("[SERVER]: Server Online! {}:{}\n", address.host, address.port).c_str());
  enet_host_flush(server);
  return true;
}

/*
enetserver_process_message
Processes incoming messages from clients.
*/
void enetserver_process_message(ENetEvent *event)
{
  ENetPacket *packet = event->packet;

  // Get the sender's player ID from the peer
  auto GetPlayerID = [&]()
  {
    int sender_id = -1;
    for (int i = 0; i < users_count; i++)
    {
      if (users[i].peer == event->peer)
      {
        sender_id = users[i].player_id;
        break;
      }
    }
    return sender_id;
  };

  // Validate sender
  int sender_id = GetPlayerID();
  // printf("[SERVER DEBUG]: Packet from sender_id %d\n", sender_id);

  if (sender_id < 0 || sender_id >= MAX_PLAYERS)
  {
    printf("[SERVER]: Received packet from unknown peer, ignoring.\n");
    return;
  }

  // Validate packet size
  if (packet->dataLength < 1)
  {
    printf("[SERVER]: Received empty packet.\n");
    return;
  }

  uint8_t message_type = packet->data[0];
  unsigned char *payload = packet->data + 1;
  size_t payload_len = packet->dataLength - 1;

  switch (message_type)
  {
  // MESSAGE_TYPE_CONNECT
  case MESSAGE_TYPE_CONNECT:
  {
    MultiplayerUser *stored_user = (MultiplayerUser *)event->peer->data;
    if (!stored_user)
      return;

    MultiplayerUser incoming_temp;
    memcpy(&incoming_temp, payload, sizeof(MultiplayerUser));
    strncpy(stored_user->username, incoming_temp.username, sizeof(stored_user->username));

    printf("[SERVER]: Player confirmed: ID=%d, Username=%s\n",
           stored_user->player_id, stored_user->username);

    SendToClient(event->peer, MESSAGE_TYPE_ASSIGN_ID, &stored_user->player_id, sizeof(int));
    break;
  }

  // MESSAGE_TYPE_PACKET_FLOAT
  case MESSAGE_TYPE_PACKET_FLOAT:
  {
    float val;
    memcpy(&val, payload, sizeof(float));
    std::string name = (char *)(payload + sizeof(float));

    auto it = float_handlers.find(name);
    if (it != float_handlers.end())
    {
      printf("[SERVER]: Handling float packet '%s' from player %d\n", name.c_str(), sender_id);
      it->second(sender_id, payload, payload_len);
    }
    else
    {
      // Unknown to C++ — forward to Squirrel
      client_to_server_packets.push_back({sender_id, name, val});
    }
    break;
  }

  // MESSAGE_TYPE_PACKET_VECTOR3
  case MESSAGE_TYPE_PACKET_VECTOR3:
  {
    Vector3 vec;
    memcpy(&vec.x, payload, sizeof(float));
    memcpy(&vec.y, payload + sizeof(float), sizeof(float));
    memcpy(&vec.z, payload + sizeof(float) * 2, sizeof(float));
    std::string name = (char *)(payload + sizeof(float) * 3);

    auto it = vec3_handlers.find(name);
    if (it != vec3_handlers.end())
    {
      // printf("[SERVER]: Handling vec3 packet '%s' from player %d\n", name.c_str(), sender_id);
      it->second(sender_id, payload, payload_len);
    }
    else
    {
      // Unknown to C++ — forward to Squirrel
      client_to_server_packets.push_back({sender_id, name, vec});
    }
    break;
  }

  default:
    printf("[SERVER]: Unknown message type: %d from player %d\n", message_type, sender_id);
    break;
  }
};

/*
enetserver_update
Polls for incoming messages from clients and handles them.
This function should be called in the main game loop to keep the server updated.
*/
void enetserver_update()
{
  while (enet_host_service(server, &event, 0) > 0)
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      ConnectUser(event.peer);
      break;

    case ENET_EVENT_TYPE_RECEIVE:
      enetserver_process_message(&event);
      enet_packet_destroy(event.packet);
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      DisconnectUser(event.peer);
      break;

    default:
      break;
    }
  }
}

/*
enetserver_start
Starts the ENet server and begins listening for incoming connections.
Called when user presses "Start Server" button.
*/
void enetserver_start()
{
  // start server
  if (enetserver_init())
  {
    server_online = true;
    return;
  }
  printf("[SERVER]: Failed to start server. Is another server already running?\n");
}

/*
enetserver_stop
Stops the ENet server and cleans up resources.
Called when user presses "Stop Server" button.
*/
void enetserver_stop()
{
  // stop server
  if (server != NULL)
  {
    enet_host_destroy(server);
    server = NULL;
  }
  users_count = 0;
  memset(users, 0, sizeof(users));
  server_online = false;
  printf("[SERVER]: Server stopped.\n");
}