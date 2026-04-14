// client.cpp
#include "enet.h"
#include <cstdint>
#include "global.h"
#include "sq.h"
#include <net_utils.h>
#include <gameobject3d.h>
#include <player.h>

ENetHost *client = NULL;
ENetPeer *peer = NULL;

const char *msg = "Hello server!";
char buffer[1024];
size_t len;

/*
enetclient_init
Initializes the ENet client and connects to the server.
Returns true (1) on success, false (0) on failure.
*/
int enetclient_init(void)
{
  memset(users, 0, sizeof(users));

  if (enet_initialize() != 0)
  {
    fprintf(stderr, "[CLIENT]: ENet initialization failed.\n");
    return false;
  }
  atexit(enet_deinitialize);

  client = enet_host_create(NULL, 1, 2, 0, 0);
  if (!client)
  {
    fprintf(stderr, "[CLIENT]: Failed to create ENet client host.\n");
    return false;
  }

  if (!enetclient_connect(global_server_ip.c_str(), global_server_port))
  {
    fprintf(stderr, "[CLIENT]: Failed to connect to server.\n");
    return false;
  }

  printf("[CLIENT]: ENet client initialized successfully.\n");

  return true;
}

/*
enetclient_connect
Connects to the specified server using the provided host and port.
*/
int enetclient_connect(const char *host, unsigned short port)
{
  if (!client)
    return false;

  ENetAddress address;
  if (enet_address_set_host(&address, host) != 0)
  {
    fprintf(stderr, "[CLIENT]: Invalid host address.\n");
    return false;
  }
  address.port = port;

  peer = enet_host_connect(client, &address, 2, 0);
  if (!peer)
  {
    fprintf(stderr, "[CLIENT]: No available peers for connection.\n");
    return false;
  }

  for (int i = 0; i < 100; i++) // try for 5sec
  {
    if (server_online)
    {
      enetserver_update();
    }

    ENetEvent event;
    if (enet_host_service(client, &event, 50) > 0)
    {
      if (event.type == ENET_EVENT_TYPE_CONNECT)
      {
        printf("[CLIENT]: Connected to server %s:%u\n", host, port);
        global_client_init_called = true;
        return true;
      }
    }
  }

  printf("[CLIENT]: Connection to server timed out or failed.\n");
  return false;
}

/*
enetclient_send
Sends data to the connected server.
*/
int enetclient_send(const void *data, size_t len)
{
  if (!peer)
    return false;

  ENetPacket *packet = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE);
  if (!packet)
    return false;

  if (enet_peer_send(peer, 0, packet) != 0)
  {
    enet_packet_destroy(packet);
    return false;
  }
  enet_host_flush(client);
  return true;
}

/*
enetclient_poll
Polls for incoming messages from the server.
*/
int enetclient_poll(void *buffer, size_t maxlen, size_t *out_len)
{
  if (!client)
    return false;

  ENetEvent event;
  int ret = enet_host_service(client, &event, 0);
  if (ret > 0 && event.type == ENET_EVENT_TYPE_RECEIVE)
  {
    size_t packet_len = event.packet->dataLength;
    if (packet_len > maxlen)
      packet_len = maxlen;

    memcpy(buffer, event.packet->data, packet_len);
    *out_len = packet_len;
    enet_packet_destroy(event.packet);
    return true; // received a message
  }
  return false; // no message
}

/*
enetclient_disconnect
Disconnects from the server and cleans up resources.
*/
void enetclient_disconnect(void)
{
  if (peer)
  {
    enet_peer_disconnect(peer, 0);

    ENetEvent event;
    while (enet_host_service(client, &event, 3000) > 0)
    {
      if (event.type == ENET_EVENT_TYPE_DISCONNECT)
      {
        printf("[CLIENT]: Disconnected from server.\n");
        break;
      }
    }
    peer = NULL;
  }

  if (client)
  {
    enet_host_destroy(client);
    client = NULL;
  }
}

/*
send_example
An example function to send a message to the server.
Placeholder with different types of data you can send.
*/
static int send_example()
{
  uint8_t message_type = MESSAGE_TYPE_HELLO; // Example message type

  // example data to send
  int some_int = 42;
  const char *some_string = "Hello, ENet!";

  // calculate the total packet size in bytes
  size_t str_len = strlen(some_string) + 1; // +1 for null terminator
  size_t packet_size = 1 + sizeof(some_int) + str_len;
  ;

  // store the data in a buffer
  unsigned char buffer[256];
  buffer[0] = message_type; // First byte is the message type
  memcpy(buffer + 1, &some_int, sizeof(some_int));
  memcpy(buffer + sizeof(some_int) + 1, some_string, str_len);

  // put the buffer into an ENet packet
  ENetPacket *packet = enet_packet_create(buffer, packet_size, ENET_PACKET_FLAG_RELIABLE);
  if (!packet)
    return false;

  // send the packet to the server
  enet_peer_send(peer, 0, packet);
  enet_host_flush(peer->host);
  return true;
}

