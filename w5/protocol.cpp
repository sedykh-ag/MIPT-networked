#include "protocol.h"
#include "snapshot.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

void send_input_state(ENetPeer *peer, InputState state)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(InputState),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  memcpy(ptr, &state, sizeof(InputState)); ptr += sizeof(InputState);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint32_t time, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  memcpy(ptr, &time, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &x, sizeof(float)); ptr += sizeof(float);
  memcpy(ptr, &y, sizeof(float)); ptr += sizeof(float);
  memcpy(ptr, &ori, sizeof(float)); ptr += sizeof(float);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, Snapshot snapshot)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Snapshot),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  memcpy(ptr, &snapshot, sizeof(Snapshot)); ptr += sizeof(Snapshot);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_input_state(ENetPacket *packet, InputState &state)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  state = *(InputState*)(ptr); ptr += sizeof(InputState);
}

void deserialize_snapshot(ENetPacket *packet, uint32_t &time, uint16_t &eid, float &x, float &y, float &ori)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  time = *(uint32_t*)(ptr); ptr += sizeof(uint32_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  x = *(float*)(ptr); ptr += sizeof(float);
  y = *(float*)(ptr); ptr += sizeof(float);
  ori = *(float*)(ptr); ptr += sizeof(float);
}

void deserialize_snapshot(ENetPacket *packet, Snapshot &snapshot)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  snapshot = *(Snapshot*)(ptr); ptr += sizeof(Snapshot);
}
