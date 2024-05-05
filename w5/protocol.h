#pragma once
#include <enet/enet.h>
#include <cstdint>
#include "entity.h"
#include "snapshot.h"

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_INPUT,
  E_SERVER_TO_CLIENT_SNAPSHOT
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_input_state(ENetPeer *peer, InputState state);
void send_snapshot(ENetPeer *peer, Snapshot snapshot);

MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_input_state(ENetPacket *packet, InputState &state);
void deserialize_snapshot(ENetPacket *packet, Snapshot& snapshot);