/*
enetclient_update
Sends a message to the server and processes incoming messages.
This function should be called in the main game loop to keep the client updated.
*/
void enetclient_update()
{
  if (!client)
    return;

  ENetEvent event;
  while (enet_host_service(client, &event, 0) > 0)
  {
    if (event.type == ENET_EVENT_TYPE_RECEIVE)
    {
      uint8_t type = event.packet->data[0];
      uint8_t *payload = event.packet->data + 1;

      if (type == MESSAGE_TYPE_PACKET_FLOAT)
      {
        float val;
        memcpy(&val, payload, sizeof(float));
        std::string _name = (char *)(payload + sizeof(float));

        // Push to client queue
        server_to_client_packets.push_back(Packet{
            .client_id = -1, // From server
            .name = _name,
            .value = val});
      }
      else if (type == MESSAGE_TYPE_PACKET_VECTOR3)
      {
        Vector3 vec;
        memcpy(&vec.x, payload, sizeof(float));
        memcpy(&vec.y, payload + sizeof(float), sizeof(float));
        memcpy(&vec.z, payload + (sizeof(float) * 2), sizeof(float));

        // Name starts after the 3 floats (12 bytes)
        std::string _name = (char *)(payload + (sizeof(float) * 3));
        server_to_client_packets.push_back(Packet{-1, _name, vec});
      }
      else if (type == MESSAGE_TYPE_INTERNAL_SPAWN)
      {
        int id_to_spawn;
        memcpy(&id_to_spawn, payload, sizeof(int));
        // printf("[CLIENT]: Spawn received: %d\n", id_to_spawn);

        if (id_to_spawn == my_local_player_id)
        {
          enet_packet_destroy(event.packet);
          continue;
        }

        bool already_exists = false;
        for (auto &gameobject : gameobjects)
        {
          if (gameobject->client_id == id_to_spawn)
          {
            already_exists = true;
            break;
          }
        }
        if (!already_exists)
        {
          // printf("[CLIENT]: Spawning proxy for player %d\n", id_to_spawn);
          Vector3 spawnpos = {0, 0, 0};
          auto obj = InstanceCreate<GameObject3D>(spawnpos);
          obj->client_id = id_to_spawn;
          users[id_to_spawn].object_ref = obj;
          users[id_to_spawn].player_id = id_to_spawn;
        }
      }
      else if (type == MESSAGE_TYPE_INTERNAL_POS_UPDATE)
      {
        int id;
        Vector3 pos;

        memcpy(&id, payload, sizeof(int));
        memcpy(&pos, payload + sizeof(int), sizeof(Vector3));
        enet_packet_destroy(event.packet);

        if (id == my_local_player_id)
          continue;

        for (auto &obj : gameobjects)
        {
          if (obj->client_id == id)
          {
            obj->position = pos;
            break;
          }
        }
        continue;
      }
      else if (type == MESSAGE_TYPE_ASSIGN_ID)
      {
        memcpy(&my_local_player_id, payload, sizeof(int));

        // Check if server already created our object (host case)
        bool found = false;
        for (auto &obj : gameobjects)
        {
          if (obj->client_id == my_local_player_id)
          {
            // Server already created it, just mark as ours
            obj->is_me = true;
            found = true;
            // printf("[CLIENT]: Using existing server object for player %d\n", my_local_player_id);
            break;
          }
        }

        // Only create if not found (client)
        if (!found)
        {
          // printf("[CLIENT]: Creating new object for player %d\n", my_local_player_id);
          Vector3 spawnpos = {0, 0, 0};
          auto obj = InstanceCreate<Player>(spawnpos);
          obj->client_id = my_local_player_id;
          obj->is_me = true;
          users[my_local_player_id].object_ref = obj;
          users[my_local_player_id].player_id = my_local_player_id;
        }

        enet_packet_destroy(event.packet);
        continue;
      }
      else if (type == MESSAGE_TYPE_UPDATE_SENDFLAGS)
      {
        int id;
        int sendflags;

        memcpy(&id, payload, sizeof(int));
        memcpy(&sendflags, payload + sizeof(int), sizeof(int));
        enet_packet_destroy(event.packet);

        if (id == my_local_player_id)
          continue;

        for (auto &obj : gameobjects)
        {
          if (obj->client_id == id)
          {
            obj->sendflags = sendflags;
            printf("Set sendflags to: %d\n", sendflags);
            break;
          }
        }
        continue;
      }

      enet_packet_destroy(event.packet);
    }
  }
}