#include "server_ops.h"
#include "player.h"

/*
SendToClient
Send a packet directly to the CPP client.cpp file of a certain user
Not to to squirrel Client :)
This helps to directly communicate with client when you dont want squirrel code to interfere
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
  auto obj = InstanceCreate<Player>({-50, 0, 5});
  obj->client_id = id;
  obj->is_me = (id == my_local_player_id);
  camera->position = obj->position;
  users[id].object_ref = obj;
};

/*
ConnectUser
Called when a client connects to server
*/
void ConnectUser(ENetPeer *peer)
{
  printf("[SERVER]: A new client connected from %x:%u. ",
         event.peer->address.host,
         event.peer->address.port);

  // add user
  if (users_count < MAX_PLAYERS)
  {
    int id = users_count++;
    users[id].player_id = id;
    users[id].peer = event.peer;
    event.peer->data = &users[id];
    peer->data = &users[id];
    SendToClient(peer, MESSAGE_TYPE_ASSIGN_ID, &id, sizeof(int));
    printf("ID: %d\n", id);
  }
};

/*
RequestJoin
Spawns the joining player and syncs world state to them
*/
void RequestJoin(int sender_id)
{
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
};

/*
SetPosition
Updates authority position and rebroadcasts to all other clients
*/
void SetPosition(Vector3 new_pos, int sender_id)
{
  if (users[sender_id].object_ref)
    users[sender_id].object_ref->position = new_pos;

  char buffer[sizeof(int) + sizeof(Vector3)];
  memcpy(buffer, &sender_id, sizeof(int));
  memcpy(buffer + sizeof(int), &new_pos, sizeof(Vector3));

  for (int i = 0; i < users_count; i++)
    if (users[i].peer && users[i].player_id != sender_id)
      SendToClient(users[i].peer, MESSAGE_TYPE_INTERNAL_POS_UPDATE, buffer, sizeof(buffer));
};

/*
DisconnectUser
Removes a user from the list and destroys their game object
*/
void DisconnectUser(ENetPeer *peer)
{
  for (int i = 0; i < users_count; i++)
  {
    if (users[i].peer != peer)
      continue;

    printf("[SERVER]: Player %d (%s) disconnected.\n", users[i].player_id, users[i].username);

    for (auto &obj : gameobjects)
      if (obj->client_id == users[i].player_id)
        obj->Destroy();

    for (int j = i; j < users_count - 1; j++)
      users[j] = users[j + 1];

    users_count--;
    break;
  }
  peer->data = NULL;
};