// server_ops.h
#pragma once
#include <cstdint> // For uint8_t
#include <cstddef> // For size_t
#include "enet.h"  // This defines ENetPeer and includes Raylib (for Vector3)
struct Vector3;

void SendToClient(ENetPeer *peer, uint8_t type, const void *data, size_t data_len);
void SpawnPlayer(int id);
void ConnectUser(ENetPeer *peer);
void RequestJoin(int sender_id);
void SetPosition(Vector3 new_pos, int sender_id);
void SetAngle(float new_angle, int sender_id);
void DisconnectUser(ENetPeer *peer);