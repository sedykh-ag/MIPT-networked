#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include "snapshot.h"
#include "constants.h"

void usleep(int us) // windows does not have usleep
{
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

static std::vector<Entity> entities;
static std::vector<InputState> input_states;
static std::map<uint16_t, ENetPeer*> controlledMap;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x44000000 * (rand() % 5) +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x000000FF;
  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  Entity ent = {0, color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, newEid};
  entities.push_back(ent);
  input_states.resize(entities.size());

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  InputState state{};
  deserialize_input_state(packet, state);
  for (size_t i = 0; i < entities.size(); i++)
  {
    if (entities[i].eid == state.eid)
      input_states[i] = state;
  }
  // std::cout << "received state eid: " << state.eid << '\n';
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  float lastTime = enet_time_get() * 0.001f; // [s]
  InputState state = { .eid = invalid_entity };
  
  float accumulatorTime = 0.0f; // [s]
  uint32_t tick = 0;

  while (true)
  {
    // update time
    float curTime = enet_time_get() * 0.001f; // [s]
    float dt = curTime - lastTime; // actual (not fixed) dt
    lastTime = curTime;
    tick = (uint32_t)ceil(curTime / fixedDt);

    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_INPUT:
            on_input(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    accumulatorTime += dt;
    while (accumulatorTime >= fixedDt)
    {
      accumulatorTime -= fixedDt;

      // simulate all entities
      for (size_t i = 0; i < entities.size(); i++)
      {
        entities[i] = simulate_entity(entities[i], input_states[i], fixedDt, serverNoise);
        entities[i].tick = input_states[i].tick;
      }

      // send snapshot to all clients
      if (tick % snapshotsInterval != 0)
        continue;
      
      Snapshot snapshot = { .tick = 0 };
      for (size_t i = 0; i < entities.size(); i++)
      {
        snapshot.eid[i] = entities[i].eid;
        snapshot.x[i] = entities[i].x;
        snapshot.y[i] = entities[i].y;
        snapshot.speed[i] = entities[i].speed;
        snapshot.ori[i] = entities[i].ori;
        snapshot.tick[i] = entities[i].tick; // CAUTION tick may be different for different entities!
      }
      snapshot.time = enet_time_get();
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        send_snapshot(peer, snapshot);
      }
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


