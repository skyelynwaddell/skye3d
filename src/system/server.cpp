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
#include "raylib.h"
#include "server_handlers.h"
#include "net_utils.h"
#include <format>
#include <gameobject3d.h>

ENetHost *server;
ENetAddress address;
ENetEvent event;
MultiplayerUser users[MAX_PLAYERS];
int client_count = 0;

/*
enetserver_init
Initializes the ENet server and starts listening on the specified port.
*/
int enetserver_init()
{
  client_count = 0;

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
    for (int i = 0; i < client_count; i++)
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

  if (sender_id < 0 || sender_id >= MAX_PLAYERS)
  {
    return;
  }

  // Validate packet size
  if (packet->dataLength < 1)
  {
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
      it->second(sender_id, payload, payload_len);
    }
    else
    {
      // Unknown to C++ — forward to lua
      client_to_server_packets.push_back({sender_id, name, vec});
    }
    break;
  }

  default:
    // printf("[SERVER]: Unknown message type: %d from player %d\n", message_type, sender_id);
    break;
  }
};

// In server.cpp or your main server logic
// client decoder stays unchanged.
static int last_client_count = 0;
static void enetserver_sync_entities()
{
  if (!server_online)
    return;

  // Check if a new client joined since the last tick
  bool force_full_sync = (client_count > last_client_count);
  last_client_count = client_count;

  for (auto &obj : gameobjects)
  {
    // If nothing changed and no new player needs a full update, skip
    if (obj->sync_flags == SYNC_NONE && !force_full_sync)
      continue;

    int id = obj->client_id;
    int current_flags = force_full_sync ? SYNC_ALL : obj->sync_flags;

    // Prepare Packets
    struct // pos
    {
      int id;
      Vector3 pos;
    } pPos = {id, obj->position};
    struct // angle
    {
      int id;
      float angle;
    } pAngle = {id, obj->angle};
    struct // size
    {
      int id;
      Vector3 box;
      Vector3 offset;
    } pSize = {id,
               obj->collision_box,
               obj->collision_offset};
    struct // classname
    {
      int id;
      char classname[64];
    } pClassname;
    pClassname.id = id;
    memset(pClassname.classname, 0, 64);
    obj->classname.copy(pClassname.classname, 63);

    struct // visible
    {
      int id;
      bool visible;
    } pVisible = {
        id,
        obj->visible,
    };

    struct
    {
      int id;
      char model_path[128];
      Vector3 scale;
    } pModel;
    pModel.id = id;
    memset(pModel.model_path, 0, 128);
    obj->game_model.model_path.copy(pModel.model_path, 127);
    pModel.scale = obj->game_model.scale;

    for (int i = 0; i < client_count; i++)
    {
      if (!users[i].peer || users[i].player_id == my_local_player_id)
        continue;

      if (current_flags & SYNC_POS)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_POS_UPDATE, &pPos, sizeof(pPos));

      if (current_flags & SYNC_ANGLE)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_ANGLE_UPDATE, &pAngle, sizeof(pAngle));

      if (current_flags & SYNC_SIZE)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_SIZE_UPDATE, &pSize, sizeof(pSize));

      if (current_flags & SYNC_CLASSNAME)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_CLASSNAME_UPDATE, &pClassname, sizeof(pClassname));

      if (current_flags & SYNC_VISIBLE)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_VISIBLE_UPDATE, &pVisible, sizeof(pVisible));

      if (current_flags & SYNC_MODEL)
        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_MODEL_UPDATE, &pModel, sizeof(pModel));

      if (current_flags & SYNC_SCRIPTVARS)
      {
        std::vector<uint8_t> buffer;

        // Pack the ID
        uint8_t *idPtr = (uint8_t *)&id;
        buffer.insert(buffer.end(), idPtr, idPtr + sizeof(int));

        // Pack the number of variables
        int varCount = (int)obj->script_vars.size();
        uint8_t *countPtr = (uint8_t *)&varCount;
        buffer.insert(buffer.end(), countPtr, countPtr + sizeof(int));

        for (auto const &[key, val] : obj->script_vars)
        {
          // Pack Key String (Length + Data)
          int keyLen = (int)key.length();
          buffer.insert(buffer.end(), (uint8_t *)&keyLen, (uint8_t *)&keyLen + sizeof(int));
          buffer.insert(buffer.end(), key.begin(), key.end());

          // Pack Variant Type
          int typeIndex = (int)val.index(); // 0=string, 1=float, 2=bool, 3=int
          buffer.insert(buffer.end(), (uint8_t *)&typeIndex, (uint8_t *)&typeIndex + sizeof(int));

          // Pack Variant Value
          if (typeIndex == 0)
          { // string
            std::string s = std::get<std::string>(val);
            int sLen = (int)s.length();
            buffer.insert(buffer.end(), (uint8_t *)&sLen, (uint8_t *)&sLen + sizeof(int));
            buffer.insert(buffer.end(), s.begin(), s.end());
          }
          else if (typeIndex == 1)
          { // float
            float f = std::get<float>(val);
            uint8_t *p = (uint8_t *)&f;
            buffer.insert(buffer.end(), p, p + sizeof(float));
          }
          else if (typeIndex == 2)
          { // bool
            bool b = std::get<bool>(val);
            buffer.push_back(b ? 1 : 0);
          }
          else if (typeIndex == 3)
          { // int
            int i = std::get<int>(val);
            uint8_t *p = (uint8_t *)&i;
            buffer.insert(buffer.end(), p, p + sizeof(int));
          }
        }

        SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_SCRIPTVARS_UPDATE, buffer.data(), buffer.size());
      }
    }

    // Reset flags after syncing (only if we weren't just forcing a full sync for a new guy)
    obj->sync_flags = SYNC_NONE;
  }
}

/*
enetserver_update
Polls for incoming messages from clients and handles them.
This function should be called in the main game loop to keep the server updated.
*/
void enetserver_update()
{
  enetserver_sync_entities();

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
  client_count = 0;
  memset(users, 0, sizeof(users));
  server_online = false;
  printf("[SERVER]: Server stopped.\n");
}
