#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOGDI  // All GDI defines and routines (Fixes Rectangle conflict)
#define NOUSER // All USER defines and routines (Fixes CloseWindow/ShowCursor conflict)

#include <cstdint>

extern "C"
{
#include <enet/include/enet.h>
}

#ifdef _WIN32
#undef near
#undef far
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#endif

#undef Rectangle
#undef CloseWindow
#undef ShowCursor

/* Server */
extern ENetHost *server;
extern ENetAddress address;
extern ENetEvent event;

/* Client */
extern ENetHost *client;
extern ENetPeer *peer;
extern const char *msg;
extern char buffer[1024];
extern size_t len;

typedef enum MESSAGE_TYPE
{
  MESSAGE_TYPE_HELLO = 0,
  MESSAGE_TYPE_CONNECT,
  MESSAGE_TYPE_PACKET_FLOAT,
  MESSAGE_TYPE_PACKET_VECTOR3,
  MESSAGE_TYPE_PACKET_STRING,
  MESSAGE_TYPE_INTERNAL_SPAWN,
  MESSAGE_TYPE_INTERNAL_POS_UPDATE,
  MESSAGE_TYPE_INTERNAL_ANGLE_UPDATE,
  MESSAGE_TYPE_INTERNAL_SIZE_UPDATE,
  MESSAGE_TYPE_INTERNAL_CLASSNAME_UPDATE,
  MESSAGE_TYPE_INTERNAL_VISIBLE_UPDATE,
  MESSAGE_TYPE_INTERNAL_MODEL_UPDATE,
  MESSAGE_TYPE_INTERNAL_SCRIPTVARS_UPDATE,
  MESSAGE_TYPE_ASSIGN_ID,
  MESSAGE_TYPE_UPDATE_SENDFLAGS
} MESSAGE_TYPE;

#pragma pack(push, 1)

class GameObject3D;
typedef struct MultiplayerUser
{
  ENetPeer *peer;
  int player_id;
  char username[64];
  GameObject3D *object_ref = nullptr;
  float last_pos_x = 0;
  float last_pos_y = 0;
  float last_pos_z = 0;
} MultiplayerUser;

typedef struct ENetMessage
{
  uint8_t message_type;
  MultiplayerUser user;
} ENetMessage;

#pragma pack(pop)

#define MAX_PLAYERS 64
extern MultiplayerUser users[MAX_PLAYERS];
extern int client_count;

/* ENet Client*/
int enetclient_init(void);                                         // Initialize the ENet client system (call once at startup)
int enetclient_connect(const char *host, unsigned short port);     // Connect to a server at host:port
int enetclient_send(const void *data, size_t len);                 // Send a message (data with length) to the server reliably
int enetclient_poll(void *buffer, size_t maxlen, size_t *out_len); // Poll for incoming messages, returns 1 if a message is received, 0 otherwise If received, fills buffer up to maxlen, returns actual length in out_len
void enetclient_disconnect(void);                                  // Disconnect and cleanup
void enetclient_update(void);                                      // Disconnect and cleanup

/* ENet Server*/
inline int server_online = false; // Flag to indicate if the server is online
int enetserver_init();
void enetserver_process_message(ENetEvent *event);
void enetserver_update();
void enetserver_start();
void enetserver_stop();

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 256

void parse_server_properties(const char *filename);