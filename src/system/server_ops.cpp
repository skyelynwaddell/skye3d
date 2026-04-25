#include "server_ops.h"
#include "player.h"

/*
SendToClient
Send a packet directly to the CPP client.cpp file of a certain user
Not to to lua Client :)
This helps to directly communicate with client when you dont want lua code to interfere
For example movement
*/
void SendToClient(ENetPeer *peer, uint8_t type, const void *data, size_t data_len)
{
  if (!peer)
    return;
  ENetPacket *packet = enet_packet_create(NULL, data_len + 1, ENET_PACKET_FLAG_RELIABLE);
  packet->data[0] = type;
  if (data && data_len > 0)
  {
    memcpy(packet->data + 1, data, data_len);
  }
  enet_peer_send(peer, 0, packet);
};

/*
SpawnPlayer
Creates a Player object on the server for the given id
*/
void SpawnPlayer(int id)
{
  // Host already created their own object via enetclient_update ASSIGN_ID
  // Just link the reference instead of creating a duplicate
  for (auto &obj : gameobjects)
  {
    if (obj->client_id == id)
    {
      users[id].object_ref = obj.get();
      obj->is_me = (id == my_local_player_id);
      return;
    }
  }

  // Remote client — no object exists yet, create it
  auto data = InfoPlayerStart();
  auto obj = InstanceCreate<Player>(data.origin);
  obj->client_id = id;
  obj->is_me = (id == my_local_player_id);
  camera->position = obj->position;
  users[id].object_ref = obj;

  obj->classname = "dummy_player";
  obj->collision_box = {1.0f, 2.0f, 1.0f};
  obj->collision_offset = {0.0f, 0.0f, 0.0f};
};

/*
ConnectUser
Called when a client connects to server.
*/
void ConnectUser(ENetPeer *peer)
{
  printf("[SERVER]: A new client connected from %x:%u. ",
         peer->address.host,
         peer->address.port);

  int id = -1;
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (users[i].peer == nullptr)
    {
      id = i;
      break;
    }
  }
  if (id < 0)
  {
    printf("[SERVER]: Server full, rejecting connection.\n");
    enet_peer_disconnect(peer, 0);
    return;
  }

  users[id].player_id = id;
  users[id].peer = peer;
  users[id].object_ref = nullptr;
  users[id].username[0] = '\0';
  peer->data = &users[id];
  if (id >= client_count)
    client_count = id + 1;

  SendToClient(peer, MESSAGE_TYPE_ASSIGN_ID, &id, sizeof(int));
  printf("ID: %d\n", id);
};

/*
RequestJoin
*/
void RequestJoin(int sender_id)
{
  if (sender_id < 0 || sender_id >= MAX_PLAYERS)
    return;

  if (users[sender_id].object_ref == nullptr)
    SpawnPlayer(sender_id);

  // Tell everyone about the new player
  for (int i = 0; i < MAX_PLAYERS; i++)
    if (users[i].peer)
      SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_SPAWN, &sender_id, sizeof(int));

  // Send the new player the current world state
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (!users[i].peer || i == sender_id)
      continue;

    SendToClient(users[sender_id].peer, MESSAGE_TYPE_INTERNAL_SPAWN, &i, sizeof(int));

    if (users[i].object_ref)
    {
      struct
      {
        int id;
        Vector3 pos;
      } pData = {i, users[i].object_ref->position};
      SendToClient(users[sender_id].peer, MESSAGE_TYPE_INTERNAL_POS_UPDATE, &pData, sizeof(pData));
    }
  }

  if (sender_id == my_local_player_id)
    Net_ToSQClient(sender_id, "request_join", (float)sender_id);
  else if (users[sender_id].peer)
    Net_ToCPPClient(users[sender_id].peer, "request_join", (float)sender_id);
};

/*
SetPosition
Updates authority position and rebroadcasts to all other clients
*/
void SetPosition(Vector3 new_pos, int sender_id)
{
  if (users[sender_id].object_ref)
    users[sender_id].object_ref->position = new_pos;

  struct
  {
    int id;
    Vector3 pos;
  } pData = {sender_id, new_pos};

  for (int i = 0; i < client_count; i++)
    if (users[i].peer && users[i].player_id != sender_id)
      SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_POS_UPDATE, &pData, sizeof(pData));
};

void SetAngle(float new_angle, int sender_id)
{
  if (users[sender_id].object_ref)
    users[sender_id].object_ref->angle = new_angle;

  struct
  {
    int id;
    float angle;
  } pData = {sender_id, new_angle};

  for (int i = 0; i < client_count; i++)
    if (users[i].peer && users[i].player_id != sender_id)
      SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_ANGLE_UPDATE, &pData, sizeof(pData));
};

/*
DisconnectUser
Removes a user from the list and destroys their game object.
*/
void DisconnectUser(ENetPeer *peer)
{
  if (!peer)
    return;

  for (int i = 0; i < client_count; i++)
  {
    if (users[i].peer != peer)
      continue;

    printf("[SERVER]: Player %d (%s) disconnected.\n", users[i].player_id, users[i].username);

    for (auto &obj : gameobjects)
      if (obj->client_id == users[i].player_id)
        obj->Destroy();

    users[i].peer = nullptr;
    users[i].object_ref = nullptr;
    users[i].username[0] = '\0';
    users[i].player_id = -1;
    break;
  }
  peer->data = nullptr;
};